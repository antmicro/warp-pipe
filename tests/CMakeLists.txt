# required for GTest
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  fff
  GIT_REPOSITORY https://github.com/meekrosoft/fff.git
  # We depend on the CUSTOM_FFF_FUNCTION_TEMPLATE being present in FFF. This is
  # not the case in the latest v1.1 release, so we're directly pointing to a
  # sha of a recent commit instead.
  GIT_TAG 5111c61e1ef7848e3afd3550044a8cf4405f4199
)
FetchContent_MakeAvailable(fff)

# sources of the test suite
set(warp_pipe_test_src
  ${CMAKE_SOURCE_DIR}/tests/common.cc
  ${CMAKE_SOURCE_DIR}/tests/test_client.cc
  ${CMAKE_SOURCE_DIR}/tests/test_crc.cc
  ${CMAKE_SOURCE_DIR}/tests/test_server.cc
)

set(warp_pipe_test_cflags
  -fsanitize=address
)

set(warp_pipe_cov_cflags
  -fprofile-arcs
  -ftest-coverage
)

add_executable(warppipe-tests
  ${warp_pipe_test_src}
)

# Add special library with coverage options enabled
# This allows to enable coverage, but don't require gcov on runtime
get_target_property(warppipe_SOURCES warppipe SOURCES)
get_target_property(warppipe_INCLUDES warppipe INCLUDE_DIRECTORIES)
add_library(warppipe-test_static STATIC
  ${warppipe_SOURCES}
)

target_include_directories(warppipe-test_static
  PUBLIC
    ${warppipe_INCLUDES}
)

target_compile_options(warppipe-test_static
  PRIVATE
    ${warp_pipe_cov_cflags}
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTEST_MAIN REQUIRED gtest_main)

target_link_libraries(warppipe-tests
  PRIVATE
    gcov
    warppipe-test_static
    ${GTEST_MAIN_LIBRARIES}
)

target_include_directories(warppipe-tests
  PRIVATE
    ${fff_SOURCE_DIR}
    ${GTEST_MAIN_INCLUDE_DIRS}
)

target_compile_options(warppipe-tests
  PRIVATE
    ${warp_pipe_cflags}
    ${warp_pipe_test_cflags}
    ${warp_pipe_cov_cflags}
    ${GTEST_MAIN_CFLAGS_OTHER}
)

set_target_properties(warppipe-tests PROPERTIES LINK_FLAGS "--coverage -fsanitize=address")
