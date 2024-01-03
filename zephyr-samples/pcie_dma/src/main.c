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
typedef int8_t dma_data_type;

dma_data_type *data_end;

void finished_cb(const struct device *dev, void *user_data,
			       uint32_t channel, int status)
{
	int32_t i;

	printf("Data transfer finished: ");
	for (i = 0; i < DATA_SIZE; ++i)
		printf("0x%x ", data_end[i]);
	printf("\n");

	free(data_end);
}

void configure_dma(const struct device *const dma_device, int32_t channel)
{
	struct dma_block_config block = {
		.source_address = 0x0, // address offset in remote DMA memory
		.dest_address = (uint32_t) data_end,
		.block_size = DATA_SIZE
	};
	struct dma_config conf = {
		.dma_slot = 0,
		.source_burst_length = 4,
		.dest_burst_length = 4,
		.block_count = 1,
		.head_block = &block,
		.dma_callback = finished_cb	// Callback to finished transfer
	};

	dma_config(dma_device, 0, &conf);	// Configure the channel
}

int main(void)
{
	const struct device *const dma_device = DEVICE_DT_GET(DT_NODELABEL(dma));

	data_end = malloc(DATA_SIZE * sizeof(dma_data_type));

	configure_dma(dma_device, 0);		// Configure the channel
	dma_start(dma_device, 0);		// Start transmition

	return 0;
}
