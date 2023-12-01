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

if (NOT DEFINED ZEPHYR_BASE)
  cmake_minimum_required(VERSION 3.22)
  project("PCIe Communication Server" LANGUAGES C CXX)

  set(CMAKE_C_STANDARD 11)
  set(CMAKE_BUILD_TYPE Debug)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  # required for GTest
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  FetchContent_Declare(
    fff
    GIT_REPOSITORY https://github.com/meekrosoft/fff.git
    # We depend on the CUSTOM_FFF_FUNCTION_TEMPLATE being present in FFF. This is
    # not the case in the latest v1.1 release, so we're directly pointing to a
    # sha of a recent commit instead.
    GIT_TAG 5111c61e1ef7848e3afd3550044a8cf4405f4199
  )
  FetchContent_MakeAvailable(googletest fff)

  # find dependencies
  find_program(LCOV lcov REQUIRED)
  find_program(GENHTML genhtml REQUIRED)

  # include paths
  include_directories(
    ${CMAKE_SOURCE_DIR}/inc
    ${fff_SOURCE_DIR}
  )

  # shared sources between the server and test suite
  set(pcie_comm_src
    ${CMAKE_SOURCE_DIR}/src/server.c
    ${CMAKE_SOURCE_DIR}/src/client.c
    ${CMAKE_SOURCE_DIR}/src/crc.c
    ${CMAKE_SOURCE_DIR}/src/proto.c
  )

  # sources of the test suite
  set(pcie_comm_test_src
    ${CMAKE_SOURCE_DIR}/tests/common.cc
    ${CMAKE_SOURCE_DIR}/tests/test_client.cc
    ${CMAKE_SOURCE_DIR}/tests/test_crc.cc
    ${CMAKE_SOURCE_DIR}/tests/test_server.cc
  )

  set(pcie_comm_cflags
    -Wall
    -Werror
  )

  set(pcie_comm_test_cflags
    -fprofile-arcs
    -ftest-coverage
    -fsanitize=address
  )

  add_executable(server
    ${CMAKE_SOURCE_DIR}/src/main.c
    ${pcie_comm_src}
  )

  target_compile_options(server
    PRIVATE
      ${pcie_comm_cflags}
  )

  set(PACKAGE_SOVERSION 0)
  set(PACKAGE_VERSION ${PACKAGE_SOVERSION}.0.0)

  add_library(pcied SHARED
    ${pcie_comm_src}
  )
  target_compile_options(pcied
    PRIVATE
      ${pcie_comm_cflags}
  )
  set_target_properties(pcied PROPERTIES
    VERSION ${PACKAGE_VERSION}
    SOVERSION ${PACKAGE_SOVERSION}
  )

  include(GNUInstallDirs)
  set(includedir ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
  set(libdir ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
  set(prefix ${CMAKE_INSTALL_PREFIX})
  configure_file(pcied.pc.in ${CMAKE_CURRENT_BINARY_DIR}/pcied.pc @ONLY)

  install(DIRECTORY inc/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/pcied.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  )
  install(FILES
    LICENSE
    README.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
  )
  install(TARGETS pcied LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

  add_executable(server-tests
    ${pcie_comm_src}
    ${pcie_comm_test_src}
  )

  target_link_libraries(server-tests
    PRIVATE
      gcov
      gtest_main
  )

  target_compile_options(server-tests
    PRIVATE
      ${pcie_comm_cflags}
      ${pcie_comm_test_cflags}
  )

  set_target_properties(server-tests PROPERTIES LINK_FLAGS "--coverage -fsanitize=address")
elseif(CONFIG_DMA_EMUL)
  add_subdirectory(zephyr-samples)
endif()
