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
#include <pcie_comm/crc.h>

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
		memcpy(&tport_out, msg, len);

		return len;
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

TEST(TestClient, ClientAckFailsErrno) {
	struct client_t client;
	enum pcie_dllp_type type = PCIE_DLLP_ACK;
	uint16_t seqno = 1234;

	RESET_FAKE(send);
	send_fake.return_val = -1;

	client_create(&client, 10);
	client_ack(&client, type, seqno);

	ASSERT_FALSE(client.active);
}

TEST(TestClient, ClientReadFails) {
	struct client_t client;

	RESET_FAKE(recv);
	recv_fake.return_val = sizeof(struct pcie_dllp) + 100;

	client_create(&client, 10);
	client_read(&client);

	ASSERT_FALSE(client.active);
}

TEST(TestClient, ClientReadDLLP) {
	struct client_t client;

	struct pcie_transport tport_out = {0};
	int total_sent = 0;
	tport_out.t_proto = PCIE_PROTO_DLLP;
	tport_out.t_dllp.dl_type = PCIE_DLLP_ACK;
	pcie_lcrc32(&tport_out.t_tlp);

	RESET_FAKE(recv);
	recv_fake.custom_fake = [&](int sockfd, void *msg, size_t len, int flags) {
		memcpy(msg, &tport_out, len);
		total_sent += len;
		return len;
	};

	client_create(&client, 10);
	client_read(&client);

	ASSERT_TRUE(client.active);
}

TEST(TestClient, ClientReadTLPEmpty) {
	struct client_t client;
	struct pcie_transport tport_out = {0};
	int total_sent = 0;
	tport_out.t_proto = PCIE_PROTO_TLP;
	pcie_lcrc32(&tport_out.t_tlp);

	RESET_FAKE(recv);
	recv_fake.custom_fake = [&](int sockfd, void *msg, size_t len, int flags) {
		memcpy(msg, &tport_out, len);
		total_sent += len;
		return len;
	};

	client_create(&client, 10);
	client_read(&client);

	ASSERT_FALSE(client.active);
}

TEST(TestClient, ClientReadTLPTotalLengthNegative) {
	struct client_t client;
	struct pcie_transport tport_out = {0};
	int total_sent = 0;
	tport_out.t_proto = PCIE_PROTO_TLP;
	tport_out.t_tlp.dl_tlp.tlp_fmt = 4; // undefined format
	pcie_lcrc32(&tport_out.t_tlp);

	RESET_FAKE(recv);
	recv_fake.custom_fake = [&](int sockfd, void *msg, size_t len, int flags) {
		memcpy(msg, &tport_out, len);
		total_sent += len;
		return len;
	};

	client_create(&client, 10);
	client_read(&client);

	ASSERT_FALSE(client.active);
}

TEST(TestClient, ClientReadTLPTotalRecvFail) {
	struct client_t client;
	struct pcie_transport tport_out = {0};
	int total_sent = 0;
	tport_out.t_proto = PCIE_PROTO_TLP;
	tport_out.t_tlp.dl_tlp.tlp_fmt = 0;
	pcie_lcrc32(&tport_out.t_tlp);

	std::function<int(int sockfd, void *msg, size_t len, int flags)> custom_fakes [2] = {
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, &tport_out, len);
			total_sent += len;
			return len;
		},
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, ((uint8_t*)(&tport_out)) + total_sent, len);
			total_sent += len;
			return len;
		}
	};

	RESET_FAKE(recv);
	SET_CUSTOM_FAKE_SEQ(recv, custom_fakes, 2);

	client_create(&client, 10);
	client_read(&client);

	ASSERT_FALSE(client.active);
}

TEST(TestClient, ClientReadTLPIOWR) {
	struct client_t client;
	struct pcie_transport tport_out = {0};
	int total_sent = 0;
	tport_out.t_proto = PCIE_PROTO_TLP;
	tport_out.t_tlp.dl_tlp.tlp_fmt = 2;
	tport_out.t_tlp.dl_tlp.tlp_type = 2;
	pcie_lcrc32(&tport_out.t_tlp);

	std::function<int(int sockfd, void *msg, size_t len, int flags)> custom_fakes [2] = {
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, &tport_out, len);
			total_sent += len;
			return len;
		},
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, ((uint8_t*)(&tport_out)) + total_sent, len);
			total_sent += len;
			return len;
		}
	};

	RESET_FAKE(recv);
	RESET_FAKE(send);
	send_fake.return_val = sizeof(struct pcie_dllp) + 1;
	SET_CUSTOM_FAKE_SEQ(recv, custom_fakes, 2);

	client_create(&client, 10);
	client_read(&client);

	ASSERT_TRUE(client.active);
}

