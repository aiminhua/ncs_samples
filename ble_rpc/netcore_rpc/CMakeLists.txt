#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
cmake_minimum_required(VERSION 3.13.1)


find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
)

target_sources_ifdef(CONFIG_RPC_REMOTE_API app
                     PRIVATE src/rpc_net_api_general.c src/rpc_net_api.c)

target_sources_ifdef(CONFIG_RPC_SMP_BT app
                     PRIVATE src/rpc_net_smp_bt.c)
					 
# NORDIC SDK APP END

zephyr_library_include_directories(.)
