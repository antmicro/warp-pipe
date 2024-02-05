/*
 * Copyright 2024 Antmicro <www.antmicro.com>
 * Copyright 2024 Meta
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
#include <unistd.h>
#include <stdlib.h>

#include <warppipe/client.h>
#include <warppipe/server.h>
#include <warppipe/config.h>
#include <zephyr/ztest.h>

struct read_compl_data {
	uint8_t *buf;
	int buf_size;
	int ret;
	bool finished;
};

static void read_compl(const warppipe_completion_status completion_status, const void *data, int length, void *private_data)
{
	read_compl_data *read_data = (read_compl_data *)private_data;

	if (completion_status.error_code) {
		read_data->ret = completion_status.error_code;
		return;
	}

	zassert_equal(length, read_data->buf_size, "Unexpected size of completion data: got: %d, expected: %d", length, read_data->buf_size);

	read_data->ret = length;
	memcpy(read_data->buf, data, length);

	read_data->finished = true;
}

static int wait_for_completion(warppipe_server *server, read_compl_data *read_data)
{
	while (!read_data->finished && !server->quit)
		warppipe_server_loop(server);

	if (server->quit) {
		return -1;
	}
	return 0;
}

static int read_config_header_field(warppipe_server *server, warppipe_client *client, uint64_t addr, int length, uint8_t *buf)
{
	uint64_t aligned_addr = addr & ~0x3;
	uint64_t offset = addr & 0x3;
	uint32_t aligned_buf;

	zassert_true(length + offset <= 4, "Invalid config read");

	read_compl_data read_data = {
		.buf = (uint8_t *)&aligned_buf,
		.buf_size = 4,
		.ret = 0,
		.finished = false,
	};

	client->private_data = (void *)&read_data;
	zassert_equal(warppipe_config0_read(client, aligned_addr, 4, &read_compl), 0, "Unexpected config data read");

	zassert_equal(wait_for_completion(server, &read_data), 0, "Failed to wait for response");
	zassert_equal(read_data.ret, 4, "Invalid read length");

	memcpy(buf, (uint8_t *)&aligned_buf + offset, length);
	return length;
}

static int enumerate(warppipe_server *server, warppipe_client *client)
{
	uint16_t vendor_id;
	uint8_t header_type;

	/* Read vendor id. If 0xFFFF, then it is not enabled. */
	zassert_equal(read_config_header_field(server, client, 0x0, 2, (uint8_t *)&vendor_id), 2, "Incomplete config read");
	zassert_equal(vendor_id, 0x1, "Invalid vendor_id read");

	zassert_equal(read_config_header_field(server, client, 0xE, 1, &header_type), 1, "Incomplete config read");
	zassert_equal(header_type, 0x0, "Invalid header_type");

	uint32_t bar, old_bar;

	/* Check and register BARs. */
	for (int bar_offset = 0x10; bar_offset < 0x28; bar_offset += 4) {
		zassert_equal(read_config_header_field(server, client, bar_offset, sizeof(uint32_t), (uint8_t *)&old_bar), sizeof(uint32_t), "Failed to read config");

		bar = 0xFFFFFFFF;
		zassert_equal(warppipe_config0_write(client, bar_offset, &bar, sizeof(uint32_t)), 0, "Failed to write config");

		zassert_equal(read_config_header_field(server, client, bar_offset, sizeof(uint32_t), (uint8_t *)&bar), sizeof(uint32_t), "Failed to get BAR");
		// if bar is equal to zero, it doesn't exist
		if (bar == 0x0)
			continue;

		uint32_t size = -(bar & ~0xF);
		uint32_t bar_idx = (bar_offset - 0x10) / sizeof(uint32_t);
		uint32_t bar_addr = bar_offset * 0x10000;

		zassert_equal(warppipe_register_bar(client, bar_addr, size, bar_idx, NULL, NULL), 0, "Failed to register BAR");

		zassert_equal(warppipe_config0_write(client, bar_offset, &bar_addr, sizeof(uint32_t)), 0, "Failed to write BAR");
	}

	return 0;
}

static void *fixture_setup(void)
{
	warppipe_server *fixture = (warppipe_server *)malloc(sizeof(warppipe_server));
	zassume_not_null(fixture, NULL);
	fixture->listen = false;
	fixture->quit = false;
	fixture->addr_family = AF_UNSPEC;
	fixture->host = "127.0.0.1";
	fixture->port = SERVER_PORT_NUM;

	return fixture;
}

