cmake_minimum_required(VERSION 3.22)
project("PCIe Communication Server" C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)

add_executable(server)
target_compile_options(server PRIVATE -Wall)
target_compile_options(server PRIVATE -Werror)

# include paths
include_directories(inc)

# source files
target_sources(server PRIVATE src/main.c)
target_sources(server PRIVATE src/server.c)
target_sources(server PRIVATE src/client.c)
