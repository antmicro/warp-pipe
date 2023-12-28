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

#include <errno.h>
#include <limits.h>
#include <yaml.h>
#include <warppipe/proto.h>

#define GET_HI_OF_SPLIT2(number) (number >> 8)
#define GET_LO_OF_SPLIT2(number) (number & 0xFF)

#define GET_HI_OF_SPLIT3(number) (number >> 16)
#define GET_MID_OF_SPLIT3(number) ((number >> 8) & 0xFF)
#define GET_LO_OF_SPLIT3(number) (number & 0xFF)

#define GET_HI_OF_SPLIT4(number) (number >> 24)
#define GET_MID_HI_OF_SPLIT4(number) ((number >> 16) & 0xFF)
#define GET_MID_LO_OF_SPLIT4(number) ((number >> 8) & 0xFF)
#define GET_LO_OF_SPLIT4(number) (number & 0xFF)

/* parser_keys correspond to `PARSE_` states in enum parser_state and must be provided in the same order. */
const char * const parser_keys[] = {
	"vendor_id",
	"device_id",
	"command",
	"status",
	"revision_id",
	"class_code",
	"cache_line_size",
	"latency_timer",
	"header_type",
	"bist",
	"cardbus_cis_pointer",
	"subsystem_vendor_id",
	"subsystem_id",
	"expansion_rom_base_address",
	"capabilities_pointer",
	"interrupt_line",
	"interrupt_pin",
	"min_gnt",
	"max_lat"
};

enum parser_state {
	PARSE_VENDOR_ID,
	PARSE_DEVICE_ID,
	PARSE_COMMAND,
	PARSE_STATUS,
	PARSE_REVISION_ID,
	PARSE_CLASS_CODE,
	PARSE_CACHE_LINE_SIZE,
	PARSE_LATENCY_TIMER,
	PARSE_HEADER_TYPE,
	PARSE_BIST,
	PARSE_CARDBUS_CIS_POINTER,
	PARSE_SUBSYSTEM_VENDOR_ID,
	PARSE_SUBSYSTEM_ID,
	PARSE_EXPANSION_ROM_BASE_ADDRESS,
	PARSE_CAPABILITIES_POINTER,
	PARSE_INTERRUPT_LINE,
	PARSE_INTERRUPT_PIN,
	PARSE_MIN_GNT,
	PARSE_MAX_LAT,
	DONE,
	PARSE_KEY
};

/**
 * Fill already allocated `struct pcie_configuration_space_header_type0` with data from a YAML file.
 * pathname - name of an existing YAML file
 * result - non-NULL struct pcie_configuration_space_header_type0 to fill with the data
 */
