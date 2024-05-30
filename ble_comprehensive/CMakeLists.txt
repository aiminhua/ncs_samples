# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

# set(mcuboot_DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/child_image/mcuboot.overlay)

SET(hci_rpmsg_b0n_OVERLAY_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/child_image/b0n.conf")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(comprensive_example)

# NORDIC SDK APP START

target_sources(app PRIVATE src/main.c)

target_sources_ifdef(CONFIG_EXAMPLE_I2C app
                     PRIVATE src/i2c_thread.c)
					 
target_sources_ifdef(CONFIG_EXAMPLE_SPIM app
                     PRIVATE src/spi_thread.c)
					 
target_sources_ifdef(CONFIG_EXAMPLE_HS_UART app
                     PRIVATE src/uart_thread.c)
					 
target_sources_ifdef(CONFIG_EXAMPLE_ADC app
                     PRIVATE src/adc_thread.c)

target_sources_ifdef(CONFIG_EXAMPLE_EXT_INT app
                     PRIVATE src/io_int_thread.c)                     
					 
target_sources_ifdef(CONFIG_EXAMPLE_FLASH_SETTINGS app
                     PRIVATE src/settings_thread.c)

target_sources_ifdef(CONFIG_EXAMPLE_FLASH_NVS app
                     PRIVATE src/nvs_thread.c)

zephyr_library_include_directories(.)

# NORDIC SDK APP END