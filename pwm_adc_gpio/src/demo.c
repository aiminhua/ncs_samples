/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/nordic-nrf-gpio.h>
#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(demo, CONFIG_LOG_DEFAULT_LEVEL);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

const struct gpio_dt_spec led_pwr =
        GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, led_gpios);

static const struct pwm_dt_spec red_pwm_led =
	PWM_DT_SPEC_GET(DT_ALIAS(red_pwm_led));
static const struct pwm_dt_spec green_pwm_led =
	PWM_DT_SPEC_GET(DT_ALIAS(green_pwm_led));
static const struct pwm_dt_spec blue_pwm_led =
	PWM_DT_SPEC_GET(DT_ALIAS(blue_pwm_led));

#define STEP_SIZE PWM_USEC(2000)

int blink(void)
{
	uint32_t pulse_red, pulse_green, pulse_blue; /* pulse widths */
	int ret;

		for (pulse_red = 0U; pulse_red <= red_pwm_led.period;
			pulse_red += STEP_SIZE) {
			ret = pwm_set_pulse_dt(&red_pwm_led, pulse_red);
			if (ret != 0) {
				printk("Error %d: red write failed\n", ret);
				return 0;
			}
			LOG_INF("Red duty:%d / cycle:%d", pulse_red, red_pwm_led.period);
			k_sleep(K_MSEC(200));
		}
		pwm_set_pulse_dt(&red_pwm_led, 0);

		for (pulse_green = 0U;
				pulse_green <= green_pwm_led.period;
				pulse_green += STEP_SIZE) {
			ret = pwm_set_pulse_dt(&green_pwm_led,
							pulse_green);
			if (ret != 0) {
				printk("Error %d: green write failed\n",
						ret);
				return 0;
			}
			LOG_INF("Green duty: %d / cycle %d", pulse_green, green_pwm_led.period);
			k_sleep(K_MSEC(200));
		}
		pwm_set_pulse_dt(&green_pwm_led, 0);

		for (pulse_blue = 0U;
				pulse_blue <= blue_pwm_led.period;
				pulse_blue += STEP_SIZE) {
			ret = pwm_set_pulse_dt(&blue_pwm_led,
							pulse_blue);
			if (ret != 0) {
				printk("Error %d: "
						"blue write failed\n",
						ret);
				return 0;
			}
			LOG_INF("Blue duty:%d / cycle:%d", pulse_blue, blue_pwm_led.period);
			k_sleep(K_MSEC(200));
		}
		pwm_set_pulse_dt(&blue_pwm_led,	0);	

		return 0;		
		
}

void set_rgb_pulse(uint32_t red, uint32_t green, uint32_t blue)
{
	int ret;

	if (red > red_pwm_led.period)
	{
		red = red_pwm_led.period;
	}

	if (green > green_pwm_led.period)
	{
		green = green_pwm_led.period;
	}

	if (blue > blue_pwm_led.period)
	{
		blue = blue_pwm_led.period;
	}

	ret = pwm_set_pulse_dt(&red_pwm_led, red);
	ret |= pwm_set_pulse_dt(&green_pwm_led,	green);	
	ret |= pwm_set_pulse_dt(&blue_pwm_led,	blue);
	if (ret)
	{
		LOG_ERR("RGB duty setting failed:%d", ret);
	}	

}

int demo_thread(void)
{	
	
	LOG_INF("**ADC sampling example");
	/* Configure the pin */
	gpio_pin_configure_dt(&led_pwr, GPIO_OUTPUT | NRF_GPIO_DRIVE_H0H1);

	/* Set the pin to its active level */
	gpio_pin_set_dt(&led_pwr, 1);

	printk("PWM-based RGB LED control\n");

	if (!pwm_is_ready_dt(&red_pwm_led) ||
	    !pwm_is_ready_dt(&green_pwm_led) ||
	    !pwm_is_ready_dt(&blue_pwm_led)) {
		printk("Error: one or more PWM devices not ready\n");
		return -EIO;
	}

	// while (1) {                
        LOG_INF("demo thread");
		blink();
	// }

	return 0;
}

K_THREAD_DEFINE(demo_thread_id, 1024, demo_thread, NULL, NULL,
		NULL, 7, 0, 0);