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
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <pcie_comm/client.h>
#include <pcie_comm/config.h>
#include <pcie_comm/proto.h>
#include <pcie_comm/crc.h>

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

void client_cpl(struct client_t *client, const struct pcie_tlp *pkt, const void *data, int length)
{
	client->seqno++;
	struct pcie_transport tport = {
		.t_proto = PCIE_PROTO_TLP,
		.t_tlp = {
			.dl_seqno_hi = client->seqno >> 8,
			.dl_seqno_lo = client->seqno & 0xff,
			.dl_tlp = *pkt,
		},
	};
	tport.t_tlp.dl_tlp.tlp_fmt = PCIE_TLP_CPLD >> 5;
	tport.t_tlp.dl_tlp.tlp_type = PCIE_TLP_CPLD & 0x1F;
	int headerlen = tport.t_tlp.dl_tlp.tlp_cpl.c_data - (uint8_t *)&tport;
	int n = send(client->fd, &tport, headerlen, MSG_MORE);

	if (n < headerlen) {
		syslog(LOG_ERR, "Sending TLP: %s. Disconnecting.",
		       n < 0 ? strerror(errno) : "unexpected EOF");
		client->active = false;
		return;
	}
	int sent = 0;

	while (sent < length) {
		n = send(client->fd, data + sent, length - sent, MSG_MORE);
		if (n <= 0) {
			syslog(LOG_ERR, "Sending TLP: %s. Disconnecting.",
			       n < 0 ? strerror(errno) : "unexpected EOF");
			client->active = false;
			return;
		}
		sent += n;
	}

	uint32_t crc = crc32p(&tport.t_tlp, tport.t_tlp.dl_tlp.tlp_cpl.c_data, 0xffffffff, TLP_LCRC32_POLY);

	crc = ~crc32p(data, data + length, crc, TLP_LCRC32_POLY);

	uint8_t crc_data[4];

	for (int i = 0; i < 4; i++) {
		crc_data[i] = crc & 0xff;
		crc = crc >> 8;
	}

	n = send(client->fd, crc_data, 4, 0);
	if (n < 4) {
		syslog(LOG_ERR, "Sending TLP: %s. Disconnecting.",
		       n < 0 ? strerror(errno) : "unexpected EOF");
		client->active = false;
		return;
	}
}

void handle_tlp(struct client_t *client, const struct pcie_tlp *pkt)
{
	int data_len = tlp_data_length(pkt);

	switch ((enum pcie_tlp_type)(pkt->tlp_fmt << 5 | pkt->tlp_type)) {
	case PCIE_TLP_IORD:
	case PCIE_TLP_MRD32:
	case PCIE_TLP_MRD64:
		(void)pkt->tlp_req.r_address32;
		syslog(LOG_DEBUG, "Got read request TLP");
		client_cpl(client, pkt, "\xff\xff\xff\xff", data_len * 4);  /* TODO: call a handler */
		break;
	case PCIE_TLP_IOWR:
	case PCIE_TLP_MWR32:
	case PCIE_TLP_MWR64:
		(void)pkt->tlp_req.r_data32;
		syslog(LOG_DEBUG, "Got write request TLP");
		break;
	case PCIE_TLP_CPL:
	case PCIE_TLP_CPLD:
		(void)pkt->tlp_cpl.c_data;
		syslog(LOG_DEBUG, "Got completion TLP");
		break;
	case PCIE_TLP_MRDLK32:
	case PCIE_TLP_MRDLK64:
		syslog(LOG_DEBUG, "Got locked read request TLP");
		break;
	}
}

void client_ack(struct client_t *client, enum pcie_dllp_type type, uint16_t seqno)
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

	pcie_crc16(&tport.t_dllp);

	int n = send(client->fd, &tport, 1 + sizeof(struct pcie_dllp), 0);

	if (n != 1 + sizeof(struct pcie_dllp)) {
		syslog(LOG_ERR, "Sending DLLP: %s. Disconnecting.",
		       n < 0 ? strerror(errno) : "unexpected EOF");
		client->active = false;
		return;
	}
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
			int total = tlp_total_length(&tport->t_tlp.dl_tlp);

			if (total < 0) {  // error
				syslog(LOG_ERR, "Unknown TLP format: %d! Disconnecting.", tport->t_tlp.dl_tlp.tlp_fmt);
				client->active = false;
				return;
			}
			total += 7;  // 7 = 1B proto + 2B DL header + 4B DL LCRC32

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

	syslog(LOG_NOTICE, "New client connected!");
}