TEST(TestClient, ClientReadTLPCPL) {
	struct client_t client;
	struct pcie_transport tport_out = {0};
	int total_sent = 0;
	tport_out.t_proto = PCIE_PROTO_TLP;
	tport_out.t_tlp.dl_tlp.tlp_fmt = 0;
	tport_out.t_tlp.dl_tlp.tlp_type = 10;
	pcie_lcrc32(&tport_out.t_tlp);

	std::function<int(int sockfd, void *msg, size_t len, int flags)> custom_fakes [2] = {
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, &tport_out, len);
			total_sent += len;
			return len;
		},
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, ((uint8_t*)(&tport_out)) + total_sent, len);
			total_sent += len;
			return len;
		}
	};

	RESET_FAKE(recv);
	RESET_FAKE(send);
	send_fake.return_val = sizeof(struct pcie_dllp) + 1;
	SET_CUSTOM_FAKE_SEQ(recv, custom_fakes, 2);

	client_create(&client, 10);
	client_read(&client);

	ASSERT_TRUE(client.active);
}

TEST(TestClient, ClientReadTLPMRDLK32) {
	struct client_t client;
	struct pcie_transport tport_out = {0};
	int total_sent = 0;
	tport_out.t_proto = PCIE_PROTO_TLP;
	tport_out.t_tlp.dl_tlp.tlp_fmt = 0;
	tport_out.t_tlp.dl_tlp.tlp_type = 1;
	pcie_lcrc32(&tport_out.t_tlp);

	std::function<int(int sockfd, void *msg, size_t len, int flags)> custom_fakes [2] = {
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, &tport_out, len);
			total_sent += len;
			return len;
		},
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, ((uint8_t*)(&tport_out)) + total_sent, len);
			total_sent += len;
			return len;
		}
	};

	RESET_FAKE(recv);
	RESET_FAKE(send);
	send_fake.return_val = sizeof(struct pcie_dllp) + 1;
	SET_CUSTOM_FAKE_SEQ(recv, custom_fakes, 2);

	client_create(&client, 10);
	client_read(&client);

	ASSERT_TRUE(client.active);
}

TEST(TestClient, ClientReadTLPCPLFail) {
	struct client_t client;
	struct pcie_transport tport_out = {0};
	int total_sent = 0;
	tport_out.t_proto = PCIE_PROTO_TLP;
	tport_out.t_tlp.dl_tlp.tlp_fmt = 0;
	tport_out.t_tlp.dl_tlp.tlp_type = 2;
	pcie_lcrc32(&tport_out.t_tlp);

	std::function<int(int sockfd, void *msg, size_t len, int flags)> custom_fakes [2] = {
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, &tport_out, len);
			total_sent += len;
			return len;
		},
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, ((uint8_t*)(&tport_out)) + total_sent, len);
			total_sent += len;
			return len;
		}
	};

	int custom_send_fake_return_vals[3] = {sizeof(struct pcie_dllp) + 1, 15, -1};

	RESET_FAKE(recv);
	RESET_FAKE(send);
	SET_RETURN_SEQ(send, custom_send_fake_return_vals, 3);
	SET_CUSTOM_FAKE_SEQ(recv, custom_fakes, 2);

	client_create(&client, 10);
	pcie_register_read_cb(&client, [](uint64_t addr, void *data, int length) {});
	client_read(&client);

	EXPECT_EQ(total_sent, sizeof(struct pcie_transport));

	ASSERT_FALSE(client.active);
}

TEST(TestClient, ClientReadTLPCPLCRCFail) {
	struct client_t client;
	struct pcie_transport tport_out = {0};
	struct pcie_transport tport_response = {0};
	int total_sent = 0;
	tport_out.t_proto = PCIE_PROTO_TLP;
	tport_out.t_tlp.dl_tlp.tlp_fmt = 0;
	tport_out.t_tlp.dl_tlp.tlp_type = 2;

	std::function<int(int sockfd, void *msg, size_t len, int flags)> custom_fakes_recv [2] = {
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, &tport_out, len);
			total_sent += len;
			return len;
		},
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, ((uint8_t*)(&tport_out)) + total_sent, len);
			total_sent += len;
			return len;
		}
	};
	std::function<int(int sockfd, void *msg, size_t len, int flags)> custom_fakes_send [1] = {
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(&tport_response, msg, len);
			return len;
		}
	};

	RESET_FAKE(recv);
	RESET_FAKE(send);
	SET_CUSTOM_FAKE_SEQ(send, custom_fakes_send, 1);
	SET_CUSTOM_FAKE_SEQ(recv, custom_fakes_recv, 2);

	client_create(&client, 10);
	client_read(&client);

	EXPECT_EQ(tport_response.t_proto, PCIE_PROTO_DLLP);
	ASSERT_TRUE(pcie_crc16_valid(&tport_response.t_dllp));
	ASSERT_EQ(tport_response.t_dllp.dl_acknak.dl_nak, PCIE_DLLP_NAK);
}

