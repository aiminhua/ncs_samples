/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Sample app to demonstrate PWM-based RGB LED control
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/nordic-nrf-gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>
#ifndef CONFIG_BOARD_NRF51DK_NRF51422
#include <hal/nrf_saadc.h>
#endif

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define ADC_DEVICE_NAME		DT_NODE_FULL_NAME(DT_NODELABEL(adc))
#define ADC_SAMPLE_INTERVAL	20  //unit:ms
#define BATTERY_VOLTAGE(sample) (sample * 6	* 600 / 1024)

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)
const struct gpio_dt_spec adc_gnd =
        GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, gnd_gpios);
const struct gpio_dt_spec adc_vdd =
        GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, vdd_gpios);

static const struct adc_channel_cfg ch0_cfg_dt =
    ADC_CHANNEL_CFG_DT(DT_CHILD(DT_NODELABEL(adc), channel_0));
static const struct adc_channel_cfg ch1_cfg_dt =
    ADC_CHANNEL_CFG_DT(DT_CHILD(DT_NODELABEL(adc), channel_1));
	static const struct adc_channel_cfg ch2_cfg_dt =
    ADC_CHANNEL_CFG_DT(DT_CHILD(DT_NODELABEL(adc), channel_2));

static const struct device *adc_dev;
static int16_t adc_buffer[3];

static struct adc_sequence sequence = {
	.options	= NULL,
	.buffer		= adc_buffer,
	.buffer_size	= sizeof(adc_buffer),
	.resolution	= 10,
	.oversampling	= 0,
	.calibrate	= false,
};

// static struct adc_sequence sequence_calibrate = {
// 	.options	= NULL,
// 	.channels	= BIT(ADC_CHANNEL_ID),
// 	.buffer		= adc_buffer,
// 	.buffer_size	= sizeof(adc_buffer),
// 	.resolution	= ADC_RESOLUTION,
// 	.oversampling	= ADC_OVERSAMPLING,
// 	.calibrate	= true,
// };

// static bool calibrated;


static int init_adc(void)
{
	int err;
	/* Configure the pin */
	gpio_pin_configure_dt(&adc_gnd, GPIO_OUTPUT);
	/* Set the pin to its active level */
	gpio_pin_set_dt(&adc_gnd, 1);

	/* Configure the pin */
	gpio_pin_configure_dt(&adc_vdd, GPIO_OUTPUT);
	/* Set the pin to its active level */
	gpio_pin_set_dt(&adc_vdd, 1);	

	adc_dev = device_get_binding(ADC_DEVICE_NAME);
	if (!adc_dev) {
		LOG_ERR("Cannot get ADC device");
		return -EIO;
	}

	err = adc_channel_setup(adc_dev, &ch0_cfg_dt);
	err |= adc_channel_setup(adc_dev, &ch1_cfg_dt);
	err |= adc_channel_setup(adc_dev, &ch2_cfg_dt);
	if (err) {
		LOG_ERR("Setting up the ADC channels failed:%d", err);
		return err;
	}

	return 0;
}

extern void set_rgb_pulse(uint32_t red, uint32_t green, uint32_t blue);

int adc_sample_sync(void)
{
	int err;

	sequence.channels = BIT(ch0_cfg_dt.channel_id) | BIT(ch1_cfg_dt.channel_id) | BIT(ch2_cfg_dt.channel_id);
	err = adc_read(adc_dev, &sequence);
	if (err) {
		LOG_WRN("ADC read err %d", err);
		return err;
	}
	// LOG_INF("R:%d G:%d B:%d", adc_buffer[0], adc_buffer[1], adc_buffer[2]);
	set_rgb_pulse(adc_buffer[0]*20000, adc_buffer[1]*20000, adc_buffer[2]*20000);

	return 0;
		
}

int main(void)
{	
	init_adc();
	k_sleep(K_SECONDS(9));

	while (1) {
        adc_sample_sync();
        k_sleep(K_MSEC(ADC_SAMPLE_INTERVAL)); 
	}
	return 0;
}
