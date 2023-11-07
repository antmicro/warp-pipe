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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>

#include <pcie_comm/server.h>
#include <pcie_comm/client.h>
#include <pcie_comm/config.h>

static inline void server_track_max_fd(struct server_t *server, int new_fd)
{
	if (server->max_fd < new_fd)
		server->max_fd = new_fd;
}

static bool server_should_disconnect_client(struct client_t *client)
{
	return !client->active;
}

void server_disconnect_clients(struct server_t *server, bool (*condition)(struct
			client_t *client))
{
	struct client_node_t *i, *tmp;

	for (i = TAILQ_FIRST(&server->clients); i != NULL; i = tmp) {
		tmp = TAILQ_NEXT(i, next);
		if ((condition == NULL) || condition(i->client)) {
			close(i->client->fd);
			free(i->client);
			TAILQ_REMOVE(&server->clients, i, next);
			free(i);
			syslog(LOG_NOTICE, "Client disconnected!");
		}
	}
}

static void server_read(struct server_t *server)
{
	struct client_node_t *i;

	TAILQ_FOREACH(i, &server->clients, next)
		if (FD_ISSET(i->client->fd, &server->read_fds))
			client_read(i->client);
}


static int server_accept(struct server_t *server)
{
	int fd, ret;
	struct sockaddr_in sock_addr;
	socklen_t sock_addr_len = sizeof(sock_addr);
	struct client_t *new_client;
	struct client_node_t *new_client_node;

	ret = listen(server->fd, SERVER_LISTEN_QUEUE_SIZE);
	if (ret == -1) {
		perror("Failed to listen for connections on the socket!");
		return -1;
	}

	fd = accept(server->fd, (struct sockaddr *)&sock_addr, &sock_addr_len);
	if (fd == -1)
		return fd;

	new_client = malloc(sizeof(struct client_t));
	if (!new_client)
		goto fail;

	new_client_node = malloc(sizeof(struct client_node_t));
	if (!new_client_node)
		goto fail_free_client;

	client_create(new_client, fd);
	new_client_node->client = new_client;
	TAILQ_INSERT_TAIL(&server->clients, new_client_node, next);

	server_track_max_fd(server, fd);

	return 0;

fail_free_client:
	free(new_client);
fail:
	return -1;
}

int server_create(struct server_t *server)
{
	int fd_flags, ret;
	char in_addr[INET_ADDRSTRLEN];
	struct sockaddr_in sock_addr = {
		.sin_addr = {INADDR_ANY},
		.sin_port = htons(SERVER_PORT_NUM),
		.sin_family = AF_INET,
	};

	/* set up server socket */
	server->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server->fd == -1) {
		perror("Failed to create a socket!");
		return -1;
	}
	server->max_fd = server->fd;

	/* set the socket to be non-blocking */
	fd_flags = fcntl(server->fd, F_GETFL, 0);
	fcntl(server->fd, F_SETFL, fd_flags | O_NONBLOCK);

	ret = bind(server->fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
	if (ret == -1) {
		perror("Failed to bind the socket!");
		return -1;
	}

	/* set up client linked list */
	TAILQ_INIT(&server->clients);

	inet_ntop(AF_INET, &(sock_addr.sin_addr), in_addr, INET_ADDRSTRLEN);
	syslog(LOG_NOTICE, PRJ_NAME_LONG " listening on %s:%d.", in_addr,
			ntohs(sock_addr.sin_port));

	return 0;
}

void server_loop(struct server_t *server)
{
	struct client_node_t *i;

	/* set up descriptors sets */
	FD_ZERO(&server->read_fds);
	FD_SET(server->fd, &server->read_fds);
	TAILQ_FOREACH(i, &server->clients, next)
		FD_SET(i->client->fd, &server->read_fds);

	/* 1 sec delay for select */
	struct timeval tv = {
		.tv_sec = 1,
		.tv_usec = 0,
	};
	select(server->max_fd + 1, &server->read_fds, NULL, NULL, &tv);

	/* check if there's any incoming connection */
	if (FD_ISSET(server->fd, &server->read_fds))
		server_accept(server);

	/* read loop */
	server_read(server);

	/* remove inactive clients */
	server_disconnect_clients(server, server_should_disconnect_client);
}
