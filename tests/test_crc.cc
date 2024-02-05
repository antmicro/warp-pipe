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

#include <warppipe/proto.h>
#include <warppipe/crc.h>

TEST(TestCrc, Crc32Correct) {
	warppipe_pcie_transport tport = {
		.t_proto = PCIE_PROTO_TLP,
		.t_tlp = {
			.dl_seqno_hi = 0,
			.dl_seqno_lo = 0,
		},
	};
	tport.t_tlp.dl_tlp.tlp_fmt = PCIE_TLP_MRD32 >> 5;
	tport.t_tlp.dl_tlp.tlp_type = PCIE_TLP_MRD32 & 0x1f;
	pcie_lcrc32(&tport.t_tlp);
	bool valid = pcie_lcrc32_valid(&tport.t_tlp);

	ASSERT_TRUE(valid);

	uint32_t actual_crc = 0;
	for (int i = 0; i < 4; i++) {
		/* there is no data, so that's where the CRC goes */
		actual_crc += tport.t_tlp.dl_tlp.tlp_req.r_data32[i] << 8 * i;
	}
	ASSERT_EQ(actual_crc, 0xd1bb79c7);
}

TEST(TestCrc, Crc16Correct) {
	warppipe_pcie_transport tport = {
		.t_proto = PCIE_PROTO_DLLP,
		.t_dllp = {
			.dl_acknak = {
				.dl_nak = 0,
				.dl_seqno_hi = 0,
				.dl_seqno_lo = 0,
			},
		},
	};
	pcie_crc16(&tport.t_dllp);
	bool valid = pcie_crc16_valid(&tport.t_dllp);

	ASSERT_TRUE(valid);

	uint16_t actual_crc = tport.t_dllp.dl_crc16[0] | tport.t_dllp.dl_crc16[1] << 8;
	ASSERT_EQ(actual_crc, 0x62b3);
}
