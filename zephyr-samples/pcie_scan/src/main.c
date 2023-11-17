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

#include <zephyr/drivers/pcie/pcie.h>
#include <stdio.h>

bool scan_cb(pcie_bdf_t bdf, pcie_id_t id, void *cb_data)
{
	uint32_t conf_id = pcie_conf_read(bdf, PCIE_CONF_ID);
	uint32_t classrev = pcie_conf_read(bdf, PCIE_CONF_CLASSREV);
	uint32_t type = pcie_conf_read(bdf, PCIE_CONF_TYPE);
	uint32_t bar_conf;
	int bar;

	printf("[%d:%x.%d] Found PCIe endpoint: vendor=0x%x device=0x%x ",
			PCIE_BDF_TO_BUS(bdf), PCIE_BDF_TO_DEV(bdf),
			PCIE_BDF_TO_FUNC(bdf), PCIE_ID_TO_VEND(conf_id),
			PCIE_ID_TO_DEV(conf_id));
	printf("class=0x%x subclass=0x%x progif=0x%x rev=0x%x bridge=%d\n",
			PCIE_CONF_CLASSREV_CLASS(classrev),
			PCIE_CONF_CLASSREV_SUBCLASS(classrev),
			PCIE_CONF_CLASSREV_PROGIF(classrev),
			PCIE_CONF_CLASSREV_REV(classrev),
			PCIE_CONF_TYPE_BRIDGE(type));

	if (!PCIE_CONF_TYPE_BRIDGE(type)) {
		for (bar = PCIE_CONF_BAR0; bar <= PCIE_CONF_BAR5; bar++) {
			bar_conf = pcie_conf_read(bdf, bar);

			if (conf_id == PCIE_CONF_BAR_NONE)
				continue;
			printf("[%d:%x.%d]\t BAR %d: type=%s",
					PCIE_BDF_TO_BUS(bdf),
					PCIE_BDF_TO_DEV(bdf),
					PCIE_BDF_TO_FUNC(bdf),
					bar - PCIE_CONF_BAR0,
					PCIE_CONF_BAR_IO(bar_conf) ? "io" : "mem");

			if (PCIE_CONF_BAR_64(bar_conf)) {
				printf(" size=64 addr=0x%08x", pcie_conf_read(bdf, ++bar));
				printf("%08lx\n", PCIE_CONF_BAR_ADDR(bar_conf));
			} else {
				printf(" size=32 addr=0x%08lx\n", PCIE_CONF_BAR_ADDR(bar_conf));
			}
		}

	}

	return true;
}

int main(void)
{
	struct pcie_scan_opt scan_opt = {
		.cb = scan_cb,
		.cb_data = NULL,
		.flags = (PCIE_SCAN_RECURSIVE | PCIE_SCAN_CB_ALL),
	};

	printf("Scanning PCIe endpoints...\n");
	pcie_scan(&scan_opt);

	return 0;
}
