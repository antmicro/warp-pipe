#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <stdbool.h>

#include <warppipe/client.h>
#include <warppipe/server.h>

struct read_compl_data_t {
	uint8_t *buf;
	uint32_t buf_size;
	int ret;
	bool finished;
};

/* Read a single field from configuration space header.
 *
 * params:
 *	server: warppipe server
 *	client: warppipe client
 *	addr (offset): address of field from configuration space header that will be read
 *	length: size of the requested field
 *	buf: buffer for holding read data
 *
 * NOTE: The field address (offset) and size must be consistent with the specification (see https://wiki.osdev.org/PCI#Header_Type_0x0).
 * Only 32 bit values are supported.
 */
int read_config_header_field(struct warppipe_server_t *server, struct warppipe_client_t *client, uint64_t addr, int length, uint8_t *buf);

/* Read data from memory registed by given BAR.
 *
 * params:
 *	server: warppipe server
 *	client: warppipe client
 *	bar: bar index
 *	addr: address (offset) inside a BAR registered region
 *	length: length of requested read
 *	buf: buffer for holding read data
 */
int read_data(struct warppipe_server_t *server, struct warppipe_client_t *client, int bar, uint64_t addr, int length, uint8_t *buf);

#endif /* __COMMON_H__ */
