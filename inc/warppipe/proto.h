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

#ifndef WARP_PIPE_PROTO_H
#define WARP_PIPE_PROTO_H

#include <endian.h>
#include <assert.h>
#include <stdint.h>

enum pcie_proto {
	PCIE_PROTO_DLLP = 2,
	PCIE_PROTO_TLP = 3,
};

enum pcie_dllp_type {
	PCIE_DLLP_ACK = 0x00,
	PCIE_DLLP_NAK = 0x10,
	PCIE_DLLP_NOP = 0x31,
};

enum pcie_tlp_fmt {
	PCIE_TLP_FMT_3DW = 0,
	PCIE_TLP_FMT_4DW = 1,
	PCIE_TLP_FMT_NODATA = 0,
	PCIE_TLP_FMT_DATA = 2,

	PCIE_TLP_FMT_3DW_NODATA = PCIE_TLP_FMT_3DW | PCIE_TLP_FMT_NODATA,
	PCIE_TLP_FMT_4DW_NODATA = PCIE_TLP_FMT_4DW | PCIE_TLP_FMT_NODATA,
	PCIE_TLP_FMT_3DW_DATA = PCIE_TLP_FMT_3DW | PCIE_TLP_FMT_DATA,
	PCIE_TLP_FMT_4DW_DATA = PCIE_TLP_FMT_4DW | PCIE_TLP_FMT_DATA,
	PCIE_TLP_FMT_PREFIX = 4,
};

enum pcie_tlp_type {
	PCIE_TLP_MRD32 = PCIE_TLP_FMT_3DW_NODATA << 5 | 0x00,
	PCIE_TLP_MRD64 = PCIE_TLP_FMT_4DW_NODATA << 5 | 0x00,
	PCIE_TLP_MRDLK32 = PCIE_TLP_FMT_3DW_NODATA << 5 | 0x01,
	PCIE_TLP_MRDLK64 = PCIE_TLP_FMT_4DW_NODATA << 5 | 0x01,
	PCIE_TLP_MWR32 = PCIE_TLP_FMT_3DW_DATA << 5 | 0x00,
	PCIE_TLP_MWR64 = PCIE_TLP_FMT_4DW_DATA << 5 | 0x00,
	PCIE_TLP_IORD = PCIE_TLP_FMT_3DW_NODATA << 5 | 0x02,
	PCIE_TLP_IOWR = PCIE_TLP_FMT_3DW_DATA << 5 | 0x02,
	PCIE_TLP_CPL = PCIE_TLP_FMT_3DW_NODATA << 5 | 0x0a,
	PCIE_TLP_CPLD = PCIE_TLP_FMT_3DW_DATA << 5 | 0x0a,
	PCIE_TLP_CR0 = PCIE_TLP_FMT_3DW_NODATA << 5 | 0x04,
	PCIE_TLP_CW0 = PCIE_TLP_FMT_3DW_DATA << 5 | 0x04,
	PCIE_TLP_CR1 = PCIE_TLP_FMT_3DW_NODATA << 5 | 0x05,
	PCIE_TLP_CW1 = PCIE_TLP_FMT_3DW_DATA << 5 | 0x05,
};

union pcie_id {
	uint8_t id[2];
	struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
		uint8_t bus:8;
		uint8_t func:1;
		uint8_t dev:7;
#elif __BYTE_ORDER == __BIG_ENDIAN
		uint8_t bus:8;
		uint8_t dev:7;
		uint8_t func:1;
#else
# error	"__BYTE_ORDER is neither __LITTLE_ENDIAN nor __BIG_ENDIAN. Please fix <bits/endian.h>"
#endif
	};
};

struct pcie_tlp {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	uint8_t tlp_type:5;
	uint8_t tlp_fmt:3;

	uint8_t tlp_th:1;
	uint8_t tlp_ln:1;
	uint8_t tlp_attr_hi:1;
	uint8_t tlp_t8:1;
	uint8_t tlp_tc:3;
	uint8_t tlp_t9:1;

