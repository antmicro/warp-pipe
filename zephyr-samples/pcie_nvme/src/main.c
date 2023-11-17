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
#include <zephyr/storage/disk_access.h>

#define DISK_NAME "nvme0n0"

static const char *disk_pdrv = DISK_NAME;

int main(void)
{
	int rc, i;
	uint32_t cmd_buf, sector_sz;
	uint8_t *wbuf, *rbuf;

	rc = disk_access_init(disk_pdrv);
	if (rc) {
		printf("Disk access init failed: %d\n", rc);
		return -1;
	}

	printf("Checking disk status... ");
	rc = disk_access_status(disk_pdrv);
	if (rc == DISK_STATUS_OK)
		printf("OK\n");
	else
		printf("Fail\n");

	rc = disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_COUNT, &cmd_buf);
	printf("Disk reports %u sectors\n", cmd_buf);

	rc = disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE, &cmd_buf);
	printf("Disk reports sector size %u\n", cmd_buf);

	sector_sz = cmd_buf;
	wbuf = malloc(sector_sz);
	for (i = 0; i < sector_sz; i++)
		wbuf[i] = 2 * i;
	rbuf = malloc(sector_sz);

	printf("Performing writes to a single disk sector...\n");
	disk_access_write(disk_pdrv, wbuf, 10, 1);

	printf("Reading and verifying data... ");
	disk_access_read(disk_pdrv, rbuf, 10, 1);
	for (i = 0; i < sector_sz; i++) {
		if (wbuf[i] != rbuf[i]) {
			printf("rbuf[%d] != wbuf[%d] (%d != %d)!", i, i, rbuf[i], wbuf[i]);
			return -1;
		}
	}
	printf("Correct!\n");

	return 0;
}
