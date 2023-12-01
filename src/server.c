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

#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <stddef.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/signal.h>

#include <stdio.h>
#include <unistd.h>

#include <pcie_comm/server.h>
#include <pcie_comm/client.h>
#include <pcie_comm/config.h>

#ifndef NI_MAXSERV
#define NI_MAXSERV 32
#endif
#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif

typedef void (*sig_t) (int);
static sig_t sigint_handler;
static struct server_t *pcie_server;

static void handle_sigint(int signo)
{
	/* if previously we would ignore signal, ignore this one too */
	if (sigint_handler == SIG_IGN)
		return;
	syslog(LOG_NOTICE, "Received SIGINT. " PRJ_NAME_LONG " will shut down.");
	if (pcie_server) {
		pcie_server->quit = true;

		/* disconnect every user */
		server_disconnect_clients(pcie_server, NULL);
		close(pcie_server->fd);
	}

	/* reset signal handler and raise it again */
	if (sigint_handler == SIG_DFL) {
		signal(signo, SIG_DFL);
		raise(signo);
		signal(signo, handle_sigint);
	/* call previous handler */
	} else if (sigint_handler)
		sigint_handler(signo);
}

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
			if (!server->listen)
				server->quit = true;
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
	struct sockaddr_storage sock_addr;
	socklen_t sock_addr_len = sizeof(sock_addr);
	struct client_t *new_client;
	struct client_node_t *new_client_node;
	char host[NI_MAXHOST];
	char port[NI_MAXSERV];

	if (server->listen) {
		fd = accept(server->fd, (struct sockaddr *)&sock_addr, &sock_addr_len);
		if (fd == -1) {
			perror("accept");
			return -1;
		}

		ret = getnameinfo((struct sockaddr *)&sock_addr, sock_addr_len, host, sizeof(host), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
		if (ret != 0) {
			fprintf(stderr, "getnameinfo: %s\n", gai_strerror(ret));
			return -1;
		}

		syslog(LOG_NOTICE, "New client connection from %s:%s", host, port);
	} else {
		if (!TAILQ_EMPTY(&server->clients))
			return -1;
		fd = server->fd;
	}

	new_client = malloc(sizeof(struct client_t));
	if (!new_client)
		goto fail;

	new_client_node = malloc(sizeof(struct client_node_t));
	if (!new_client_node)
		goto fail_free_client;

	client_create(new_client, fd);
	new_client_node->client = new_client;
	TAILQ_INSERT_TAIL(&server->clients, new_client_node, next);
	if (server->server_client_accept_cb)
		server->server_client_accept_cb(new_client);

	server_track_max_fd(server, fd);

	return 0;

fail_free_client:
	free(new_client);
fail:
	return -1;
}

void server_register_accept_cb(struct server_t *server, server_client_accept_cb_t server_client_accept_cb)
{
	server->server_client_accept_cb = server_client_accept_cb;
}

int server_create(struct server_t *server)
{
	int fd_flags, ret;
	char host[NI_MAXHOST];
	char port[NI_MAXSERV];
	int sfd = -1;
	int enable = 1;

	struct addrinfo  hints;
	struct addrinfo  *result, *rp;

	/* save current server for signal handler */
	pcie_server = server;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = server->addr_family;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = (server->listen ? AI_PASSIVE : 0) | AI_ADDRCONFIG;
	hints.ai_protocol = 0;

	ret = getaddrinfo(server->host, server->port, &hints, &result);

	if (ret != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
	}

	/* set up server socket */
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;

		if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0) {
			perror("Failed to set SO_REUSEADDR: ");
			return -1;
		}
#ifdef SO_REUSEPORT
		if (setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) != 0) {
			perror("Failed to set SO_REUSEPORT: ");
			return -1;
		}
#endif

		if ((server->listen ? bind : connect)(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			if (server->listen) {
				ret = listen(sfd, SERVER_LISTEN_QUEUE_SIZE);
				if (ret == -1) {
					perror("Failed to listen for connections on the socket!");
					return -1;
				}

			}

			struct sockaddr_storage addr;
			socklen_t addrlen = sizeof(addr);

			ret = (server->listen ? getsockname : getpeername)(sfd, (struct sockaddr *)&addr, &addrlen);
			if (ret != 0) {
				perror(server->listen ? "getsockname" : "getpeername");
				return -1;
			}

			ret = getnameinfo((struct sockaddr *)&addr, addrlen, host, sizeof(host), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
			if (ret != 0) {
				fprintf(stderr, "getnameinfo: %s\n", gai_strerror(ret));
				return -1;
			}

			syslog(LOG_NOTICE, PRJ_NAME_LONG " %s %s:%s.", server->listen ? "listening on" : "connected to", host, port);
			break;
		}

		close(sfd);
	}

	freeaddrinfo(result);

	if (rp == NULL) {
		perror(sfd == -1 ? "Failed to create a socket" :
		       server->listen ? "Failed to bind the socket" : "Failed to connect the socket");
		return -1;
	}
	server->max_fd = server->fd = sfd;

	/* set the socket to be non-blocking */
	fd_flags = fcntl(sfd, F_GETFL, 0);
	fcntl(sfd, F_SETFL, fd_flags | O_NONBLOCK);

	/* set up client linked list */
	TAILQ_INIT(&server->clients);

	/* When in client mode, create client node for itself */
	if (!server->listen)
		server_accept(server);

	sigint_handler = signal(SIGINT, handle_sigint);

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