	uint8_t tlp_length_hi:2;
	uint8_t tlp_at:1;
	uint8_t tlp_attr_lo:2;
	uint8_t tlp_ep:1;
	uint8_t tlp_td:1;
#elif __BYTE_ORDER == __BIG_ENDIAN
	uint8_t tlp_fmt:3;
	uint8_t tlp_type:5;

	uint8_t tlp_t9:1;
	uint8_t tlp_tc:3;
	uint8_t tlp_t8:1;
	uint8_t tlp_attr_hi:1;
	uint8_t tlp_ln:1;
	uint8_t tlp_th:1;

	uint8_t tlp_td:1;
	uint8_t tlp_ep:1;
	uint8_t tlp_attr_lo:2;
	uint8_t tlp_at:1;
	uint8_t tlp_length_hi:2;
#else
# error	"__BYTE_ORDER is neither __LITTLE_ENDIAN nor __BIG_ENDIAN. Please fix <bits/endian.h>"
#endif
	uint8_t tlp_length_lo:8;

	union {
		struct {
			union pcie_id r_requester;
			uint8_t r_tag;
#if __BYTE_ORDER == __LITTLE_ENDIAN
			uint8_t r_first_be:4;
			uint8_t r_last_be:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
			uint8_t r_last_be:4;
			uint8_t r_first_be:4;
#else
# error	"__BYTE_ORDER is neither __LITTLE_ENDIAN nor __BIG_ENDIAN. Please fix <bits/endian.h>"
#endif
			union {
				struct {
					uint8_t r_address32[4];
					uint8_t r_data32[];
				};
				struct {
					uint8_t r_address64[8];
					uint8_t r_data64[];
				};
			};
		} tlp_req;
		struct {
			union pcie_id c_completer;
#if __BYTE_ORDER == __LITTLE_ENDIAN
			uint8_t c_byte_count_hi:4;
			uint8_t c_bcm:1;
			uint8_t c_status:3;

			uint8_t c_byte_count_lo:8;

			union pcie_id c_requester;
			uint8_t c_tag;

			uint8_t c_lower_address:7;
			uint8_t c_rsvd1:1;
#elif __BYTE_ORDER == __BIG_ENDIAN
			uint8_t c_status:3;
			uint8_t c_bcm:1;
			uint8_t c_byte_count_hi:4;

			uint8_t c_byte_count_lo:8;

			union pcie_id c_requester;
			uint8_t c_tag;

			uint8_t c_rsvd1:1;
			uint8_t c_lower_address:7;
#else
# error	"__BYTE_ORDER is neither __LITTLE_ENDIAN nor __BIG_ENDIAN. Please fix <bits/endian.h>"
#endif
			uint8_t c_data[];
		} tlp_cpl;
	};
};

struct pcie_dllp {
	union {
		uint8_t dl_type;
		struct {
			uint8_t dl_nak;
			uint8_t dl_rsvd1_hi:8;
#if __BYTE_ORDER == __LITTLE_ENDIAN
			uint8_t dl_seqno_hi:4;
			uint8_t dl_rsvd1_lo:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
			uint8_t dl_rsvd1_lo:4;
			uint8_t dl_seqno_hi:4;
#else
# error	"__BYTE_ORDER is neither __LITTLE_ENDIAN nor __BIG_ENDIAN. Please fix <bits/endian.h>"
#endif
			uint8_t dl_seqno_lo:8;
		} dl_acknak;
		struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
			uint8_t fc_vcid:3;
			uint8_t fc_rsvd1:1;
			uint8_t fc_kind:2;
			uint8_t fc_type:2;

			uint8_t fc_hdrfc_hi:6;
			uint8_t fc_hdr_scale:2;

			uint8_t fc_datafc_hi:4;
			uint8_t fc_data_scale:2;
			uint8_t fc_hdrfc_lo:2;
#elif __BYTE_ORDER == __BIG_ENDIAN
			uint8_t fc_type:2;
			uint8_t fc_kind:2;
			uint8_t fc_rsvd1:1;
			uint8_t fc_vcid:3;

