#include <zephyr/logging/log.h>

#include <warppipe/client.h>

#include "common.h"

LOG_MODULE_REGISTER(common, LOG_LEVEL_DBG);

static void read_compl(const struct warppipe_completion_status, const void *, int, void *);
static int wait_for_completion(struct warppipe_server *, struct read_compl_data *);

int read_config_header_field(struct warppipe_server *server, struct warppipe_client *client, uint64_t addr, int length, uint8_t *buf)
{
	int ret;
	uint64_t aligned_addr = addr & ~0x3;
	uint64_t offset = addr & 0x3;
	uint32_t aligned_buf;

	if (length + offset > 4) {
		LOG_ERR("%s: Invalid arguments: 0x%x, %d", __func__, addr, length);
		return -1;
	}

	struct read_compl_data read_data = {
		.finished = false,
		.buf = (void *)&aligned_buf,
		.buf_size = 4,
		.ret = 0,
	};

	client->private_data = (void *)&read_data;
	ret = warppipe_config0_read(client, aligned_addr, 4, &read_compl);
	if (ret < 0) {
		LOG_ERR("Failed to read config space at addr %lx-%lx", addr, addr + length);
		return ret;
	}

	ret = wait_for_completion(server, &read_data);
	if (ret < 0)
		return ret;

	memcpy(buf, (uint8_t *)&aligned_buf + offset, length);
	return read_data.ret;
}

int read_data(struct warppipe_server *server, struct warppipe_client *client, int bar, uint64_t addr, int length, uint8_t *buf)
{
	int ret;
	struct read_compl_data read_data = {
		.finished = false,
		.buf = (void *)buf,
		.buf_size = length,
		.ret = 0,
	};

	client->private_data = (void *)&read_data;
	ret = warppipe_read(client, bar, addr, length, &read_compl);
	if (ret < 0) {
		LOG_ERR("Failed to read bar %d at addr %lx-%lx", bar, addr, addr + length);
		return ret;
	}

	ret = wait_for_completion(server, &read_data);
	if (ret < 0)
		return ret;

	return read_data.ret;
}

static void read_compl(const struct warppipe_completion_status completion_status, const void *data, int length, void *private_data)
{
	struct read_compl_data *read_data = (struct read_compl_data *)private_data;

	read_data->finished = true;

	if (completion_status.error_code) {
		read_data->ret = completion_status.error_code;
		return;
	}

	if (length > read_data->buf_size) {
		LOG_WRN("Read %d bytes, expected at most %d (%d bytes will be lost)", length, read_data->buf_size, length - read_data->buf_size);
		length = read_data->buf_size;
	}

	read_data->ret = length;
	memcpy(read_data->buf, data, length);
}

static int wait_for_completion(struct warppipe_server *server, struct read_compl_data *read_data)
{
	while (!read_data->finished && !server->quit)
		warppipe_server_loop(server);

	if (server->quit) {
		LOG_ERR("Server disconnected");
		return -1;
	}
	return 0;
}
