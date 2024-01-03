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

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <sys/socket.h>
#include <sys/queue.h>

#include <warppipe/client.h>
#include <warppipe/server.h>
#include <warppipe/config.h>

#include "../../common/common.h"


LOG_MODULE_REGISTER(pcie_native, LOG_LEVEL_DBG);

static struct warppipe_server_t pcie_server = {
	.listen = false,
	.addr_family = AF_UNSPEC,
	.host = CONFIG_PCIE_PIPE_SERVER,
	.port = SERVER_PORT_NUM,
	.quit = false,
};

static void print_read_data(uint8_t *buffer, int length)
{
	LOG_PRINTK("Read data (len: %d):", length);
	for (int i = 0; i < length; i++)
		LOG_PRINTK(" %02x", buffer[i]);
	LOG_PRINTK("\n");
}

static int enumerate(struct warppipe_server_t *server, struct warppipe_client_t *client)
{
	int ret;
	uint16_t vendor_id;
	uint8_t header_type;

	/* TODO: Disable proper bits in command register. */

	/* Read vendor id. If 0xFFFF, then it is not enabled. */
	ret = read_config_header_field(server, client, 0x0, 2, (uint8_t *)&vendor_id);
	if (ret < 0 || vendor_id == 0xFFFF)
		return -1;

	/* Check device type (only 0 is supported). */
	ret = read_config_header_field(server, client, 0xE, 1, &header_type);
	if (ret < 0 || header_type != 0x0)
		return -1;

	/* TODO: Print some useful info data from header. */

	uint32_t bar, old_bar;

	/* Check and register BARs. */
	for (int bar_offset = 0x10; bar_offset < 0x28; bar_offset += 4) {
		ret = read_config_header_field(server, client, bar_offset, sizeof(uint32_t), (uint8_t *)&old_bar);
		if (ret < 0)
			continue;

		bar = 0xFFFFFFFF;
		ret = warppipe_config0_write(client, bar_offset, &bar, sizeof(uint32_t));
		if (ret < 0)
			continue;

		ret = read_config_header_field(server, client, bar_offset, sizeof(uint32_t), (uint8_t *)&bar);
		if (ret < 0 || bar == 0x0)
			continue;

		uint32_t size = -(bar & ~0xF);
		uint32_t bar_idx = (bar_offset - 0x10) / sizeof(uint32_t);
		uint32_t bar_addr = bar_offset * 0x10000;

		LOG_INF("Registering bar %d at 0x%x (size: %d)", bar_idx, bar_addr, size);

		ret = warppipe_register_bar(client, bar_addr, size, bar_idx, NULL, NULL);
		if (ret < 0)
			continue;

		ret = warppipe_config0_write(client, bar_offset, &bar_addr, sizeof(uint32_t));
		if (ret < 0)
			continue;
	}

	/* TODO: Enable memory in command register. */

	return 0;
}

int main(void)
{
	int ret;

	LOG_INF("Started app");

	if (warppipe_server_create(&pcie_server) == -1) {
		LOG_ERR("Failed to create server!");
		return -1;
	}

	LOG_INF("Started client");

	struct warppipe_client_t *client = TAILQ_FIRST(&pcie_server.clients)->client;

	ret = enumerate(&pcie_server, client);
	if (ret < 0) {
		LOG_ERR("Failed to enumerate: %d\n", ret);
		return -1;
	}

	uint8_t buf[16];

	ret = read_data(&pcie_server, client, 0, 0, 16, buf);
	assert(ret == 16);
	print_read_data(buf, ret);

	ret = read_data(&pcie_server, client, 1, 0, 16, buf);
	assert(ret == 16);
	print_read_data(buf, ret);

	memcpy(buf, "hello", 5);
	ret = warppipe_write(client, 1, 0, buf, strlen("hello"));
	assert(ret == 0);

	ret = read_data(&pcie_server, client, 1, 0, 16, buf);
	assert(ret == 16);
	print_read_data(buf, ret);

	assert(strncmp(buf, "hello", 5) == 0);

	LOG_INF("The next read should fail");
	ret = read_data(&pcie_server, client, 2, 0, 16, buf);
	assert(ret == -1);

	LOG_INF("All checks has succeded.");
	return 0;
}
