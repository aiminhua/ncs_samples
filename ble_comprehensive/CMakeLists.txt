# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ble_comprehensive)

target_sources(app PRIVATE src/main.c)

target_sources_ifdef(CONFIG_SAMPLE_I2C app
                     PRIVATE src/i2c_thread.c)

target_sources_ifdef(CONFIG_SAMPLE_SPIM app
                     PRIVATE src/spi_thread.c)

target_sources_ifdef(CONFIG_SAMPLE_HS_UART app
                     PRIVATE src/uart_thread.c)

target_sources_ifdef(CONFIG_SAMPLE_ADC app
                     PRIVATE src/adc_thread.c)

target_sources_ifdef(CONFIG_SAMPLE_EXT_INT app
                     PRIVATE src/io_int_thread.c)

target_sources_ifdef(CONFIG_SAMPLE_FLASH_SETTINGS app
                     PRIVATE src/settings_thread.c)

target_sources_ifdef(CONFIG_SAMPLE_FLASH_NVS app
                     PRIVATE src/nvs_thread.c)

if (CONFIG_DTM_IN_APP)
    add_subdirectory(src/dtm)
endif()

zephyr_library_include_directories(.)
