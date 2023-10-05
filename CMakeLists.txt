cmake_minimum_required(VERSION 3.22)
project("PCIe Communication Server" C)

set(CMAKE_C_STANDARD 11)

add_executable(server)
target_compile_options(server PRIVATE -Wall)
target_sources(server PRIVATE src/main.c)
