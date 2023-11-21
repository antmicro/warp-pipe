/*
 * Copyright 2023 Antmicro <www.antmicro.com>
 * Copyright 2023 Meta
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/socket.h>

#include <stdlib.h>
#include <sys/types.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

#include <pcie_comm/client.h>
#include <pcie_comm/config.h>
#include <pcie_comm/proto.h>
#include <pcie_comm/crc.h>
#include <pcie_comm/config.h>

void handle_dllp(struct client_t *client, const struct pcie_dllp *pkt)
{
	if (pkt->dl_type == PCIE_DLLP_ACK || pkt->dl_type == PCIE_DLLP_NAK) {
		uint16_t seqno = pkt->dl_acknak.dl_seqno_hi << 8 | pkt->dl_acknak.dl_seqno_lo;

		syslog(LOG_DEBUG, "Got ACK/NAK DLLP for seqno = 0x%03x", seqno);
	} else if (pkt->dl_fc.fc_type != 0 && pkt->dl_fc.fc_rsvd1 == 0) {
		(void)pkt->dl_fc;
		syslog(LOG_DEBUG, "Got credit DLLP");
	} else {
		syslog(LOG_WARNING, "Unknown DLLP type: %d", pkt->dl_type);
	}
}

int client_send_pcie_transport(struct client_t *client, struct pcie_transport *tport)
{
	int packet_length = 1 + sizeof(tport->t_dllp);

	if (tport->t_proto == PCIE_PROTO_TLP) {
		client->seqno++;
		tport->t_tlp.dl_seqno_hi = client->seqno >> 8;
		tport->t_tlp.dl_seqno_lo = client->seqno & 0xff;
		pcie_lcrc32(&tport->t_tlp);
		packet_length += tlp_total_length(&tport->t_tlp.dl_tlp);
	} else if (tport->t_proto == PCIE_PROTO_DLLP) {
		pcie_crc16(&tport->t_dllp);
	}

	int n = send(client->fd, tport, packet_length, 0);

	if (n != packet_length) {
		syslog(LOG_ERR, "Sending transport packet: %s. Disconnecting.",
		       n < 0 ? strerror(errno) : "unexpected EOF");
		client->active = false;
		return -1;
	}
	syslog(LOG_DEBUG, "Send pcie transport length: %d", packet_length);
	return n;
}

void handle_memory_read_request(struct client_t *client, const struct pcie_tlp *pkt)
{
	syslog(LOG_DEBUG, "Got read request TLP");
	if (!client->pcie_read_cb) {
		syslog(LOG_ERR, "Completer is missing pcie_read callback. Please register pcie_read function using `pcie_register_read_cb` before requesting memory read.");
		return;
	}
	int data_len = tlp_data_length(pkt);
	uint64_t addr = tlp_req_get_addr(pkt);

	struct pcie_transport *tport = calloc(1, sizeof(struct pcie_transport) + data_len * 4);
	struct pcie_tlp *tlp = &tport->t_tlp.dl_tlp;

	tport->t_proto = PCIE_PROTO_TLP;
	tlp->tlp_fmt = PCIE_TLP_CPLD >> 5;
	tlp->tlp_type = PCIE_TLP_CPLD & 0x1F;
	tlp->tlp_length_hi = pkt->tlp_length_hi;
	tlp->tlp_length_lo = pkt->tlp_length_lo;
	tlp->tlp_cpl.c_tag = pkt->tlp_req.r_tag;

	int read_error = client->pcie_read_cb(addr, tlp->tlp_cpl.c_data, data_len * 4, client->opaque);

	if (read_error)
		tlp->tlp_fmt &= ~PCIE_TLP_FMT_DATA;  // send Cpl instead of CplD to indicate failure

	// TODO: check status and resend if fail
	client_send_pcie_transport(client, tport);

	free(tport);
}

void handle_memory_write_request(struct client_t *client, const struct pcie_tlp *pkt)
{
	syslog(LOG_DEBUG, "Got write request TLP");
	if (!client->pcie_write_cb) {
		syslog(LOG_ERR, "Completer is missing pcie_write callback. Please register pcie_write function using `pcie_register_write_cb` before requesting memory write.");
		return;
	}
	int data_len = tlp_data_length(pkt) * 4;
	uint64_t addr = tlp_req_get_addr(pkt);

	const void *data = pkt->tlp_fmt & PCIE_TLP_FMT_4DW ? pkt->tlp_req.r_data64 : pkt->tlp_req.r_data32;

	client->pcie_write_cb(addr, data, data_len, client->opaque);
}

void handle_completion(struct client_t *client, const struct pcie_tlp *pkt)
{
	struct completion_status_t completion_status;

	completion_status.error_code = 0;

	syslog(LOG_DEBUG, "Got completion TLP");
	if (client->pcie_completion_cb[pkt->tlp_cpl.c_tag] == NULL) {
		syslog(LOG_ERR, "Couldn't find read request for completion with tag: %d", pkt->tlp_req.r_tag);
		return;
	}

	int data_len = tlp_data_length(pkt) * 4;

	client->pcie_completion_cb[pkt->tlp_cpl.c_tag](completion_status, pkt->tlp_cpl.c_data, data_len);
	client->pcie_completion_cb[pkt->tlp_cpl.c_tag] = NULL;
}

void handle_tlp(struct client_t *client, const struct pcie_tlp *pkt)
{
	switch ((enum pcie_tlp_type)(pkt->tlp_fmt << 5 | pkt->tlp_type)) {
	case PCIE_TLP_IORD:
	case PCIE_TLP_MRD32:
	case PCIE_TLP_MRD64:
		handle_memory_read_request(client, pkt);
		break;
	case PCIE_TLP_IOWR:
	case PCIE_TLP_MWR32:
	case PCIE_TLP_MWR64:
		handle_memory_write_request(client, pkt);
		break;
	case PCIE_TLP_CPL:
		/* no data, used for IO, configuration write, read completition with error */
		break;
	case PCIE_TLP_CPLD:
		handle_completion(client, pkt);
		break;
	case PCIE_TLP_MRDLK32:
	case PCIE_TLP_MRDLK64:
		syslog(LOG_DEBUG, "Got locked read request TLP");
		break;
	}
}

