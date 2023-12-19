cmake_minimum_required(VERSION 3.20)
project("PCIe Mock memory" LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(warp-pipe REQUIRED warp-pipe)
set(warp_pipe_cflags
  -Wall
)

add_executable(memory_mock
  ${CMAKE_SOURCE_DIR}/main.c
)

target_link_libraries(memory_mock
  PRIVATE
    ${warp-pipe_LINK_LIBRARIES}
)

target_include_directories(memory_mock
  PRIVATE
    ${warp-pipe_INCLUDE_DIRS}
)

target_compile_options(memory_mock
  PRIVATE
    ${warp_pipe_cflags}
)