static void fixture_before(void *fixture)
{
	warppipe_server *pcie_server = (warppipe_server *)fixture;
	pcie_server->quit = false;

	zassert_equal(warppipe_server_create(pcie_server), 0, "Failed to connect to server");

	auto client = TAILQ_FIRST(&pcie_server->clients)->client;
	zassert_equal(enumerate(pcie_server, client), 0, "Failed to enumerate pcie device");
}

static void fixture_after(void *fixture)
{
	warppipe_server *pcie_server = (warppipe_server *)fixture;
	pcie_server->quit = true;
	warppipe_server_loop(pcie_server);
}

static void fixture_teardown(void *fixture)
{
	warppipe_server *pcie_server = (warppipe_server *)fixture;
	pcie_server->quit = true;
	warppipe_server_loop(pcie_server);
	free(pcie_server);
}

ZTEST_SUITE(tests, NULL, fixture_setup, fixture_before, fixture_after, fixture_teardown);

static int read_data(warppipe_server *pcie_server, warppipe_client *client, int bar, uint64_t addr, int length, read_compl_data *read_data)
{
	int ret;

	client->private_data = (void *)read_data;
	ret = warppipe_read(client, bar, addr, length, &read_compl);
	if (ret < 0) {
		return ret;
	}

	wait_for_completion(pcie_server, read_data);

	return read_data->ret;
}


ZTEST_F(tests, test_write)
{
	warppipe_server *pcie_server = (warppipe_server *)fixture;
	auto client = TAILQ_FIRST(&pcie_server->clients)->client;
	uint8_t buf[16];

	memcpy(buf, "hello", 5);
	zassert_equal(warppipe_write(client, 1, 0, buf, strlen("hello")), 0, "Failed to write data to PCIe BAR");
}

ZTEST_F(tests, test_read)
{
	warppipe_server *pcie_server = (warppipe_server *)fixture;
	auto client = TAILQ_FIRST(&pcie_server->clients)->client;

	uint8_t read_buf[16];
	read_compl_data read = {
		.buf = read_buf,
		.buf_size = 16,
		.ret = -1,
		.finished = false
	};
	zassert_equal(read_data(pcie_server, client, 0, 0, 16, &read), 16, "Failed to read data to PCIe BAR");
	for (int i = 0; i < 16; i++) {
		zassert_equal(read_buf[i], 0x10 + i, "Unexpected data read");
	}
}

ZTEST_F(tests, test_unaligned_read)
{
	warppipe_server *pcie_server = (warppipe_server *)fixture;
	auto client = TAILQ_FIRST(&pcie_server->clients)->client;

	uint8_t read_buf[1];
	read_compl_data read = {
		.buf = read_buf,
		.buf_size = 1,
		.ret = -1,
		.finished = false
	};
	zassert_equal(read_data(pcie_server, client, 0, 1, 1, &read), 1, "Failed to read data to PCIe BAR");
	zassert_equal(read_buf[0], 0x11, "Unexpected data read: got: %x, expected: %x", read_buf[0], 0x11);
}

ZTEST_F(tests, test_unaligned_write)
{
	warppipe_server *pcie_server = (warppipe_server *)fixture;
	auto client = TAILQ_FIRST(&pcie_server->clients)->client;
	uint8_t buf[5];

	memcpy(buf, "hello", 5);
	zassert_equal(warppipe_write(client, 1, 1, buf, 5), 0, "Failed to write data to PCIe BAR");

	uint8_t read_buf[5];
	read_compl_data read = {
		.buf = read_buf,
		.buf_size = 5,
		.ret = -1,
		.finished = false
	};
	zassert_equal(read_data(pcie_server, client, 1, 1, 5, &read), 5, "Failed to read data to PCIe BAR");
	zassert_equal(read_buf[0], 'h', "Unexpected data read: got: %c, expected: %s", read_buf[0], "h");
	zassert_equal(read_buf[1], 'e', "Unexpected data read: got: %c, expected: %s", read_buf[1], "e");
	zassert_equal(read_buf[2], 'l', "Unexpected data read: got: %c, expected: %s", read_buf[2], "l");
	zassert_equal(read_buf[3], 'l', "Unexpected data read: got: %c, expected: %s", read_buf[3], "l");
	zassert_equal(read_buf[4], 'o', "Unexpected data read: got: %c, expected: %s", read_buf[4], "o");
}
