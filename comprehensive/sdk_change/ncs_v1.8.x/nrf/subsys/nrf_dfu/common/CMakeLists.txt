#
# Copyright (c) 2019-2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_sources(crc32.c)
zephyr_library_sources(nrf_dfu.c)
zephyr_library_sources(nrf_dfu_flash.c)
zephyr_library_sources(nrf_dfu_handling_error.c)
zephyr_library_sources(nrf_dfu_req_handler.c)
zephyr_library_sources(nrf_dfu_settings.c)
zephyr_library_sources(nrf_dfu_transport.c)
zephyr_library_sources(nrf_dfu_validation.c)
zephyr_library_sources(dfu-cc.pb.c)
zephyr_library_sources(pb_common.c)
zephyr_library_sources(pb_decode.c)
zephyr_linker_sources(SECTIONS dfu_trans.ld)
