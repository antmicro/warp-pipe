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
#include <zephyr/kernel.h>

#include <zephyr/drivers/virtualization/ivshmem.h>
int main(void)
{
	const struct device *ivshmem;
	uintptr_t mem;
	size_t size;
	uint32_t *data_ptr;

	ivshmem = DEVICE_DT_GET_ONE(qemu_ivshmem);
	if (!device_is_ready(ivshmem)) {
		printf("ivshmem device is not ready!\n");
		return -1;
	}

	size = ivshmem_get_mem(ivshmem, &mem);
	if (size == 0) {
		printf("Size cannot not be 0!\n");
		return -1;
	}
	printf("Shared memory size: 0x%lx\n", size);
	if (!(void *)mem) {
		printf("Shared memory cannot be null!\n");
		return -1;
	}

	data_ptr = (uint32_t *)mem;

	for (int i = 0; i < 512; i++)
		data_ptr[i] = i;

	printf("Reading and verifying data... ");
	for (int i = 0; i < 512; i++) {
		if (data_ptr[i] != i) {
			printf("data_ptr[%d] != %d!", i, i);
			return -1;
		}
	}
	printf("Correct!\n");

	return 0;
}
