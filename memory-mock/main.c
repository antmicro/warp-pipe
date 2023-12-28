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

#include <endian.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <getopt.h>
#include <string.h>

#include <warppipe/server.h>
#include <warppipe/client.h>
#include <warppipe/config.h>
#include <warppipe/proto.h>

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

/* XXX: temporary PCIe configuration space structure */
struct pcie_configuration_space_header {
	uint16_t	vendor_id;
	uint16_t	device_id;
	uint8_t		__unused[10];
	uint8_t		header_type;
	uint8_t		__unused_bist;
	uint32_t	bar[6];
};

struct bar_config_t {
	uint8_t config;
	uint32_t size;
	warppipe_read_cb_t read_cb;
	warppipe_write_cb_t write_cb;
	void *data;
};

#define BAR_INACTIVE 0

#define BAR_N 6
#define BAR_CONF_MASK 0xF
#define BAR_SIZE_MASK (~0xF)
#define BAR_ADDR_MASK (~0xF)
#define BAR_DECODE_SIZE(s) (~((s) & ~0xF) + 1)
#define BAR_MASK_SIZE(s) ((s) - 1)
#define BAR_CHECK_SIZE(s) ((((s) - 1) & (s)) == 0)

#define BAR_PREFETCHABLE_OFFSET 3
#define BAR_TYPE_OFFSET 1
#define BAR_MEMORY_SPACE_OFFSET 0

#define BAR_PREFETCHABLE_MASK 0x3
#define BAR_TYPE_MASK 0x1
#define BAR_MEMORY_SPACE_MASK 0x1

#define BAR_PREFETCHABLE (0x1 << BAR_PREFETCHABLE_OFFSET)
#define BAR_TYPE_32B (0x00 << BAR_TYPE_OFFSET)
#define BAR_TYPE_64B (0x10 << BAR_TYPE_OFFSET)
#define BAR_MEMORY_SPACE 0x1

int read_bar(struct bar_config_t bar, uint64_t addr, void *data, int length, void *private_data)
{
	if (addr > bar.size)
		return 1;

	int max_len = bar.size - addr;

	if (length > max_len)
		length = max_len;

	memcpy(data, (uint8_t *)bar.data + addr, length);
	return 0;
}

void write_bar(struct bar_config_t bar, uint64_t addr, const void *data, int length, void *private_data)
{
	if (addr > bar.size)
		return;

	int max_len = bar.size - addr;

	if (length > max_len)
		length = max_len;

	memcpy((uint8_t *)bar.data + addr, data, length);
}

#define DECLARE_BAR_READ_CB(idx) \
	static int __read_bar_cb_##idx(uint64_t addr, void *data, int length, void *private_data)
#define DECLARE_BAR_WRITE_CB(idx) \
	static void __write_bar_cb_##idx(uint64_t addr, const void *data, int length, void *private_data)

#define DEFINE_BAR_READ_CB(idx) \
	int __read_bar_cb_##idx(uint64_t addr, void *data, int length, void *private_data)\
	{ \
		return read_bar(bars_config[(idx)], addr, data, length, private_data); \
	}

#define DEFINE_BAR_WRITE_CB(idx) \
	void __write_bar_cb_##idx(uint64_t addr, const void *data, int length, void *private_data)\
	{ \
		write_bar(bars_config[(idx)], addr, data, length, private_data); \
	}

#define REF_BAR_CB(type, idx) __##type##_bar_cb_##idx

/* XXX: We need to define the functions to use them in the definition below. */
DECLARE_BAR_READ_CB(0);
DECLARE_BAR_READ_CB(1);
DECLARE_BAR_WRITE_CB(1);

#define BAR0_SIZE 128
static int8_t bar0_memory[BAR0_SIZE] = {
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
};

#define BAR1_SIZE 1024
static int8_t bar1_memory[BAR1_SIZE] = {};

