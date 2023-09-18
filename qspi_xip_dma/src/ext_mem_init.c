/*
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/drivers/pinctrl.h>
#include <nrfx_qspi.h>
#include <hal/nrf_clock.h>
#include <zephyr/drivers/flash/nrf_qspi_nor.h>


int qspi_xip_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	nrf_qspi_nor_xip_enable(DEVICE_DT_GET(DT_INST(0, nordic_qspi_nor)), false);

	nrf_qspi_nor_xip_enable(DEVICE_DT_GET(DT_INST(0, nordic_qspi_nor)), true);

    printk("in QSPI XIP mode\r\n");
	return 0;
}

SYS_INIT(qspi_xip_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
