# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

# Use static partition layout to ensure consistency between builds.
# This is to ensure settings storage will be at the same location after the DFU.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pm_static_external_flash.yml")
  set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_SOURCE_DIR}/pm_static_external_flash.yml)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(comprensive_example)

# NORDIC SDK APP START

target_sources(app PRIVATE src/main.c)

target_sources_ifdef(CONFIG_EXAMPLE_IIC app
                     PRIVATE src/iic_thread.c)
					 
target_sources_ifdef(CONFIG_EXAMPLE_SPIM app
                     PRIVATE src/spi_thread.c)
					 
target_sources_ifdef(CONFIG_EXAMPLE_HS_UART app
                     PRIVATE src/uart_thread.c)
					 
target_sources_ifdef(CONFIG_EXAMPLE_ADC app
                     PRIVATE src/adc_thread.c)					 

if (CONFIG_EXAMPLE_RPC_IN_USE AND CONFIG_RPC_REMOTE_API)
  target_sources(app PRIVATE src/rpc_thread1.c)
endif()

if (CONFIG_EXAMPLE_RPC_IN_USE AND CONFIG_RPC_SIMULATE_UART)
  target_sources(app PRIVATE src/rpc_thread2.c)
endif()                 
					 
target_sources_ifdef(CONFIG_RPC_REMOTE_API app
                     PRIVATE src/rpc_app_api_general.c src/rpc_app_api.c)
					 
target_sources_ifdef(CONFIG_RPC_NUS_DEDICATE app
                     PRIVATE src/rpc_app_nus.c)						 

target_sources_ifdef(CONFIG_EXAMPLE_DFU_OTA app
                     PRIVATE src/rpc_app_smp.c)		

target_sources_ifdef(CONFIG_EXAMPLE_FLASH app
                     PRIVATE src/flash_thread.c)

target_sources_ifdef(CONFIG_EXAMPLE_RAW_NRFX app
                      PRIVATE src/raw_nrfx_driver_thread.c)


zephyr_library_include_directories(.)

# NORDIC SDK APP END