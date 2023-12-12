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

#define DATA_SIZE 32

int main(void)
{
	mm_reg_t mm;
	uint8_t result[DATA_SIZE];

	device_map(&mm, 0xfea00000, 0x100, K_MEM_CACHE_NONE);
	for (int i = 0; i < DATA_SIZE; i++) {
		uint32_t read = sys_read32(mm);

		result[i] = (uint8_t)read;
	}

	printf("Data transfer finished: ");
	for (int i = 0; i < DATA_SIZE; ++i)
		printf("0x%x ", result[i]);
	printf("\n");


	return 0;
}
