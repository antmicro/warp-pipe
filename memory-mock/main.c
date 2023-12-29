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

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <getopt.h>
#include <string.h>

#include <warppipe/server.h>
#include <warppipe/client.h>
#include <warppipe/config.h>

static struct warppipe_server_t server = {
	.listen = true,
	.addr_family = AF_UNSPEC,
	.host = NULL,
	.port = SERVER_PORT_NUM,
	.quit = false,
};

static void handle_sigint(int signo)
{
	server.quit = true;
	syslog(LOG_NOTICE, "Received SIGINT. " PRJ_NAME_LONG " will shut down.");

	/* disconnect every user */
	warppipe_server_disconnect_clients(&server, NULL);
}

int8_t memory[] = {
0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24,
0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32,
0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,
0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e,
0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55,
0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c,
0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63,
0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a,
0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71,
0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86,
0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d,
0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94,
0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2,
0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0,
0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
};

static int move;

int read_cb_imp(uint64_t addr, void *data, int length, void *private_data)
{
	memcpy(data, memory + move, length);
	move++;
	return 0;
}

void server_client_accept(struct warppipe_client_t *client, void *private_data)
{
	warppipe_register_bar(client, 0x1000, 4 * 1024, 0, read_cb_imp, NULL);
}

int main(int argc, char *argv[])
{
	int c;
	int ret;

	while ((c = getopt(argc, argv, "ca:p:46")) != -1) {
		switch (c) {
		case 'c':
			server.listen = false;
			break;
		case 'a':
			server.host = optarg;
			break;
		case 'p':
			server.port = optarg;
			break;
		case '4':
		case '6':
			server.addr_family = (c == '4' ? AF_INET : AF_INET6);
			break;
		default:  /* '?' */
			fprintf(stderr,
				"Usage: %s [-4|-6] [-c] [-a <addr>] [-p <port>]\n"
				"\n"
				"Options:\n"
				" -4|-6      force IPv4/IPv6 (default: system preference)\n"
				" -c         client mode (default: server mode),\n"
				" -a <addr>  server address (default: wildcard address for server, loopback address for client),\n"
				" -p <port>  server port (default: " SERVER_PORT_NUM "),\n"
				"\n", *argv);
			return 1;
		}
	}

	/* syslog initialization */
	setlogmask(LOG_UPTO(LOG_DEBUG));
	openlog(PRJ_NAME_SHORT, LOG_CONS | LOG_NDELAY, LOG_USER);

	/* signal initialization */
	signal(SIGINT, handle_sigint);

	/* server initialization */
	syslog(LOG_NOTICE, "Starting " PRJ_NAME_LONG "...");

	ret = warppipe_server_create(&server);
	warppipe_server_register_accept_cb(&server, server_client_accept);
	if (!ret)
		while (!server.quit)
			warppipe_server_loop(&server);
	else
		syslog(LOG_NOTICE, "Failed to set up server for " PRJ_NAME_LONG ".");


	syslog(LOG_NOTICE, "Shutting down " PRJ_NAME_LONG ".");
	closelog();

	return 0;
}
