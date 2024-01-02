# Copyright 2023 Antmicro <www.antmicro.com>
# Copyright 2023 Meta
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pcie_native)

set(app_sources src/main.c)

# Include Warp Pipe CMakeLists
include(../../CMakeLists.txt)

target_sources(app PRIVATE ${app_sources} ${warp_pipe_sources})
target_include_directories(app PRIVATE ${warp_pipe_include})
