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

#ifndef PCIE_COMM_CRC_H
#define PCIE_COMM_CRC_H

#include <stdbool.h>
#include <stdint.h>

/* The polynomials below are taken from the PCIe 5.0 spec, but with reversed bit
 * order so as to avoid unnecessary convoluted steps described there.
 */
#define DLLP_CRC16_POLY 0xd008
#define TLP_LCRC32_POLY 0xedb88320

struct pcie_dllp;
struct pcie_dltlp;

void pcie_crc16(struct pcie_dllp *pkt);
bool pcie_crc16_valid(struct pcie_dllp *pkt);
void pcie_lcrc32(struct pcie_dltlp *pkt);
bool pcie_lcrc32_valid(struct pcie_dltlp *pkt);
uint32_t crc32p(const void *start, const void *end, uint32_t init,
		uint32_t poly);

#endif /* PCIE_COMM_CRC_H */