static struct bar_config_t bars_config[6] = {
	[0] = {
		.config = BAR_TYPE_32B | BAR_MEMORY_SPACE,
		.size = BAR0_SIZE,
		.read_cb = REF_BAR_CB(read, 0),
		.write_cb = NULL,
		.data = bar0_memory,
	},
	[1] = {
		.config = BAR_TYPE_32B | BAR_MEMORY_SPACE,
		.size = BAR1_SIZE,
		.read_cb = REF_BAR_CB(read, 1),
		.write_cb = REF_BAR_CB(write, 1),
		.data = bar1_memory,
	},
	[2] = { .config = BAR_INACTIVE, },
	[3] = { .config = BAR_INACTIVE, },
	[4] = { .config = BAR_INACTIVE, },
	[5] = { .config = BAR_INACTIVE, },
};

/* Definitions of the BARs callbacks. */
DEFINE_BAR_READ_CB(0)
DEFINE_BAR_READ_CB(1)
DEFINE_BAR_WRITE_CB(1)

static struct pcie_configuration_space_header configuration_space = {
	.vendor_id = 0x1,
	.device_id = 0x2,
	.header_type = 0x0,
};

static struct warppipe_client_t *mock_dev_client;

#define BAR_ADDR(addr) \
	(((addr) >= offsetof(struct pcie_configuration_space_header, bar)) && \
	((addr) < offsetof(struct pcie_configuration_space_header, bar) + sizeof(uint32_t) * 6))
#define BAR_IDX(addr) \
	(((addr) - offsetof(struct pcie_configuration_space_header, bar)) / sizeof(uint32_t))


void handle_bar_write(int bar_idx, uint32_t value, int length)
{
	struct bar_config_t bar = bars_config[bar_idx];
	uint32_t bar_addr = value & ~BAR_MASK_SIZE(bar.size);

	configuration_space.bar[bar_idx] = bar_addr | bar.config;

	/* Register bar in warppipe when first write with actual address is made. */
	if (value != 0xffffffff && mock_dev_client->bar[bar_idx] == 0) {
		syslog(LOG_NOTICE, "Registering bar %d at address %x\n", bar_idx, configuration_space.bar[bar_idx]);
		warppipe_register_bar(mock_dev_client, bar_addr, bar.size, bar_idx, bar.read_cb, bar.write_cb);
	}
}

int config0_read_cb(uint64_t addr, void *data, int length, void *private_data)
{
	// XXX: only 4B aligned accesses are allowed here
	if ((addr & 0x3) || (length != 4)) {
		syslog(LOG_ERR, "Unaligned read from config0 (0x%lx)", addr);
		return 1;
	}

	uint32_t *read_addr = (uint32_t *)(((uint8_t *)&configuration_space) + addr);
	uint32_t *output = (uint32_t *)data;

	*output = htole32(*read_addr);

	return 0;
}

void config0_write_cb(uint64_t addr, const void *data, int length, void *private_data)
{
	// XXX: only 4B aligned writes are allowed here
	if ((addr & 0x3) || (length != 4)) {
		syslog(LOG_ERR, "Unaligned read from config0 (0x%lx)", addr);
		return;
	}

	uint32_t *input = (uint32_t *)data;


	if (BAR_ADDR(addr))
		handle_bar_write(BAR_IDX(addr), le32toh(*input), length);
	else
		syslog(LOG_NOTICE,  "Unhandled config0 write to 0x%lx\n", addr);
}

void server_client_accept(struct warppipe_client_t *client, void *private_data)
{
	warppipe_register_config0_read_cb(client, config0_read_cb);
	warppipe_register_config0_write_cb(client, config0_write_cb);
	mock_dev_client = client;
	for (int bar_idx = 0; bar_idx < 6; bar_idx++)
		configuration_space.bar[bar_idx] = bars_config[bar_idx].config & BAR_CONF_MASK;
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

	/* verify configuration */
	for (int i = 0; i < BAR_N; i++) {
		struct bar_config_t bar = bars_config[i];

		if (!BAR_CHECK_SIZE(bar.size)) {
			syslog(LOG_ERR, "Bar size must be a power of 2 (BAR%d has size %d).", i, bar.size);
			return 1;
		}
	}

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
