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

#include <gtest/gtest.h>
#include "common.h"

extern "C" {
#include <pcie_comm/server.h>
#include <pcie_comm/config.h>

FAKE_VALUE_FUNC(int, bind, int, void *, int);
FAKE_VALUE_FUNC(int, socket, int, int, int);
FAKE_VALUE_FUNC(int, listen, int, int);
FAKE_VALUE_FUNC(int, accept, int, struct sockaddr *, socklen_t *);
}

TEST(TestServer, CreatesServer) {
	struct server_t server;

	RESET_FAKE(bind);
	RESET_FAKE(socket);

	bind_fake.return_val = 0;
	socket_fake.return_val = 10;

	server_create(&server);

	EXPECT_EQ(server.fd, 10);
	EXPECT_EQ(server.max_fd, 10);
}

TEST(TestServer, CreatesServerFailSocket) {
	struct server_t server;

	RESET_FAKE(socket);

	socket_fake.return_val = -1;

	EXPECT_EQ(server_create(&server), -1);
}

TEST(TestServer, CreatesServerFailBind) {
	struct server_t server;

	RESET_FAKE(socket);
	RESET_FAKE(bind);

	socket_fake.return_val = 10;
	bind_fake.return_val = -1;

	EXPECT_EQ(server_create(&server), -1);
}

TEST(TestServer, ServerLoopEmpty) {
	struct server_t server;

	RESET_FAKE(bind);
	RESET_FAKE(socket);

	bind_fake.return_val = 0;
	socket_fake.return_val = 10;

	server_create(&server);
	server_loop(&server);
	server_disconnect_clients(&server, NULL);
}
