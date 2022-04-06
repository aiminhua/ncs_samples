/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#define LOG_MODULE_NAME main
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define INT_LEVEL_MODE 0
//CONFIG_GPIO_NRF_INT_EDGE_USING_SENSE

const static struct device *gpio_dev;
#define INT0_NODE DT_NODELABEL(button3)
#define INT0	DT_GPIO_LABEL(INT0_NODE, gpios)
#define INT0_PIN	DT_GPIO_PIN(INT0_NODE, gpios)
#define INT0_FLAGS	DT_GPIO_FLAGS(INT0_NODE, gpios)

static struct gpio_callback ext_int_cb_data;

void ext_int_isr(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	LOG_INF("external interrupt occurs at %d", k_uptime_get_32());

#if INT_LEVEL_MODE			
    gpio_pin_interrupt_configure(gpio_dev,  INT0_PIN,  GPIO_INT_DISABLE);
#endif	
  
}

void ext_int_init(void)
{
	int ret;

	gpio_dev = device_get_binding(INT0);
	if (!gpio_dev) {
		LOG_ERR("INT0 dev null");		
	}

	ret = gpio_pin_configure(gpio_dev, INT0_PIN, (GPIO_INPUT | GPIO_PULL_UP));
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure pin %d",
		       ret, INT0_PIN);		
	}

#if INT_LEVEL_MODE					   
	ret = gpio_pin_interrupt_configure(gpio_dev,
					   INT0_PIN,
					   GPIO_INT_LEVEL_LOW);	 	
#else
	ret = gpio_pin_interrupt_configure(gpio_dev,
					   INT0_PIN,
					   GPIO_INT_EDGE_FALLING);  
#endif

	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on pin %d",
			ret, INT0_PIN);		
	}

	gpio_init_callback(&ext_int_cb_data, ext_int_isr, BIT(INT0_PIN));
	gpio_add_callback(gpio_dev, &ext_int_cb_data);
	LOG_INF("External interrupt example at Pin:%d", INT0_PIN);
}

void main(void)
{	
	LOG_INF("** External interrupt example **");

    ext_int_init();
	while (1) {		
		LOG_INF("main thread");
        k_sleep(K_SECONDS(5));
#if INT_LEVEL_MODE		   
		gpio_pin_interrupt_configure(gpio_dev,
					   INT0_PIN,
					   GPIO_INT_LEVEL_LOW);
#endif					   	    
	}
}
