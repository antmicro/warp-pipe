cmake_minimum_required(VERSION 3.20)
project("PCIe Mock memory" LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(
  ${CMAKE_SOURCE_DIR}/../inc
)

set(warp_pipe_src
  ${CMAKE_SOURCE_DIR}/../src/server.c
  ${CMAKE_SOURCE_DIR}/../src/client.c
  ${CMAKE_SOURCE_DIR}/../src/crc.c
  ${CMAKE_SOURCE_DIR}/../src/proto.c
)

set(warp_pipe_cflags
  -Wall
)

add_executable(memory_mock
  ${CMAKE_SOURCE_DIR}/main.c
  ${warp_pipe_src}
)

target_compile_options(memory_mock
  PRIVATE
    ${warp_pipe_cflags}
)
