/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <hal/nrf_gpio.h>
#include "transport/dtm_transport.h"

#define DTM_INIT_PRIORITY 25 //(CONFIG_CONSOLE_INIT_PRIORITY + 4)

bool b_dtm_mode;

int dtm_main(void)
{
	int err;
	union dtm_tr_packet cmd;
	uint32_t i;

	__set_PSP(0x2000ce00);

	//read GPIO1.08 (Button2) to determine if DTM mode is enabled
	nrf_gpio_cfg_input(40, NRF_GPIO_PIN_PULLUP); // Configure GPIO pin 40 (Button2) as input with pull-up resistor
	if (nrf_gpio_pin_read(40) == 1) {
		printk("DTM mode not enabled by Button2\n");
		return 0; // Exit if DTM mode is not enabled
	}

	//delay for 100ms to allow the system to stabilize
	i = 0;
	while (i < 100000) {
		i++;
	}

	if (nrf_gpio_pin_read(40) == 0) {
		printk("DTM mode enabled by Button2\n");
	} else {
		printk("DTM mode not enabled by Button2\n");
		return 0; // Exit if DTM mode is not enabled
	}

	printk("Starting Direct Test Mode sample\n");

	err = dtm_tr_init();
	if (err) {
		printk("Error initializing DTM transport: %d\n", err);
		return err;
	}

	b_dtm_mode = true;

	for (;;) {
		cmd = dtm_tr_get();
		err = dtm_tr_process(cmd);
		if (err) {
			printk("Error processing command: %d\n", err);
			return err;
		}
	}
}

SYS_INIT(dtm_main, PRE_KERNEL_1, DTM_INIT_PRIORITY);