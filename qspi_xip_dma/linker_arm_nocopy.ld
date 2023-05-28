/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Cortex-M platforms.
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

/*
 * nRF5340dk is shipping a QSPI external flash mapped at 0x1000_0000 that can
 * be used for XIP
 */

MEMORY
{
    EXTFLASH (rwx) : ORIGIN = 0x10000200, LENGTH = 0x100000
}

/*
* SECTIONS {
*  .rodata :
*  {
*    *(.rodata.xip_const)
*  } > EXTFLASH  
*}
*/

/*
* __extflash_rodata_start = ADDR(.rodata.xip_const);
* __extflash_rodata_end = ADDR(.rodata.xip_const) + SIZEOF(.rodata.xip_const);
*
* SECTIONS
* {
*    .rodata.xip_const :
*    {
*        KEEP(*(.rodata.xip_const))
*    } > EXTFLASH
* }
*/

#include <zephyr/arch/arm/aarch32/cortex_m/scripts/linker.ld>
