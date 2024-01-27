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

#include <sys/socket.h>

#include <gtest/gtest.h>
#include "common.h"

extern "C" {
#include <warppipe/server.h>
#include <warppipe/config.h>

FAKE_VALUE_FUNC(int, bind, int, void *, int);
FAKE_VALUE_FUNC(int, socket, int, int, int);
FAKE_VALUE_FUNC(int, listen, int, int);
FAKE_VALUE_FUNC(int, accept, int, sockaddr *, socklen_t *);
FAKE_VALUE_FUNC(int, getsockname, int, sockaddr *, socklen_t *);
FAKE_VALUE_FUNC(int, getpeername, int, sockaddr *, socklen_t *);
FAKE_VALUE_FUNC(int, getnameinfo, sockaddr *, socklen_t *, char *, socklen_t, char *, socklen_t, int);
FAKE_VALUE_FUNC(int, setsockopt, int, int, int,const void *, socklen_t);
}

TEST(TestServer, CreatesServer) {
	warppipe_server_t server = {};
	server.listen = true;
	server.port = "0";

	RESET_FAKE(bind);
	RESET_FAKE(socket);
	RESET_FAKE(setsockopt);

	bind_fake.return_val = 0;
	socket_fake.return_val = 10;
	setsockopt_fake.return_val = 0;

	warppipe_server_create(&server);

	EXPECT_EQ(server.fd, 10);
	EXPECT_EQ(server.max_fd, 10);
}

TEST(TestServer, CreatesServerFailSocket) {
	warppipe_server_t server = {};
	server.listen = true;
	server.port = "0";

	RESET_FAKE(socket);

	socket_fake.return_val = -1;

	EXPECT_EQ(warppipe_server_create(&server), -1);
}

TEST(TestServer, CreatesServerFailBind) {
	warppipe_server_t server = {};
	server.listen = true;
	server.port = "0";

	RESET_FAKE(socket);
	RESET_FAKE(bind);

	socket_fake.return_val = 10;
	bind_fake.return_val = -1;

	EXPECT_EQ(warppipe_server_create(&server), -1);
}

TEST(TestServer, ServerLoopEmpty) {
	warppipe_server_t server = {};
	server.listen = true;
	server.port = "0";

	RESET_FAKE(bind);
	RESET_FAKE(socket);

	bind_fake.return_val = 0;
	socket_fake.return_val = 10;

	warppipe_server_create(&server);
	warppipe_server_loop(&server);
	warppipe_server_disconnect_clients(&server, NULL);
}
