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
