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

#ifndef PCIE_COMM_SERVER_H
#define PCIE_COMM_SERVER_H

#include <sys/select.h>

#include <pcie_comm/client.h>

struct server_t {
	/* server's socket fd */
	int fd;

	/* track max fd number for select */
	int max_fd;

	/* file descriptor set that will be checked with select */
	fd_set read_fds;

	/* client linked-list */
	struct client_q clients;
};

int server_create(struct server_t *server);
void server_loop(struct server_t *server);
void server_disconnect_clients(struct server_t *server, bool
		(*condition)(struct client_t *client));

#endif /* PCIE_COMM_SERVER_H */