TEST(TestClient, ClientReadCreditDLL) {
	struct client_t client;
	struct pcie_transport tport_out = {0};
	int total_sent = 0;
	tport_out.t_proto = PCIE_PROTO_DLLP;
	tport_out.t_dllp.dl_fc.fc_type = 1;
	tport_out.t_dllp.dl_fc.fc_rsvd1 = 0;
	pcie_crc16(&tport_out.t_dllp);

	std::function<int(int sockfd, void *msg, size_t len, int flags)> custom_fakes [2] = {
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, &tport_out, len);
			total_sent += len;
			return len;
		},
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, ((uint8_t*)(&tport_out)) + total_sent, len);
			total_sent += len;
			return len;
		}
	};

	int custom_send_fake_return_vals[3] = {sizeof(struct pcie_dllp) + 1, 15, 1};

	RESET_FAKE(recv);
	RESET_FAKE(send);
	SET_RETURN_SEQ(send, custom_send_fake_return_vals, 3);
	SET_CUSTOM_FAKE_SEQ(recv, custom_fakes, 2);

	client_create(&client, 10);
	client_read(&client);

	ASSERT_TRUE(client.active);
}

TEST(TestClient, ClientReadUnknownDLL) {
	struct client_t client;
	struct pcie_transport tport_out = {0};
	int total_sent = 0;
	tport_out.t_proto = PCIE_PROTO_DLLP;
	tport_out.t_dllp.dl_fc.fc_type = 0;
	tport_out.t_dllp.dl_fc.fc_rsvd1 = 0;
	pcie_crc16(&tport_out.t_dllp);

	std::function<int(int sockfd, void *msg, size_t len, int flags)> custom_fakes [2] = {
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, &tport_out, len);
			total_sent += len;
			return len;
		},
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, ((uint8_t*)(&tport_out)) + total_sent, len);
			total_sent += len;
			return len;
		}
	};

	int custom_send_fake_return_vals[3] = {sizeof(struct pcie_dllp) + 1, 15, 1};

	RESET_FAKE(recv);
	RESET_FAKE(send);
	SET_RETURN_SEQ(send, custom_send_fake_return_vals, 3);
	SET_CUSTOM_FAKE_SEQ(recv, custom_fakes, 2);

	client_create(&client, 10);
	client_read(&client);

	ASSERT_TRUE(client.active);
}

TEST(TestClient, ClientReadUnknown) {
	struct client_t client;
	struct pcie_transport tport_out = {0};
	int total_sent = 0;
	tport_out.t_proto = 111;

	RESET_FAKE(recv);
	recv_fake.custom_fake = [&](int sockfd, void *msg, size_t len, int flags) {
		memcpy(msg, &tport_out, len);
		total_sent += len;
		return len;
	};

	client_create(&client, 10);
	client_read(&client);

	ASSERT_FALSE(client.active);
}