int client_ack(struct client_t *client, enum pcie_dllp_type type, uint16_t seqno)
{
	struct pcie_transport tport = {
		.t_proto = PCIE_PROTO_DLLP,
		.t_dllp = {
			.dl_acknak = {
				.dl_nak = type,
				.dl_seqno_hi = seqno >> 8,
				.dl_seqno_lo = seqno & 0xff,
			},
		},
	};

	if (client_send_pcie_transport(client, &tport) == -1)
		return -1;

	return 0;
}

void client_read(struct client_t *client)
{
	struct pcie_transport *tport = (void *)client->buf;
	int len = 0;

	len = recv(client->fd, tport, 1 + sizeof(tport->t_dllp), 0);
	if (len != 1 + sizeof(tport->t_dllp)) {
		syslog(LOG_NOTICE, "Client disconnecting: %s.", len < 0 ? strerror(errno) : "graceful EOF");
		client->active = false;
		return;
	}

	switch ((enum pcie_proto)tport->t_proto) {
	case PCIE_PROTO_DLLP:
		if (pcie_crc16_valid(&tport->t_dllp))
			handle_dllp(client, &tport->t_dllp);
		else
			syslog(LOG_WARNING, "DLLP corrupted CRC");
		break;
	case PCIE_PROTO_TLP:
		{
			syslog(LOG_DEBUG, "Got TLP packet");
			int total = tlp_total_length(&tport->t_tlp.dl_tlp);

			if (total < 0) {  // error
				syslog(LOG_ERR, "Unknown TLP format: %d! Disconnecting.", tport->t_tlp.dl_tlp.tlp_fmt);
				client->active = false;
				return;
			}
			total += 7;  // 7 = 1B proto + 2B DL header + 4B DL LCRC32
			syslog(LOG_DEBUG, "Recieved TLP packed len: %d", total + len);

			assert(total <= CLIENT_BUFFER_SIZE);
			while (len < total) {
				int n = recv(client->fd, client->buf + len, total - len, 0);

				if (n <= 0) {
					syslog(LOG_ERR, "Receiving TLP: %s. Disconnecting.",
					       n < 0 ? strerror(errno) : "unexpected EOF");
					client->active = false;
					return;
				}
				len += n;
			}
			bool crc_ok = pcie_lcrc32_valid(&tport->t_tlp);

			client_ack(client, crc_ok ? PCIE_DLLP_ACK : PCIE_DLLP_NAK, tport->t_tlp.dl_seqno_hi << 8 | tport->t_tlp.dl_seqno_lo);
			if (crc_ok)
				handle_tlp(client, &tport->t_tlp.dl_tlp);
			else
				syslog(LOG_WARNING, "TLP corrupted CRC");
			break;
		}
	default:
		syslog(LOG_ERR, "Unknown PCIe protocol: %d! Disconnecting.", tport->t_proto);
		client->active = false;
		return;
	}
}

