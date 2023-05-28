/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>
#include <hal/nrf_saadc.h>

LOG_MODULE_REGISTER(const_var, 3);


const uint32_t xip_const[4] = {0x11, 0x22, 0x33, 0x44};

uint32_t get_xip_var(uint8_t i)
{
    return xip_const[i%4];
}