			uint8_t fc_hdr_scale:2;
			uint8_t fc_hdrfc_hi:6;

			uint8_t fc_hdrfc_lo:2;
			uint8_t fc_data_scale:2;
			uint8_t fc_datafc_hi:4;
#else
# error	"__BYTE_ORDER is neither __LITTLE_ENDIAN nor __BIG_ENDIAN. Please fix <bits/endian.h>"
#endif

			uint8_t fc_datafc_lo:8;
		} dl_fc;
	};
	uint8_t dl_crc16[2];
};

struct pcie_dltlp {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	uint8_t dl_seqno_hi:4;
	uint8_t rsvd1:4;
	uint8_t dl_seqno_lo:8;
#elif __BYTE_ORDER == __BIG_ENDIAN
	uint8_t rsvd1:4;
	uint8_t dl_seqno_hi:4;
	uint8_t dl_seqno_lo:8;
#else
# error	"__BYTE_ORDER is neither __LITTLE_ENDIAN nor __BIG_ENDIAN. Please fix <bits/endian.h>"
#endif
	union {
		struct pcie_tlp dl_tlp;
		uint8_t __pad_for_crc32[20];
	};
};

struct warppipe_pcie_transport {
	/*
	 * In C23 it could be `enum pcie_proto : uint8_t`
	 * and then `enum pcie_proto t_proto:8`.
	 * In C11 we can either lose type information, or use packed structs (non-standard).
	 */
	uint8_t t_proto:8;

	union {
		struct pcie_dllp t_dllp;
		struct pcie_dltlp t_tlp;
	};
};

// Address space: Configuration; Transaction types: read,write; (p. 104 PCIE BSR rev 5.0)

// Fmt and Type field encodings (p. 109 PCIE BSR rev 5.0)
//
// TLP Type | FMT  |  Type   | Description
// ---------|------|---------|----------------------------
// CfgRd0   | 000  | 0 0100  | Configuration Read Type 0
// CfgWr0   | 010  | 0 0100  | Configuration Write Type 0

// Type 0 Configuration Space Header
struct pcie_configuration_space_header {
	uint16_t vendor_id: 16; // HwInit
	uint16_t device_id: 16;
	uint16_t command: 16;
	uint16_t status: 16;
	uint8_t revision_id: 8;
	uint32_t class_code: 24;
	uint8_t cache_line_size: 8;
	uint8_t latency_timer: 8;
	uint8_t header_type: 8;
	uint8_t bist: 8;

	uint32_t bar[6];

	uint32_t cardbus_cis_pointer: 32;
	uint16_t subsystem_vendor_id: 16;
	uint16_t subsystem_id: 16;
	uint32_t expansion_rom_base_address: 32;
	uint8_t capabilities_pointer: 8;
	uint32_t _reserved0: 24;
	uint32_t _reserved1: 32;
	uint8_t interrupt_line: 8;
	uint8_t interrupt_pin: 8;
	uint8_t min_gnt: 8; // Does not apply to PCI Express. Read-only. Hardwire to 0.
	uint8_t max_lat: 8; // Does not apply to PCI Express. Read-only. Hardwire to 0.
};

static_assert(sizeof(struct pcie_dllp) == 6);
static_assert(sizeof(struct pcie_tlp) == 16);
static_assert(sizeof(struct warppipe_pcie_transport) == 23);

int tlp_data_length(const struct pcie_tlp *pkt);
int tlp_data_length_bytes(const struct pcie_tlp *pkt);
int tlp_total_length(const struct pcie_tlp *pkt);
void tlp_req_set_addr(struct pcie_tlp *pkt, uint64_t addr, int length);
uint64_t tlp_req_get_addr(const struct pcie_tlp *pkt);

#endif /* WARP_PIPE_PROTO_H */
