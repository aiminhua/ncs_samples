/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_gpio.h>

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
// static struct gpio_callback ext_int_cb_data2;

void ext_int_isr(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	LOG_INF("external interrupt occurs at %x", pins);

#if INT_LEVEL_MODE			
    gpio_pin_interrupt_configure(gpio_dev,  INT0_PIN,  GPIO_INT_DISABLE);
#endif	
  
}
// void ext_int_isr2(const struct device *dev, struct gpio_callback *cb,
// 		    uint32_t pins)
// {
// 	LOG_INF("external interrupt occurs at %d", pins);

// }

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

#if (INT_LEVEL_MODE == 0)
	ret = gpio_pin_interrupt_configure(gpio_dev,
					   INT0_PIN,
					   GPIO_INT_EDGE_FALLING);	 	
#else
	ret = gpio_pin_interrupt_configure(gpio_dev,
					   INT0_PIN,
					   GPIO_INT_LEVEL_LOW);
#endif

	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on pin %d",
			ret, INT0_PIN);		
	}

	gpio_init_callback(&ext_int_cb_data, ext_int_isr, BIT(INT0_PIN));
	gpio_add_callback(gpio_dev, &ext_int_cb_data);
	LOG_INF("External interrupt example at Pin:%d", INT0_PIN);

	// gpio_dev = device_get_binding("GPIO_1");
	// if (!gpio_dev) {
	// 	LOG_ERR("GPIO_1 dev null");		
	// }

	// ret = gpio_pin_configure(gpio_dev, 9, (GPIO_INPUT | GPIO_PULL_UP));
	// if (ret != 0) {
	// 	LOG_ERR("Error %d: failed to configure pin %d",
	// 	       ret, 41);		
	// }
	// ret = gpio_pin_interrupt_configure(gpio_dev,
	// 				   9,
	// 				   GPIO_INT_EDGE_FALLING);

	// if (ret != 0) {
	// 	LOG_ERR("Error %d: failed to configure interrupt on pin %d",
	// 		ret, 41);		
	// }
	// gpio_init_callback(&ext_int_cb_data2, ext_int_isr2, BIT(9));
	// gpio_add_callback(gpio_dev, &ext_int_cb_data2);

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
