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

#include <zephyr/ztest.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <warppipe/client.h>
#include <warppipe/server.h>
#include <warppipe/config.h>
#ifdef __cplusplus
}
#endif
#include <sys/socket.h>
#include <unistd.h>

static struct warppipe_server_t pcie_server = {
	.listen = false,
	.quit = false,
	.addr_family = AF_UNSPEC,
	.host = "127.0.0.1",
	.port = SERVER_PORT_NUM,
};

struct read_compl_data_t {
	uint8_t *buf;
	int buf_size;
	int ret;
	bool finished;
};

static int wait_for_completion(struct warppipe_server_t *server, struct read_compl_data_t *read_data)
{
	while (!read_data->finished && !server->quit)
		warppipe_server_loop(server);

	if (server->quit) {
		return -1;
	}
	return 0;
}

static void read_compl(const struct warppipe_completion_status_t completion_status, const void *data, int length, void *private_data)
{
	struct read_compl_data_t *read_data = (struct read_compl_data_t *)private_data;

	read_data->finished = true;

	if (completion_status.error_code) {
		read_data->ret = completion_status.error_code;
		return;
	}

	if (length > read_data->buf_size) {
		length = read_data->buf_size;
	}

	read_data->ret = length;
	memcpy(read_data->buf, data, length);
}

static int read_config_header_field(struct warppipe_server_t *server, struct warppipe_client_t *client, uint64_t addr, int length, uint8_t *buf)
{
	int ret;
	uint64_t aligned_addr = addr & ~0x3;
	uint64_t offset = addr & 0x3;
	uint32_t aligned_buf;

	if (length + offset > 4) {
		return -1;
	}

	struct read_compl_data_t read_data = {
		.buf = (uint8_t *)&aligned_buf,
		.buf_size = 4,
		.ret = 0,
		.finished = false,
	};

	client->private_data = (void *)&read_data;
	ret = warppipe_config0_read(client, aligned_addr, 4, &read_compl);
	if (ret < 0) {
		return ret;
	}

	ret = wait_for_completion(server, &read_data);
	if (ret < 0)
		return ret;

	memcpy(buf, (uint8_t *)&aligned_buf + offset, length);
	return read_data.ret;
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

	int bar_idx = 0;
	int bar_offset = 0x10;
	uint32_t bar = 0xFFFFFFFF;
	uint32_t bar_addr = 0x1000;
	uint32_t old_bar;

	ret = read_config_header_field(server, client, bar_offset, sizeof(uint32_t), (uint8_t *)&old_bar);
	if (ret < 0)
		return -1;

	ret = warppipe_config0_write(client, bar_offset, &bar, sizeof(uint32_t));
	if (ret < 0)
		return -1;

	ret = read_config_header_field(server, client, bar_offset, sizeof(uint32_t), (uint8_t *)&bar);
	if (ret < 0 || bar == 0x0)
		return -1;

	uint32_t size = -(bar & ~0xF);

	ret = warppipe_register_bar(client, bar_addr, size, bar_idx, NULL, NULL);
	if (ret < 0)
		return -1;

	ret = warppipe_config0_write(client, bar_offset, &bar_addr, sizeof(uint32_t));
	if (ret < 0)
		return -1;
	return 0;
}

ZTEST_SUITE(tests, NULL, NULL, NULL, NULL, NULL);

static bool completion_rec;

ZTEST(tests, test_read1)
{
	zassert_equal(warppipe_server_create(&pcie_server), 0, "Failed to connect to server");
	auto client = TAILQ_FIRST(&pcie_server.clients)->client;
	enumerate(&pcie_server, client);
	warppipe_read(client, 0, 0x0, 1, [](const struct warppipe_completion_status_t completion_status, const void *data, int length, void *opaque)
	{
		zassert_equal(completion_status.error_code, 0, "Completion failed");
		zassert_equal(length, 1, "length != 1 (%d)", length);
		const uint8_t *result = (const uint8_t *)data;
		zassert_equal(result[0], 0x10, "result[0] != 0x10");
		completion_rec = true;
	});

	while(!completion_rec)
		warppipe_server_loop(&pcie_server);
	completion_rec = false;

	warppipe_read(client, 0, 0x0, 4, [](const struct warppipe_completion_status_t completion_status, const void *data, int length, void *opaque)
	{
		zassert_equal(completion_status.error_code, 0, "Completion failed");
		zassert_equal(length, 4, "length != 4 (%d)", length);
		const uint32_t *result = (const uint32_t *)data;
		zassert_equal(result[0], 0x13121110, "result[0] != 0x13121110 (%x)", result[0]);
		completion_rec = true;
	});
	while(!completion_rec)
		warppipe_server_loop(&pcie_server);
	completion_rec = false;
}