int process_event(yaml_event_t *event, struct pcie_configuration_space_header_type0 *result, enum parser_state *state)
{
	char *value = 0;
	char *check_err = 0;
	unsigned long parsed_numeric = 0;
	enum parser_state next_state = 0;
	int retval = 0;

	if (!event || !result || !state)
		return -EINVAL;

	next_state = *state;
	errno = 0;

	switch (event->type) {
	case YAML_NO_EVENT:
	case YAML_MAPPING_END_EVENT:
	case YAML_DOCUMENT_END_EVENT:
	case YAML_STREAM_END_EVENT:
		next_state = DONE;
		break;
	case YAML_SCALAR_EVENT:
		value = (char *)event->data.scalar.value;
		parsed_numeric = strtoul(value, &check_err, 0);

		switch (*state) {
		case PARSE_KEY:
			for (int i = 0; i < DONE; i++) {
				if (strcmp(value, parser_keys[i]) == 0) {
					next_state = i;
					break;
				}
			}
			break;
		case PARSE_VENDOR_ID:
			result->vendor_id[1] = GET_HI_OF_SPLIT2(parsed_numeric);
			result->vendor_id[0] = GET_LO_OF_SPLIT2(parsed_numeric);
			next_state = PARSE_KEY;
			break;
		case PARSE_DEVICE_ID:
			result->device_id[1] = GET_HI_OF_SPLIT2(parsed_numeric);
			result->device_id[0] = GET_LO_OF_SPLIT2(parsed_numeric);
			next_state = PARSE_KEY;
			break;
		case PARSE_COMMAND:
			result->command[1] = GET_HI_OF_SPLIT2(parsed_numeric);
			result->command[0] = GET_LO_OF_SPLIT2(parsed_numeric);
			next_state = PARSE_KEY;
			break;
		case PARSE_STATUS:
			result->status[1] = GET_HI_OF_SPLIT2(parsed_numeric);
			result->status[0] = GET_LO_OF_SPLIT2(parsed_numeric);
			next_state = PARSE_KEY;
			break;
		case PARSE_REVISION_ID:
			result->revision_id = parsed_numeric;
			next_state = PARSE_KEY;
			break;
		case PARSE_CLASS_CODE:
			result->class_code[2] = GET_HI_OF_SPLIT3(parsed_numeric);
			result->class_code[1] = GET_MID_OF_SPLIT3(parsed_numeric);
			result->class_code[0] = GET_LO_OF_SPLIT3(parsed_numeric);
			next_state = PARSE_KEY;
			break;
		case PARSE_CACHE_LINE_SIZE:
			result->cache_line_size = parsed_numeric;
			next_state = PARSE_KEY;
			break;
		case PARSE_LATENCY_TIMER:
			result->latency_timer = parsed_numeric;
			next_state = PARSE_KEY;
			break;
		case PARSE_HEADER_TYPE:
			result->header_type = parsed_numeric;
			next_state = PARSE_KEY;
			break;
		case PARSE_BIST:
			result->bist = parsed_numeric;
			next_state = PARSE_KEY;
			break;
		case PARSE_CARDBUS_CIS_POINTER:
			result->cardbus_cis_pointer = GET_HI_OF_SPLIT4(parsed_numeric) << 24 | GET_MID_HI_OF_SPLIT4(parsed_numeric) << 16 | GET_MID_LO_OF_SPLIT4(parsed_numeric) << 8 | GET_LO_OF_SPLIT4(parsed_numeric);
			next_state = PARSE_KEY;
			break;
		case PARSE_SUBSYSTEM_VENDOR_ID:
			result->subsystem_vendor_id[1] = GET_HI_OF_SPLIT2(parsed_numeric);
			result->subsystem_vendor_id[0] = GET_LO_OF_SPLIT2(parsed_numeric);
			next_state = PARSE_KEY;
			break;
		case PARSE_SUBSYSTEM_ID:
			result->subsystem_id[1] = GET_HI_OF_SPLIT2(parsed_numeric);
			result->subsystem_id[0] = GET_LO_OF_SPLIT2(parsed_numeric);
			next_state = PARSE_KEY;
			break;
		case PARSE_EXPANSION_ROM_BASE_ADDRESS:
			result->expansion_rom_base_address = GET_HI_OF_SPLIT4(parsed_numeric) << 24 | GET_MID_HI_OF_SPLIT4(parsed_numeric) << 16 | GET_MID_LO_OF_SPLIT4(parsed_numeric) << 8 | GET_LO_OF_SPLIT4(parsed_numeric);
			next_state = PARSE_KEY;
			break;
		case PARSE_CAPABILITIES_POINTER:
			result->capabilities_pointer = parsed_numeric;
			next_state = PARSE_KEY;
			break;
		case PARSE_INTERRUPT_LINE:
			result->interrupt_line = parsed_numeric;
			next_state = PARSE_KEY;
			break;
		case PARSE_INTERRUPT_PIN:
			result->interrupt_pin = parsed_numeric;
			next_state = PARSE_KEY;
			break;
		case PARSE_MIN_GNT:
			result->min_gnt = parsed_numeric;
			next_state = PARSE_KEY;
			break;
		case PARSE_MAX_LAT:
			result->max_lat = parsed_numeric;
			next_state = PARSE_KEY;
			break;
		case DONE:
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	// Handle strtoul errors.
	if ((event->type == YAML_SCALAR_EVENT && *state != PARSE_KEY)
		&& (check_err == value || *check_err != '\0' || (parsed_numeric == ULONG_MAX && errno == ERANGE))) {

		fprintf(stderr, "Error when parsing unsigned long value: %s.\n", (char *)event->data.scalar.value);
		retval = -ERANGE;
	}

	*state = next_state;
	return retval;
}

int pcie_configuration_space_header_from_yaml(FILE *data, struct pcie_configuration_space_header_type0 *result)
{
	int retval = 0;
	yaml_parser_t parser;
	enum parser_state state = PARSE_KEY;

	if (result == NULL)
		return -EINVAL;

	if (data == NULL)
		return -ENOENT;

	yaml_parser_initialize(&parser);
	yaml_parser_set_input_file(&parser, data);

	do {
		yaml_event_t event;

		retval = yaml_parser_parse(&parser, &event);

		if (retval == 0) {
			// libyaml functions return 0 on failure
			retval = -1;
		} else {
			retval = process_event(&event, result, &state);
			yaml_event_delete(&event);
		}
	} while (state != DONE && retval == 0);

	yaml_parser_delete(&parser);
	fclose(data);
	return retval;
}
