/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME extint_thread
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

//Correspondent to Button4. Change it as per your board's definition
#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)
const struct gpio_dt_spec ext_int =
        GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, extint_gpios);

K_SEM_DEFINE(sem_i2c_op, 0, 1);

static struct gpio_callback ext_int_cb_data;


void ext_int_isr(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	LOG_INF("External interrupt occurs on pin 0x%x at 0x%x", pins, k_uptime_get());   
    k_sem_give(&sem_i2c_op);    
}

void config_io_interrupt(void)
{
	int ret;

	if (!gpio_is_ready_dt(&ext_int)) {
		LOG_ERR("Error: GPIO device %s is not ready",
		       ext_int.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&ext_int, GPIO_INPUT | GPIO_PULL_UP);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure pin %d",
		       ret, ext_int.pin);		
	}

	ret = gpio_pin_interrupt_configure_dt(&ext_int, GPIO_INT_EDGE_TO_INACTIVE);	                       
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d",
			ret, ext_int.port->name, ext_int.pin);		
	}

	gpio_init_callback(&ext_int_cb_data, ext_int_isr, BIT(ext_int.pin));
	gpio_add_callback(ext_int.port, &ext_int_cb_data);

	LOG_INF("External interrupt set on %s pin %d",ext_int.port->name, ext_int.pin);
}

void io_thread(void)
{	
	LOG_INF("** External interrupt example **");

    config_io_interrupt();   

	while (1) {              
        LOG_INF("external interrupt thread");
        k_sleep(K_SECONDS(20));
	}
}

K_THREAD_DEFINE(io_thread_id, 512, io_thread, NULL, NULL,
		NULL, 7, 0, 0);