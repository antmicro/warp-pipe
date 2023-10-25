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

#include <stdio.h>

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
	n = send(client->fd, "\xcc\xcc\xcc\xcc", 4, 0);  /* TODO: compute LCRC32, for now hardcode 0xcccccccc */
	if (n < 4) {
		syslog(LOG_ERR, "Sending TLP: %s. Disconnecting.",
		       n < 0 ? strerror(errno) : "unexpected EOF");
		client->active = false;
		return;
	}
}

void handle_tlp(struct client_t *client, const struct pcie_tlp *pkt)
{
	int data_len = pkt->tlp_length_hi << 8 | pkt->tlp_length_lo;

	data_len = ((data_len - 1) & 0x3FF) + 1;  // 0 means 1024

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
			.dl_type = type,
			.dl_acknak = {
				.dl_seqno_hi = seqno >> 8,
				.dl_seqno_lo = seqno & 0xff,
			},
			.dl_crc16_hi = 0xcc, .dl_crc16_lo = 0xcc,  /* TODO: compute crc16, for now hardcode 0xcccc */
		},
	};
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
		handle_dllp(client, &tport->t_dllp);
		break;
	case PCIE_PROTO_TLP:
		{
			int hdr_len = 0;
			int data_len = tport->t_tlp.dl_tlp.tlp_length_hi << 8 | tport->t_tlp.dl_tlp.tlp_length_lo;

			data_len = ((data_len - 1) & 0x3FF) + 1;  // 0 means 1024

			switch (tport->t_tlp.dl_tlp.tlp_fmt) {
			case 0:
				data_len = 0;
				[[fallthrough]];
			case 2:
				hdr_len = 3;  // 3 DW header
				break;
			case 1:
				data_len = 0;
				[[fallthrough]];
			case 3:
				hdr_len = 4;  // 4 DW header
				break;
			default:
				syslog(LOG_ERR, "Unknown TLP format: %d! Disconnecting.", tport->t_tlp.dl_tlp.tlp_fmt);
				client->active = false;
				return;
			}
			int total = 7 + (hdr_len + data_len) * 4;  // 7 = 1B proto + 2B DL header + 4B DL LCRC32

			assert(total < CLIENT_BUFFER_SIZE);
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
			client_ack(client, PCIE_DLLP_ACK, tport->t_tlp.dl_seqno_hi << 8 | tport->t_tlp.dl_seqno_lo);
			handle_tlp(client, &tport->t_tlp.dl_tlp);
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
