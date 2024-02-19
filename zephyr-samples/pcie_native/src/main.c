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

static struct warppipe_server pcie_server = {
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

int main(void)
{
	int ret;

	LOG_INF("Started app");

	if (warppipe_server_create(&pcie_server) == -1) {
		LOG_ERR("Failed to create server!");
		return -1;
	}

	LOG_INF("Started client");

	struct warppipe_client *client = TAILQ_FIRST(&pcie_server.clients)->client;

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
