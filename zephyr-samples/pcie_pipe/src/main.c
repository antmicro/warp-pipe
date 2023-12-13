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

#include <stdlib.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pcie/msi.h>

#define DATA_SIZE 32

void irq_handle(const void *unused)
{
	printf("Received MSI-X IRQ!\n");
}

int main(void)
{
#ifndef WARP_PIPE_LISTEN
	mm_reg_t mm;

	device_map(&mm, 0xfea00000, 0x100, K_MEM_CACHE_NONE);
	printf("Initial memory values: ");
	for (int i = 0; i < DATA_SIZE; i++) {
		if (i % 8 == 0)
			printf("\n");

		uint32_t read = sys_read32(mm + i * 4);

		printf("0x%x ", read);
	}
	printf("\n");

	printf("Updated memory values: ");
	for (int i = 0; i < DATA_SIZE; i++) {
		if (i % 8 == 0)
			printf("\n");

		sys_write32(0xABABABAB, mm + i * 4);

		uint32_t read = sys_read32(mm + i * 4);

		printf("0x%x ", read);
	}
	printf("\n");

	uint8_t n_vectors;
	msi_vector_t vectors[1];
	pcie_bdf_t bdf = PCIE_BDF(0, 2, 0);

	n_vectors = pcie_msi_vectors_allocate(bdf,
					      1,
					      vectors,
					      1);
	if (n_vectors == 0) {
		printf("Could not allocate %u MSI-X vectors\n",
			1);
		return -1;
	}

	printf("Allocated %u vectors\n", n_vectors);

	for (int i = 0; i < n_vectors; i++) {

		if (!pcie_msi_vector_connect(bdf,
					     &vectors[i],
					     irq_handle,
					     NULL, 0)) {
			printf("Failed to connect MSI-X vector %u\n", i);
			return -1;
		}
	}

	printf("%u MSI-X Vectors connected\n", n_vectors);

	if (!pcie_msi_enable(bdf, vectors, n_vectors, 0)) {
		printf("Could not enable MSI-X\n");
		return -1;
	}
#endif
	return 0;
}
