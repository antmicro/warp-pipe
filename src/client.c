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

#include <pcie_comm/client.h>
#include <pcie_comm/config.h>

#include <stdio.h>

void client_read(struct client_t *client)
{
	char buf[CLIENT_BUFFER_SIZE];
	int len = 0;

	len = recv(client->fd, buf, CLIENT_BUFFER_SIZE, 0);
	if (len == 0) {
		client->active = false;
		return;
	}

	send(client->fd, "Hello, world!\n", 14, 0);
}

void client_create(struct client_t *client, int client_fd)
{
	client->fd = client_fd;
	client->active = true;

	syslog(LOG_NOTICE, "New client connected!");
}
