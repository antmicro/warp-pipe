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

ZTEST_SUITE(tests, NULL, NULL, NULL, NULL, NULL);

static bool completion_rec;

ZTEST(tests, test_read1)
{
	zassert_equal(warppipe_server_create(&pcie_server), 0, "Failed to connect to server");
	auto client = TAILQ_FIRST(&pcie_server.clients)->client;
	warppipe_register_bar(client, 0x1000, 4 * 1024, 0, NULL, NULL);
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