TEST(TestClient, ClientPcieRead) {
	struct client_t client;
	struct pcie_transport tport_request = {0};
	int tport_request_recv = 0;
	struct pcie_transport *tport_request2 = (struct pcie_transport *)malloc(sizeof(struct pcie_dltlp) + 40 * sizeof(uint8_t));
	int tport_response_send = 0;
	int total_sent2 = 0;
	int total_sent_rec = 0;

	std::function<int(int sockfd, void *msg, size_t len, int flags)> custom_fakes_send [4] = {
		// request
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(&tport_request, msg, len);
			tport_request_recv += len;
			return len;
		},
		// ACK/NACK
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			return len;
		},
		// response
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(tport_request2, msg, len);
			total_sent_rec += len;
			return len;
		},
		// ACK/NACK
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			return len;
		},
	};

	std::function<int(int sockfd, void *msg, size_t len, int flags)> custom_fakes_recv [4] = {
		// request
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, &tport_request, len);
			tport_response_send += len;
			return len;
		},
		// request part 2
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, ((uint8_t*)(&tport_request)) + tport_response_send, len);
			tport_response_send += len;
			return len;
		},
		// completion
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, tport_request2, len);
			total_sent2 += len;
			return len;
		},
		// completion part 2
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, ((uint8_t*)(tport_request2)) + total_sent2, len);
			total_sent2 += len;
			return len;
		}
	};

	RESET_FAKE(recv);
	RESET_FAKE(send);
	SET_CUSTOM_FAKE_SEQ(send, custom_fakes_send, 4);
	SET_CUSTOM_FAKE_SEQ(recv, custom_fakes_recv, 4);

	client_create(&client, 10);
	pcie_register_read_cb(&client, [](uint64_t addr, void *data, int length)
	{
		uint8_t *result = (uint8_t *)data;
		memset(result, 0, sizeof(uint8_t) * length * 4);
		for (int i = 0; i < length * 4; i++) {
			result[i] = (uint8_t)i;
		}
		return;
	});

	int rc = pcie_read(&client, 0x1234, 10, [](const struct completion_status_t completion_status, const void *data, int length)
	{
		uint8_t *result = (uint8_t *)data;
		ASSERT_EQ(completion_status.error_code, 0);
		ASSERT_EQ(length, 10);

		for (int i = 0; i < length * 4; i++)
			ASSERT_EQ((uint8_t)result[i], (uint8_t)i);

	});

	ASSERT_EQ(rc, 0);
	ASSERT_EQ(tport_request.t_proto, PCIE_PROTO_TLP);
	ASSERT_EQ(tport_request.t_tlp.dl_tlp.tlp_fmt, PCIE_TLP_MRD32 >> 5);
	ASSERT_EQ(tport_request.t_tlp.dl_tlp.tlp_type, PCIE_TLP_MRD32 & 0x1F);
	ASSERT_EQ(pcie_lcrc32_valid(&tport_request.t_tlp), true);
	ASSERT_EQ(send_fake.call_count, 1);
	ASSERT_EQ(recv_fake.call_count, 0);

	client_read(&client); //request

	ASSERT_EQ(send_fake.call_count, 3);
	ASSERT_EQ(recv_fake.call_count, 2);
	ASSERT_EQ(tport_request2->t_proto, PCIE_PROTO_TLP);
	ASSERT_EQ(tport_request2->t_tlp.dl_tlp.tlp_fmt, PCIE_TLP_CPLD >> 5);
	ASSERT_EQ(tport_request2->t_tlp.dl_tlp.tlp_type, PCIE_TLP_CPLD & 0x1F);

	for (int i = 0; i < 10 * 4; i++)
		ASSERT_EQ(tport_request2->t_tlp.dl_tlp.tlp_cpl.c_data[i], (uint8_t)i);

	ASSERT_TRUE(pcie_lcrc32_valid(&tport_request2->t_tlp));

	client_read(&client); //response

	ASSERT_TRUE(client.active);

	ASSERT_EQ(send_fake.call_count, 4);
	ASSERT_EQ(recv_fake.call_count, 4);
}

TEST(TestClient, ClientPcieWrite) {
	struct client_t client;
	struct pcie_transport tport_request = {0};
	int tport_response_send = 0;
	uint8_t write_data[40];

	for (int i = 0; i < 40; i++)
		write_data[i] = i;

	std::function<int(int sockfd, void *msg, size_t len, int flags)> custom_fakes_send [2] = {
		// request
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(&tport_request, msg, len);
			return len;
		},
		// ACK/NACK
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			return len;
		},
	};

	std::function<int(int sockfd, void *msg, size_t len, int flags)> custom_fakes_recv [2] = {
		// request
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, &tport_request, len);
			tport_response_send += len;
			return len;
		},
		// request part 2
		[&](int sockfd, void *msg, size_t len, int flags) -> int {
			memcpy(msg, ((uint8_t*)(&tport_request)) + tport_response_send, len);
			tport_response_send += len;
			return len;
		},
	};

	RESET_FAKE(recv);
	RESET_FAKE(send);
	SET_CUSTOM_FAKE_SEQ(send, custom_fakes_send, 2);
	SET_CUSTOM_FAKE_SEQ(recv, custom_fakes_recv, 2);

	client_create(&client, 10);
	pcie_register_write_cb(&client, [](uint64_t addr, const void *data, int length)
	{
		for (int i = 0; i < length * 4; i++) {
			ASSERT_EQ(*(((uint8_t *)data) + i), i);
		}
		return;
	});

	int rc = pcie_write(&client, 0x1234, write_data, 10);

	ASSERT_EQ(rc, 0);
	ASSERT_EQ(tport_request.t_proto, PCIE_PROTO_TLP);
	ASSERT_EQ(tport_request.t_tlp.dl_tlp.tlp_fmt, PCIE_TLP_MWR32 >> 5);
	ASSERT_EQ(tport_request.t_tlp.dl_tlp.tlp_type, PCIE_TLP_MWR32 & 0x1F);
	ASSERT_EQ(pcie_lcrc32_valid(&tport_request.t_tlp), true);
	ASSERT_EQ(send_fake.call_count, 1);
	ASSERT_EQ(recv_fake.call_count, 0);

	client_read(&client); //request

	ASSERT_TRUE(client.active);

	ASSERT_EQ(send_fake.call_count, 2);
	ASSERT_EQ(recv_fake.call_count, 2);
}
