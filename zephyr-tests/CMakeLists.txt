# Copyright 2024 Antmicro <www.antmicro.com>
# Copyright 2024 Meta
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
project(tests)

set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
find_package(PkgConfig REQUIRED)
list(APPEND PKG_CONFIG_EXECUTABLE "--static")
pkg_check_modules(warp-pipe REQUIRED warp-pipe)
list(POP_BACK PKG_CONFIG_EXECUTABLE)

set(app_sources src/main.cpp)
target_sources(app PRIVATE ${app_sources})

target_link_libraries(app
  PRIVATE
    ${warp-pipe_LINK_LIBRARIES}
)

target_include_directories(app
  PRIVATE
    ${warp-pipe_INCLUDE_DIRS}
)

target_compile_options(app
  PRIVATE
    ${warp_pipe_CFLAGS}
)
