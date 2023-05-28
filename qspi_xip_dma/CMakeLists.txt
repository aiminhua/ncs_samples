# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)

set_property(GLOBAL 
    APPEND 
    PROPERTY extra_post_build_commands    
    COMMAND ${CMAKE_OBJCOPY} --output-target=binary --remove-section=_EXTFLASH_TEXT_SECTION_NAME --remove-section=_EXTFLASH_RODATA_SECTION_NAME --remove-section=.comment --remove-section=COMMON --remove-section=.eh_frame zephyr.elf zephyr.bin
    COMMAND ${CMAKE_OBJCOPY} --output-target=ihex --remove-section=_EXTFLASH_TEXT_SECTION_NAME --remove-section=_EXTFLASH_RODATA_SECTION_NAME --remove-section=.comment --remove-section=COMMON --remove-section=.eh_frame zephyr.elf zephyr.hex    
    COMMAND ${CMAKE_OBJCOPY} --output-target=binary --only-section=_EXTFLASH_TEXT_SECTION_NAME --only-section=_EXTFLASH_RODATA_SECTION_NAME zephyr.elf xip.bin
    COMMAND ${CMAKE_OBJCOPY} --output-target=ihex --only-section=_EXTFLASH_TEXT_SECTION_NAME --only-section=_EXTFLASH_RODATA_SECTION_NAME zephyr.elf xip.hex
    COMMAND python $ENV{ZEPHYR_BASE}/../bootloader/mcuboot/scripts/imgtool.py sign --key $ENV{ZEPHYR_BASE}/../bootloader/mcuboot/root-ec-p256.pem --header-size 0x200 --align 4 --version 0.0.0+0 --pad-header --slot-size 0x100000 xip.bin xip_signed.bin
    COMMAND ${CMAKE_OBJCOPY} --input-target=binary --output-target=ihex --change-address 0x10000000 --gap-fill=0xff xip_signed.bin xip_signed.hex
    COMMAND ${CMAKE_OBJCOPY} --input-target=binary --output-target=ihex --change-address 0x10230000 --gap-fill=0xff xip_signed.bin xip_moved_test_update.hex
    COMMAND echo "####### XIP hex generation #######"
)

SET(hci_rpmsg_b0n_OVERLAY_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/child_image/b0n.conf")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(xip_dma)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Run adc_thread from the external flash (XIP). No need to copy.
# zephyr_code_relocate(src/adc_thread.c EXTFLASH_TEXT NOCOPY)

# But still relocate (copy) the data to RAM
# zephyr_code_relocate(src/adc_thread.c RAM_DATA)

zephyr_code_relocate(FILES src/adc_thread.c LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(FILES src/adc_thread.c LOCATION RAM_DATA)

zephyr_code_relocate(FILES src/const_var.c LOCATION EXTFLASH NOCOPY)
zephyr_code_relocate(FILES src/const_var.c LOCATION RAM_DATA)