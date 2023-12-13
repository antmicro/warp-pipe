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

#include <warppipe/crc.h>
#include <warppipe/proto.h>

uint32_t crc32p(const void *start, const void *end, uint32_t init, uint32_t poly)
{
	uint32_t crc = init;

	for (const uint8_t *data = start; data != end; data++) {
		crc = crc ^ *data;
		for (int i = 0; i < 8; i++) {
			if (crc & 1)
				crc = (crc >> 1) ^ poly;
			else
				crc = crc >> 1;
		}
	}
	return crc;
}

void pcie_crc16(struct pcie_dllp *pkt)
{
	uint16_t crc = ~crc32p(pkt, pkt->dl_crc16, 0xffff, DLLP_CRC16_POLY);

	for (int i = 0; i < 2; i++) {
		pkt->dl_crc16[i] = crc & 0xff;
		crc = crc >> 8;
	}
}

bool pcie_crc16_valid(struct pcie_dllp *pkt)
{
	uint16_t crc = ~crc32p(pkt, pkt->dl_crc16, 0xffff, DLLP_CRC16_POLY);

	for (int i = 0; i < 2; i++) {
		if (pkt->dl_crc16[i] != (crc & 0xff))
			return false;
		crc = crc >> 8;
	}
	return true;
}

void pcie_lcrc32(struct pcie_dltlp *pkt)
{
	int total_length = tlp_total_length(&pkt->dl_tlp);
	uint8_t *end = (uint8_t *)&pkt->dl_tlp + total_length;

	uint32_t crc = ~crc32p(pkt, end, 0xffffffff, TLP_LCRC32_POLY);

	for (int i = 0; i < 4; i++) {
		end[i] = crc & 0xff;
		crc = crc >> 8;
	}
}

bool pcie_lcrc32_valid(struct pcie_dltlp *pkt)
{
	int total_length = tlp_total_length(&pkt->dl_tlp);
	uint8_t *end = (uint8_t *)&pkt->dl_tlp + total_length;

	uint32_t crc = ~crc32p(pkt, end, 0xffffffff, TLP_LCRC32_POLY);

	for (int i = 0; i < 4; i++) {
		if (end[i] != (crc & 0xff))
			return false;
		crc = crc >> 8;
	}
	return true;
}
