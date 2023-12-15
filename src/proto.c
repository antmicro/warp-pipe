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

#include <warppipe/proto.h>

int tlp_data_length(const struct pcie_tlp *pkt)
{
	int data_len = pkt->tlp_length_hi << 8 | pkt->tlp_length_lo;

	return ((data_len - 1) & 0x3FF) + 1;  // 0 means 1024
}

int tlp_data_length_bytes(const struct pcie_tlp *pkt)
{
	int data_len_bytes = tlp_data_length(pkt) * 4;

	int last_be = data_len_bytes == 4 ? pkt->tlp_req.r_first_be : pkt->tlp_req.r_last_be;

	/* [PCIe 5.0 spec about bit order]
	 * Contiguous Byte Enables examples:
	 *   First DW BE: 1100b, Last DW BE: 0011b
	 *   First DW BE: 1000b, Last DW BE: 0111b
	 */
	if (pkt->tlp_req.r_first_be & 1)
		data_len_bytes -= 0;
	else if (pkt->tlp_req.r_first_be & 2)
		data_len_bytes -= 1;
	else if (pkt->tlp_req.r_first_be & 4)
		data_len_bytes -= 2;
	else if (pkt->tlp_req.r_first_be & 8)
		data_len_bytes -= 3;
	else
		return data_len_bytes == 4 ? 0 : -1;  // zero-length accesses

	if (last_be & 8)
		return data_len_bytes;
	if (last_be & 4)
		return data_len_bytes - 1;
	if (last_be & 2)
		return data_len_bytes - 2;
	if (last_be & 1)
		return data_len_bytes - 3;

	return -1;
}

int tlp_total_length(const struct pcie_tlp *pkt)
{
	int hdr_len = 0;
	int data_len = tlp_data_length(pkt);

	switch (pkt->tlp_fmt) {
	case PCIE_TLP_FMT_3DW_NODATA:
		data_len = 0;
		[[fallthrough]];
	case PCIE_TLP_FMT_3DW_DATA:
		hdr_len = 3;  // 3 DW header
		break;
	case PCIE_TLP_FMT_4DW_NODATA:
		data_len = 0;
		[[fallthrough]];
	case PCIE_TLP_FMT_4DW_DATA:
		hdr_len = 4;  // 4 DW header
		break;
	default:
		return -1;
	}
	return (hdr_len + data_len) * 4;
}

void tlp_req_set_addr(struct pcie_tlp *pkt, uint64_t addr, int length)
{
	uint8_t *p;
	int n;

	int align = addr & 0x3;
	int length_with_align = length + align;

	addr &= ~3ULL;

	pkt->tlp_req.r_first_be = 0xf << align;
	pkt->tlp_req.r_last_be = 0xf >> (-length_with_align & 3);

	if (length_with_align <= 4) {
		pkt->tlp_req.r_first_be &= pkt->tlp_req.r_last_be;
		pkt->tlp_req.r_last_be = 0x0;
	}

	// special case: zero-length read/write
	if (length == 0) {
		length = 4;
		pkt->tlp_req.r_first_be = 0;
	}

	length = (length_with_align + 4 - 1) / 4;

	pkt->tlp_length_hi = length >> 8;
	pkt->tlp_length_lo = length & 0xff;

	if (addr < 0x100000000ULL) {
		p = pkt->tlp_req.r_address32;
		n = sizeof(pkt->tlp_req.r_address32);
		pkt->tlp_fmt &= ~PCIE_TLP_FMT_4DW;
	} else {
		p = pkt->tlp_req.r_address64;
		n = sizeof(pkt->tlp_req.r_address64);
		pkt->tlp_fmt |= PCIE_TLP_FMT_4DW;
	}

	for (int i = 0; i < n; i++)
		p[i] = addr >> 8 * (n - i - 1);
}

uint64_t tlp_req_get_addr(const struct pcie_tlp *pkt)
{
	const uint8_t *p;
	int n;
	uint64_t addr = 0;

	if (pkt->tlp_fmt & PCIE_TLP_FMT_4DW) {
		p = pkt->tlp_req.r_address64;
		n = sizeof(pkt->tlp_req.r_address64);
	} else {
		p = pkt->tlp_req.r_address32;
		n = sizeof(pkt->tlp_req.r_address32);
	}

	for (int i = 0; i < n; i++)
		addr |= (uint64_t)p[i] << 8 * (n - i - 1);

	return addr;
}
