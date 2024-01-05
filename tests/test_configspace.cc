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

#include <gtest/gtest.h>
#include "common.h"
#include <stdio.h>

extern "C" {
#include <warppipe/proto.h>
#include <warppipe/yaml_configspace.h>
}

#define SPLIT2(hi, lo) ((hi << 8) | lo)
#define SPLIT3(hi, mid, lo) ((hi << 16) | (mid << 8) | lo)
#define SPLIT4(hi, mid_hi, mid_lo, lo) ((hi << 24) | (mid_hi << 16) | (mid_lo << 8) | lo)

TEST(TestConfigSpace, ParseTestFile) {
	struct pcie_configuration_space_header_type0 header;
	int retval = 0;

	FILE* config = tmpfile();
	fputs("vendor_id: 0x144d \n\
device_id: 0xa809 \n\
command: 1 \n\
status: 2 \n\
revision_id: 3 \n\
class_code: 4 \n\
cache_line_size: 5 \n\
latency_timer: 6 \n\
header_type: 7 \n\
bist: 8 \n\
cardbus_cis_pointer: 0 \n\
subsystem_vendor_id: 0xaaff \n\
subsystem_id: 0xf \n\
expansion_rom_base_address: 0x10 \n\
capabilities_pointer: 0x0 \n\
interrupt_line: 10 \n\
interrupt_pin: 11 \n\
min_gnt: 12 \n\
max_lat: 13", config);
	rewind(config);

	retval = pcie_configuration_space_header_from_yaml(config, &header);
	ASSERT_EQ(retval, 0);

	ASSERT_EQ(SPLIT2(header.device_id[1], header.device_id[0]), 0xa809);
	ASSERT_EQ(SPLIT2(header.command[1], header.command[0]), 1);
	ASSERT_EQ(SPLIT2(header.command[1], header.command[0]), 1);
	ASSERT_EQ(SPLIT2(header.status[1], header.status[0]), 2);
	ASSERT_EQ(header.revision_id, 3);
	ASSERT_EQ(SPLIT3(header.class_code[2], header.class_code[1], header.class_code[0]), 4);
	ASSERT_EQ(header.cache_line_size, 5);
	ASSERT_EQ(header.latency_timer, 6);
	ASSERT_EQ(header.header_type, 7);
}

TEST(TestConfigSpace, ParseTestFileNegative) {
	struct pcie_configuration_space_header_type0 header;
	int retval = 0;

	FILE* config = tmpfile();
	fputs("vendor_id: m \n\
device_id: 0 \n\
command: 1 \n\
status: 2 \n\
revision_id: 3 \n\
class_code: 4 \n\
cache_line_size: 5 \n\
latency_timer: 6 \n\
header_type: 7 \n\
bist: 8 \n\
cardbus_cis_pointer: 0 \n\
subsystem_vendor_id: 0xaaff \n\
subsystem_id: 0xf \n\
expansion_rom_base_address: 0x10 \n\
capabilities_pointer: 0x0 \n\
interrupt_line: 10 \n\
interrupt_pin: 11 \n\
min_gnt: 12 \n\
max_lat: 13", config);
	rewind(config);

	retval = pcie_configuration_space_header_from_yaml(config, &header);
	ASSERT_NE(retval, 0);
}
