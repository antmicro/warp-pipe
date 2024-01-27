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

#ifndef WARP_PIPE_SERVER_H
#define WARP_PIPE_SERVER_H

#include <sys/select.h>

#include <stdbool.h>

#include <warppipe/client.h>
#include <warppipe/config.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*warppipe_server_accept_cb_t)(struct warppipe_client *client, void *private_data);

struct warppipe_server {
	/* server's socket fd */
	int fd;

	/* whether the server is in listening mode */
	bool listen;

	/* quit request */
	bool quit;

	/* server address family */
	int addr_family;

	/* server host address */
	const char *host;

	/* server port */
	const char *port;

	/* track max fd number for select */
	int max_fd;

	/* file descriptor set that will be checked with select */
	fd_set read_fds;

	/* client linked-list */
	struct warppipe_client_q clients;

	/* optional parameter to server_client_accept_cb function */
	void *private_data;

	/* called after new client is accepted */
	warppipe_server_accept_cb_t accept_cb;
};

int warppipe_server_create(struct warppipe_server *server);
void warppipe_server_loop(struct warppipe_server *server);
void warppipe_server_disconnect_clients(struct warppipe_server *server, bool
		(*condition)(struct warppipe_client *client));
void warppipe_server_register_accept_cb(struct warppipe_server *server, warppipe_server_accept_cb_t server_accept_cb);

#ifdef __cplusplus
}
#endif

#endif /* WARP_PIPE_SERVER_H */
