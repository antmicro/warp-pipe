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

#ifndef WARP_PIPE_CONFIG_H
#define WARP_PIPE_CONFIG_H

#define PRJ_NAME_LONG			"Warp Pipe"
#define PRJ_NAME_SHORT			"warp-pipe"

#define SERVER_PORT_NUM			"2115"
#define SERVER_LISTEN_QUEUE_SIZE	64

#define CLIENT_MAX_PACKET_DATA_SIZE	4096
#define CLIENT_MAX_PACKET_HEADER_SIZE	21 /* 1 PROTO + 16 TLP + 4 LCRC32 */
#define CLIENT_BUFFER_SIZE		(CLIENT_MAX_PACKET_DATA_SIZE + CLIENT_MAX_PACKET_HEADER_SIZE)

#endif /* WARP_PIPE_CONFIG_H */
