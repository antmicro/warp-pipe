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
