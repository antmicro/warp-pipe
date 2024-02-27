/*
 * Copyright 2024 Antmicro <www.antmicro.com>
 * Copyright 2024 Meta
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

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/queue.h>

#include <warppipe/client.h>
#include <warppipe/server.h>
#include <warppipe/config.h>

#include "../../common/common.h"


LOG_MODULE_REGISTER(pcie_native, LOG_LEVEL_DBG);

#define STACK_SIZE 1024
#define NUM_THREADS 506
#define PRIORITY -1

#ifndef TRANSACTION_LENGTH
#define TRANSACTION_LENGTH 2048
#endif

#ifndef ITERATIONS
#define ITERATIONS 100
#endif


static struct k_thread threads[NUM_THREADS];

static K_THREAD_STACK_ARRAY_DEFINE(stacks, NUM_THREADS, STACK_SIZE);

// Use for access to pcie_server, client.
pthread_rwlock_t rwlock;

static struct warppipe_server pcie_server = {
	.listen = false,
	.addr_family = AF_UNSPEC,
	.host = CONFIG_PCIE_PIPE_SERVER,
	.port = SERVER_PORT_NUM,
	.quit = false,
};

static struct warppipe_client *client;

#ifdef DEBUG
static void print_read_data(uint8_t *buffer, int length)
{
	LOG_PRINTK("Read data (len: %d):", length);
	for (int i = 0; i < length; i++)
		LOG_PRINTK(" %02x", buffer[i]);
	LOG_PRINTK("\n");
}
#endif

void thread_task(int i)
{
	uint8_t buf[TRANSACTION_LENGTH];
	int ret;

	for (int k = 0; k < ITERATIONS; k++) {
		pthread_rwlock_rdlock(&rwlock);
			ret = read_data(&pcie_server, client, 1, 0, TRANSACTION_LENGTH, buf);
		pthread_rwlock_unlock(&rwlock);

		assert(ret == TRANSACTION_LENGTH);
#ifdef DEBUG
		print_read_data(buf, ret);
#endif

		// Write the sequence of i...i+TRANSACTION_LENGTH.
		for (int j = 0; j < TRANSACTION_LENGTH; j++)
			buf[j] = j+i;

		pthread_rwlock_wrlock(&rwlock);
			ret = warppipe_write(client, 1, 0, buf, TRANSACTION_LENGTH);
		pthread_rwlock_unlock(&rwlock);
		assert(ret == 0);

		pthread_rwlock_rdlock(&rwlock);
			ret = read_data(&pcie_server, client, 1, 0, TRANSACTION_LENGTH, buf);
		pthread_rwlock_unlock(&rwlock);
		assert(ret == TRANSACTION_LENGTH);
#ifdef DEBUG
		print_read_data(buf, ret);
#endif

		k_msleep(200);
	}
}

static void init_threads(void)
{
	pthread_rwlock_init(&rwlock, NULL);

	for (int i = 0 ; i < NUM_THREADS; i++) {
		k_thread_create(&threads[i], (k_thread_stack_t *)&stacks[i], STACK_SIZE,
			(k_thread_entry_t)thread_task, (void *)i, NULL, NULL,
			PRIORITY, K_USER, K_FOREVER);
	}
}

static void start_threads(void)
{
	for (int i = 0 ; i < NUM_THREADS; i++)
		k_thread_start(&threads[i]);
}

static void wait_for_threads_done(void)
{
	for (int i = 0 ; i < NUM_THREADS; i++)
		k_thread_join(&threads[i], K_FOREVER);
}

int main(void)
{
	int ret;

	LOG_INF("Started app");

	if (warppipe_server_create(&pcie_server) == -1) {
		LOG_ERR("Failed to connect to warpipe server!");
		return -1;
	}

	LOG_INF("Started client");

	client = TAILQ_FIRST(&pcie_server.clients)->client;

	ret = enumerate(&pcie_server, client);
	if (ret < 0) {
		LOG_ERR("Failed to enumerate: %d\n", ret);
		return -1;
	}

	init_threads();
	start_threads();
	wait_for_threads_done();

	LOG_INF("Done!");
	return 0;
}
