#include <bits/endian.h>
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

enum pcie_tlp_type {
	PCIE_TLP_MRD32 = 0x00,
	PCIE_TLP_MRD64 = 0x20,
	PCIE_TLP_MRDLK32 = 0x01,
	PCIE_TLP_MRDLK64 = 0x21,
	PCIE_TLP_MWR32 = 0x40,
	PCIE_TLP_MWR64 = 0x60,
	PCIE_TLP_IORD = 0x02,
	PCIE_TLP_IOWR = 0x42,
	PCIE_TLP_CPL = 0x0a,
	PCIE_TLP_CPLD = 0x4a,
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
	uint8_t dl_crc16_hi, dl_crc16_lo;
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
	struct pcie_tlp dl_tlp;
	/* uint32_t lcrc; */
};

struct pcie_transport {
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

static_assert(sizeof(struct pcie_dllp) == 6);
static_assert(sizeof(struct pcie_tlp) == 16);
static_assert(sizeof(struct pcie_transport) == 19);
