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

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>

#include <pcie_comm/server.h>
#include <pcie_comm/client.h>
#include <pcie_comm/config.h>

static bool quit;
static struct server_t server;

static void handle_sigint(int signo)
{
	quit = true;
	syslog(LOG_NOTICE, "Received SIGINT. " PRJ_NAME_LONG " will shut down.");

	/* disconnect every user */
	server_disconnect_clients(&server, NULL);
}

int main(void)
{
	int ret;

	/* syslog initialization */
	setlogmask(LOG_UPTO(LOG_DEBUG));
	openlog(PRJ_NAME_SHORT, LOG_CONS | LOG_NDELAY, LOG_USER);

	/* signal initialization */
	signal(SIGINT, handle_sigint);

	/* server initialization */
	quit = false;
	syslog(LOG_NOTICE, "Starting " PRJ_NAME_LONG "...");

	ret = server_create(&server);
	if (!ret)
		while (!quit)
			server_loop(&server);
	else
		syslog(LOG_NOTICE, "Failed to set up server for " PRJ_NAME_LONG ".");


	syslog(LOG_NOTICE, "Shutting down " PRJ_NAME_LONG ".");
	closelog();

	return 0;
}
