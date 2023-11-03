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

#include <pcie_comm/proto.h>

int tlp_data_length(const struct pcie_tlp *pkt)
{
	int data_len = pkt->tlp_length_hi << 8 | pkt->tlp_length_lo;

	return ((data_len - 1) & 0x3FF) + 1;  // 0 means 1024
}

int tlp_total_length(const struct pcie_tlp *pkt)
{
	int hdr_len = 0;
	int data_len = tlp_data_length(pkt);

	switch (pkt->tlp_fmt) {
	case 0:
		data_len = 0;
		[[fallthrough]];
	case 2:
		hdr_len = 3;  // 3 DW header
		break;
	case 1:
		data_len = 0;
		[[fallthrough]];
	case 3:
		hdr_len = 4;  // 4 DW header
		break;
	default:
		return -1;
	}
	return (hdr_len + data_len) * 4;
}