void client_create(struct client_t *client, int client_fd)
{
	client->fd = client_fd;
	client->seqno = 0;
	client->active = true;
	client->pcie_read_cb = NULL;
	client->pcie_write_cb = NULL;
	client->pcie_read_tag = 0;

	memset(client->pcie_completion_cb, 0, sizeof(pcie_completion_cb_t) * 32);
}

void pcie_register_read_cb(struct client_t *client, pcie_read_cb_t pcie_read_cb)
{
	client->pcie_read_cb = pcie_read_cb;
}

void pcie_register_write_cb(struct client_t *client, pcie_write_cb_t pcie_write_cb)
{
	client->pcie_write_cb = pcie_write_cb;
}

int pcie_read(struct client_t *client, uint64_t addr, int length, pcie_completion_cb_t pcie_completion_cb)
{
	struct pcie_transport tport = {
		.t_proto = PCIE_PROTO_TLP,
		.t_tlp = {
			.dl_tlp = {
				.tlp_fmt = PCIE_TLP_MRD64 >> 5,
				.tlp_type = PCIE_TLP_MRD64 & 0x1F,
				.tlp_req = {
					.r_tag = ++client->pcie_read_tag,
				}
			},
		},
	};

	tlp_req_set_addr(&tport.t_tlp.dl_tlp, addr, length);

	if (client_send_pcie_transport(client, &tport) == -1)
		return -1;

	if (client->pcie_completion_cb[client->pcie_read_tag] != NULL)
		syslog(LOG_ERR, "Tried to send read request with already used tag!");

	client->pcie_completion_cb[client->pcie_read_tag] = pcie_completion_cb;

	return 0;
}

int pcie_write(struct client_t *client, uint64_t addr, const void *data, int length)
{
	int rc = 0;
	struct pcie_transport *tport = malloc(sizeof(struct pcie_transport) + ((length + (addr & 3) + 3) & ~3));

	struct pcie_tlp *tlp = &tport->t_tlp.dl_tlp;

	tport->t_proto = PCIE_PROTO_TLP;
	tlp->tlp_fmt = PCIE_TLP_MWR64 >> 5;
	tlp->tlp_type = PCIE_TLP_MWR64 & 0x1F;

	tlp_req_set_addr(tlp, addr, length);

	void *payload = tlp->tlp_fmt & PCIE_TLP_FMT_4DW ? tlp->tlp_req.r_data64 : tlp->tlp_req.r_data32;

	memcpy(payload + (addr & 3), data, length);

	rc = client_send_pcie_transport(client, tport);

	free(tport);

	return rc > 0 ? 0 : rc;
}
