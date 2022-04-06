# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/spm.conf")
  set(spm_CONF_FILE
    prj.conf
    ${CMAKE_CURRENT_LIST_DIR}/spm.conf
  )
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/spm.overlay")
  set(spm_DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/spm.overlay")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c src/blinky.c)

# NORDIC SDK APP END
