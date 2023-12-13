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

#include <warppipe/client.h>
#include <warppipe/config.h>
#include <warppipe/proto.h>
#include <warppipe/crc.h>
#include <warppipe/config.h>

void handle_dllp(struct warppipe_client_t *client, const struct pcie_dllp *pkt)
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

int client_send_pcie_transport(struct warppipe_client_t *client, struct warppipe_pcie_transport *tport)
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

void handle_memory_read_request(struct warppipe_client_t *client, const struct pcie_tlp *pkt)
{
	syslog(LOG_DEBUG, "Got read request TLP");
	if (!client->read_cb) {
		syslog(LOG_ERR, "Completer is missing pcie_read callback. Please register pcie_read function using `pcie_register_read_cb` before requesting memory read.");
		return;
	}
	int data_len = tlp_data_length(pkt);
	int data_len_bytes = tlp_data_length_bytes(pkt);
	uint64_t addr = tlp_req_get_addr(pkt);

	struct warppipe_pcie_transport *tport = calloc(1, sizeof(struct warppipe_pcie_transport) + data_len * 4);
	struct pcie_tlp *tlp = &tport->t_tlp.dl_tlp;

	tport->t_proto = PCIE_PROTO_TLP;
	tlp->tlp_fmt = PCIE_TLP_CPLD >> 5;
	tlp->tlp_type = PCIE_TLP_CPLD & 0x1F;
	tlp->tlp_length_hi = pkt->tlp_length_hi;
	tlp->tlp_length_lo = pkt->tlp_length_lo;
	tlp->tlp_cpl.c_tag = pkt->tlp_req.r_tag;
	tlp->tlp_cpl.c_byte_count_hi = data_len_bytes >> 8;
	tlp->tlp_cpl.c_byte_count_lo = data_len_bytes & 0xFF;

	int read_error = client->read_cb(addr, tlp->tlp_cpl.c_data, data_len_bytes, client->opaque);

	if (read_error)
		tlp->tlp_fmt &= ~PCIE_TLP_FMT_DATA;  // send Cpl instead of CplD to indicate failure

	// TODO: check status and resend if fail
	client_send_pcie_transport(client, tport);

	free(tport);
}

void handle_memory_write_request(struct warppipe_client_t *client, const struct pcie_tlp *pkt)
{
	syslog(LOG_DEBUG, "Got write request TLP");
	if (!client->write_cb) {
		syslog(LOG_ERR, "Completer is missing pcie_write callback. Please register pcie_write function using `pcie_register_write_cb` before requesting memory write.");
		return;
	}
	int data_len = tlp_data_length_bytes(pkt);
	uint64_t addr = tlp_req_get_addr(pkt);

	const void *data = pkt->tlp_fmt & PCIE_TLP_FMT_4DW ? pkt->tlp_req.r_data64 : pkt->tlp_req.r_data32;

	client->write_cb(addr, data, data_len, client->opaque);
}

void handle_completion(struct warppipe_client_t *client, const struct pcie_tlp *pkt)
{
	struct warppipe_completion_status_t completion_status;

	completion_status.error_code = 0;

	syslog(LOG_DEBUG, "Got completion TLP");
	if (client->completion_cb[pkt->tlp_cpl.c_tag] == NULL) {
		syslog(LOG_ERR, "Couldn't find read request for completion with tag: %d", pkt->tlp_req.r_tag);
		return;
	}

	int data_len = pkt->tlp_cpl.c_byte_count_hi << 8 | pkt->tlp_cpl.c_byte_count_lo;

	client->completion_cb[pkt->tlp_cpl.c_tag](completion_status, pkt->tlp_cpl.c_data, data_len);
	client->completion_cb[pkt->tlp_cpl.c_tag] = NULL;
}

void handle_tlp(struct warppipe_client_t *client, const struct pcie_tlp *pkt)
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

int warppipe_ack(struct warppipe_client_t *client, enum pcie_dllp_type type, uint16_t seqno)
{
	struct warppipe_pcie_transport tport = {
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

void warppipe_client_read(struct warppipe_client_t *client)
{
	struct warppipe_pcie_transport *tport = (void *)client->buf;
	int len = 0;

	len = recv(client->fd, tport, 1 + sizeof(tport->t_dllp), 0);
	if (len != 1 + sizeof(tport->t_dllp)) {
		if (len != -1 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
			syslog(LOG_NOTICE, "Client disconnecting: %s.", len < 0 ? strerror(errno) : "graceful EOF");
			client->active = false;
		}
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

			warppipe_ack(client, crc_ok ? PCIE_DLLP_ACK : PCIE_DLLP_NAK, tport->t_tlp.dl_seqno_hi << 8 | tport->t_tlp.dl_seqno_lo);
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

void warppipe_client_create(struct warppipe_client_t *client, int client_fd)
{
	client->fd = client_fd;
	client->seqno = 0;
	client->active = true;
	client->read_cb = NULL;
	client->write_cb = NULL;
	client->read_tag = 0;

	memset(client->completion_cb, 0, sizeof(warppipe_completion_cb_t) * 32);
}

void warppipe_register_read_cb(struct warppipe_client_t *client, warppipe_read_cb_t read_cb)
{
	client->read_cb = read_cb;
}

void warppipe_register_write_cb(struct warppipe_client_t *client, warppipe_write_cb_t write_cb)
{
	client->write_cb = write_cb;
}

int warppipe_read(struct warppipe_client_t *client, uint64_t addr, int length, warppipe_completion_cb_t completion_cb)
{
	int tag = client->read_tag++;
	struct warppipe_pcie_transport tport = {
		.t_proto = PCIE_PROTO_TLP,
		.t_tlp = {
			.dl_tlp = {
				.tlp_fmt = PCIE_TLP_MRD64 >> 5,
				.tlp_type = PCIE_TLP_MRD64 & 0x1F,
				.tlp_req = {
					.r_tag = tag,
				}
			},
		},
	};

	tlp_req_set_addr(&tport.t_tlp.dl_tlp, addr, length);

	if (client_send_pcie_transport(client, &tport) == -1)
		return -1;

	if (client->completion_cb[tag] != NULL)
		syslog(LOG_ERR, "Tried to send read request with already used tag!");

	client->completion_cb[tag] = completion_cb;

	return 0;
}

int warppipe_write(struct warppipe_client_t *client, uint64_t addr, const void *data, int length)
{
	int rc = 0;
	struct warppipe_pcie_transport *tport = malloc(sizeof(struct warppipe_pcie_transport) + ((length + (addr & 3) + 3) & ~3));

	struct pcie_tlp *tlp = &tport->t_tlp.dl_tlp;

	tport->t_proto = PCIE_PROTO_TLP;
	tlp->tlp_fmt = PCIE_TLP_MWR64 >> 5;
	tlp->tlp_type = PCIE_TLP_MWR64 & 0x1F;

	tlp_req_set_addr(tlp, addr, length);

	uint8_t *payload = tlp->tlp_fmt & PCIE_TLP_FMT_4DW ? tlp->tlp_req.r_data64 : tlp->tlp_req.r_data32;

	memcpy(payload + (addr & 3), data, length);

	rc = client_send_pcie_transport(client, tport);

	free(tport);

	return rc > 0 ? 0 : rc;
}
