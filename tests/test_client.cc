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
#include <pcie_comm/client.h>
#include <pcie_comm/proto.h>
#include <pcie_comm/config.h>

FAKE_VALUE_FUNC(int, recv, int, void *, size_t, int);
FAKE_VALUE_FUNC(int, send, int, void *, size_t, int);
}

TEST(TestClient, CreatesClientNode) {
	struct client_t client;

	client_create(&client, 10);
	EXPECT_TRUE(client.active);
	EXPECT_EQ(client.fd, 10);
}

TEST(TestClient, ClientDisconnects) {
	struct client_t client;

	RESET_FAKE(recv);
	recv_fake.return_val = 0;

	client_create(&client, 10);
	client_read(&client);

	EXPECT_FALSE(client.active);
}

TEST(TestClient, ClientAcks) {
	struct client_t client;
	enum pcie_dllp_type type = PCIE_DLLP_ACK;
	uint16_t seqno = 1234;
	
	/* output buffer */
	struct pcie_transport tport_out = {0};

	RESET_FAKE(send);
	send_fake.custom_fake = [&](int sockfd, void *msg, size_t len, int flags) {
		tport_out = *(struct pcie_transport *)msg;

		return sizeof(struct pcie_dllp) + 1;
	};

	client_create(&client, 10);
	client_ack(&client, type, seqno);

	ASSERT_TRUE(client.active);
	ASSERT_EQ(tport_out.t_proto, PCIE_PROTO_DLLP);
	ASSERT_EQ(tport_out.t_dllp.dl_type, type);
	ASSERT_EQ(tport_out.t_dllp.dl_acknak.dl_seqno_hi, seqno >> 8);
	ASSERT_EQ(tport_out.t_dllp.dl_acknak.dl_seqno_lo, seqno & 0xff);
}

TEST(TestClient, ClientAckFails) {
	struct client_t client;
	enum pcie_dllp_type type = PCIE_DLLP_ACK;
	uint16_t seqno = 1234;

	RESET_FAKE(send);
	send_fake.return_val = sizeof(struct pcie_dllp) + 100;

	client_create(&client, 10);
	client_ack(&client, type, seqno);

	ASSERT_FALSE(client.active);
}
