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

/* client struct */
struct client_t {
	int fd;
	bool active;
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

#endif /* PCIE_COMM_CLIENT_H */
