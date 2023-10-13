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
