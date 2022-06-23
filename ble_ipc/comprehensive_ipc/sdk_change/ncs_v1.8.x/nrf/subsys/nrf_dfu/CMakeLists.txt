#
# Copyright (c) 2019-2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(inc)
zephyr_library()

zephyr_library_sources_ifdef(CONFIG_NRF_DFU_BT nrf_dfu_ble.c)
zephyr_library_sources_ifdef(CONFIG_NRF_DFU_UART nrf_dfu_serial.c nrf_dfu_serial_uart.c slip.c)
add_subdirectory_ifdef(CONFIG_NRF_DFU common)
zephyr_library_sources_ifdef(CONFIG_NRF_DFU_RPC_APP adapt_rpc_app.c)
zephyr_library_sources_ifdef(CONFIG_NRF_DFU_RPC_NET adapt_rpc_net.c)
zephyr_library_sources_ifdef(CONFIG_NRF_DFU_RPC_NET common/nrf_dfu_handling_error.c)
