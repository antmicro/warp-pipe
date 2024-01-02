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

set(warp_pipe_sources
  ${CMAKE_CURRENT_LIST_DIR}/src/server.c
  ${CMAKE_CURRENT_LIST_DIR}/src/client.c
  ${CMAKE_CURRENT_LIST_DIR}/src/crc.c
  ${CMAKE_CURRENT_LIST_DIR}/src/proto.c
)

set(warp_pipe_include ${CMAKE_CURRENT_LIST_DIR}/inc)

if (NOT DEFINED ZEPHYR_BASE)
  cmake_minimum_required(VERSION 3.20)
  project(warp-pipe LANGUAGES C CXX)

  set(CMAKE_C_STANDARD 11)
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
  endif()
  option(CMAKE_EXPORT_COMPILE_COMMANDS "Export compile-commands.json" ON)
  option(ENABLE_TESTS "Build tests" ON)

  find_program(LCOV lcov REQUIRED)

  set(warp_pipe_cflags
    -Wall
    -Werror
  )

  set(PACKAGE_SOVERSION 0)
  set(PACKAGE_VERSION ${PACKAGE_SOVERSION}.0.0)

  add_library(warppipe SHARED
    ${warp_pipe_sources}
  )

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND warp_pipe_cflags
      -fprofile-arcs
      -ftest-coverage
    )
    target_link_libraries(warppipe
      PRIVATE
        gcov
    )
  endif()

  target_include_directories(warppipe
    PUBLIC
      ${warp_pipe_include}
  )

  target_compile_options(warppipe
    PRIVATE
      ${warp_pipe_cflags}
  )

  set_target_properties(warppipe PROPERTIES
    VERSION ${PACKAGE_VERSION}
    SOVERSION ${PACKAGE_SOVERSION}
  )

  include(GNUInstallDirs)
  set(includedir ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
  set(libdir ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
  set(prefix ${CMAKE_INSTALL_PREFIX})
  configure_file(warp-pipe.pc.in ${CMAKE_CURRENT_BINARY_DIR}/warp-pipe.pc @ONLY)

  install(DIRECTORY inc/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/warp-pipe.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  )
  install(FILES
    LICENSE
    README.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
  )
  install(TARGETS warppipe LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

  if (${ENABLE_TESTS})
      include(tests/CMakeLists.txt)
  endif()

elseif(CONFIG_DMA_EMUL)
  add_subdirectory(zephyr-samples)
endif()
