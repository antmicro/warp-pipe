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

#ifndef PCIE_COMM_MACRO_H
#define PCIE_COMM_MACRO_H

#ifdef __ZEPHYR__
#define INCLUDE_EXTRA_PATH zephyr/posix/
#else
#define INCLUDE_EXTRA_PATH
#endif

#define IDENT(x) x
#define XSTR(x) #x
#define STR(x) XSTR(x)
#define ZEPHYR_INCLUDE(x) STR(IDENT(INCLUDE_EXTRA_PATH)IDENT(x))

#endif /* PCIE_COMM_MACRO_H */
