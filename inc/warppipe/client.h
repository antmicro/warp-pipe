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

#ifndef WARP_PIPE_CLIENT_H
#define WARP_PIPE_CLIENT_H

#include <sys/queue.h>
#include <stdbool.h>
#include <stdint.h>

#include <warppipe/config.h>
#include <warppipe/proto.h>

struct warppipe_completion_status_t {
	int error_code;
};

typedef int (*warppipe_read_cb_t)(uint64_t addr, void *data, int length, void *private_data);
typedef void (*warppipe_write_cb_t)(uint64_t addr, const void *data, int length, void *private_data);

typedef void (*warppipe_completion_cb_t)(const struct warppipe_completion_status_t completion_status, const void *data, int length, void *private_data);

/* client struct */
struct warppipe_client_t {
	int fd;
	int seqno;
	bool active;
	void *private_data;
	warppipe_read_cb_t bar_read_cb[6];
	warppipe_write_cb_t bar_write_cb[6];
	uint32_t bar[6];
	uint32_t bar_size[6];
	warppipe_read_cb_t cfg0_read_cb;
	warppipe_write_cb_t cfg0_write_cb;
	// 0x1F is maximum allowed tag
	warppipe_completion_cb_t completion_cb[32];
	uint8_t read_tag : 5;
	char buf[CLIENT_BUFFER_SIZE];
};

/* BSD TAILQ (sys/queue) node struct */
struct warppipe_client_node_t {
	TAILQ_ENTRY(warppipe_client_node_t) next;
	struct warppipe_client_t *client;
};

/* declare BSD TAILQ client linked-list */
TAILQ_HEAD(warppipe_client_q, warppipe_client_node_t);

void warppipe_client_create(struct warppipe_client_t *client, int client_fd);
void warppipe_client_read(struct warppipe_client_t *client);
int warppipe_ack(struct warppipe_client_t *client, enum pcie_dllp_type type, uint16_t seqno);

/* called on Completer to get config0 data */
void warppipe_register_config0_read_cb(struct warppipe_client_t *client, warppipe_read_cb_t warppipe_read_cb);
/* called on Completer to write config0 data */
void warppipe_register_config0_write_cb(struct warppipe_client_t *client, warppipe_write_cb_t warppipe_write_cb);
/* called on Completer to register new BAR with associated read/write callbacks */
int warppipe_register_bar(struct warppipe_client_t *client, uint64_t bar, uint32_t bar_size, int bar_idx, warppipe_read_cb_t read_cb, warppipe_write_cb_t write_cb);
/* called on Requester to send CR0 to Completer
 * param:
 *	client: Completer client
 *	addr (offset):   address from which Completer should read
 *	length: length of read (in bytes)
 * returns: error code
 *	0 - success
 *	-1 - network error
 */
int warppipe_config0_read(struct warppipe_client_t *client, uint64_t addr, int length, warppipe_completion_cb_t completion_cb);
/* called on Requester to send CW0 to Completer
 * param:
 *	client: Completer client
 *	addr (offset):   address where Completer should write
 *	data:   pointer contaning data to write
 *	length: length of data to write (in bytes)
 * returns: error code
 *	0 - success
 *	-1 - network error
 */
int warppipe_config0_write(struct warppipe_client_t *client, uint64_t addr, const void *data, int length);
/* called on Requester to send MRd to Completer
 * Requester needs to register completion callback and match
 * request with completion tag
 * param:
 *	client: Completer client
 *	addr:   address from which Completer should read
 *	length: length of read (in bytes)
 * returns: error code
 *	0 - success
 *	-1 - network error
 */
int warppipe_read(struct warppipe_client_t *client, int bar_idx, uint64_t addr, int length, warppipe_completion_cb_t completion_cb);
/* called on Requester to send MWr to Completer
 * param:
 *	client: Completer client
 *	addr:   address where Completer should write
 *	data:   pointer contaning data to write
 *	length: length of data to write (in bytes)
 * returns: error code
 *	0 - success
 *	-1 - network error
 */
int warppipe_write(struct warppipe_client_t *client, int bar_idx, uint64_t addr, const void *data, int length);

#endif /* WARP_PIPE_CLIENT_H */
