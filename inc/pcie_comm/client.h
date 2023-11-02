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

#ifndef PCIE_COMM_CLIENT_H
#define PCIE_COMM_CLIENT_H

#include <sys/queue.h>
#include <stdbool.h>
#include <stdint.h>

#include <pcie_comm/config.h>
#include <pcie_comm/proto.h>

struct completion_status_t {
	int error_code;
};

typedef void (*pcie_read_cb_t)(uint64_t addr, void *data, int length);
typedef void (*pcie_write_cb_t)(uint64_t addr, const void *data, int length);

typedef void (*pcie_completion_cb_t)(const struct completion_status_t completion_status, const void *data, int length);

/* client struct */
struct client_t {
	int fd;
	int seqno;
	bool active;
	char buf[CLIENT_BUFFER_SIZE];
	pcie_read_cb_t pcie_read_cb;
	pcie_write_cb_t pcie_write_cb;
	// 0x1F is maximum allowed tag
	pcie_completion_cb_t pcie_completion_cb[32];
	uint8_t pcie_read_tag;

};
/* BSD TAILQ (sys/queue) node struct */
struct client_node_t {
	TAILQ_ENTRY(client_node_t) next;
	struct client_t *client;
};

/* declare BSD TAILQ client linked-list */
TAILQ_HEAD(client_q, client_node_t);

void client_create(struct client_t *client, int client_fd);
void client_read(struct client_t *client);
void client_ack(struct client_t *client, enum pcie_dllp_type type, uint16_t seqno);

/* called on Completer to get data for Completion with data (CplD) response */
void pcie_register_read_cb(struct client_t *client, pcie_read_cb_t pcie_read_cb);
/* called on Completer to perform requested data write */
void pcie_register_write_cb(struct client_t *client, pcie_write_cb_t pcie_write_cb);
/* called on Requester to send MRd to Completer
 * Requester needs to register completion callback and match
 * request with completion tag
 * param:
 *	client: Completer client
 *	addr:   address from which Completer should read
 *	length: length of read (in DW)
 * returns: error code
 *	0 - success
 *	-1 - network error
 */
int pcie_read(struct client_t *client, uint64_t addr, int length, pcie_completion_cb_t pcie_completion_cb);
/* called on Requester to send MWr to Completer
 * param:
 *	client: Completer client
 *	addr:   address where Completer should write
 *	data:   pointer contaning data to write
 *	length: length of data to write (in DW)
 * returns: error code
 *	0 - success
 *	-1 - network error
 */
int pcie_write(struct client_t *client, uint64_t addr, const void *data, int length);

#endif /* PCIE_COMM_CLIENT_H */
