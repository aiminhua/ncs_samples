#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

# NORDIC SDK APP START

target_sources(app PRIVATE
  src/main.c src/uart_hs.c
)

target_sources(app PRIVATE src/rpc_app_api_general.c src/rpc_app_api.c)

# NORDIC SDK APP END

zephyr_library_include_directories(.)

