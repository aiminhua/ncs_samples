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

LOG_MODULE_REGISTER(adc_thread, 3);

#define NUM_OF_CH 2
#define ADC_DEVICE_NAME		DT_NODE_FULL_NAME(DT_NODELABEL(adc))
#define ADC_RESOLUTION		12
#define ADC_OVERSAMPLING	4 /* 2^ADC_OVERSAMPLING samples are averaged */
#define ADC_MAX 		4096
#define BATTERY_MEAS_VOLTAGE_GAIN	6
#define ADC_GAIN		ADC_GAIN_1_6
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_REF_INTERNAL_MV	600UL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_CHANNEL_ID		0
#define ADC_CHANNEL_INPUT	NRF_SAADC_INPUT_VDD
#define ADC_CHANNEL_ID2		1
#define ADC_CHANNEL_INPUT2	NRF_SAADC_INPUT_AIN1   //P0.05
#define ADC_SAMPLE_INTERVAL	20  //unit:s

#define BATTERY_VOLTAGE(sample) (sample * BATTERY_MEAS_VOLTAGE_GAIN	\
				 * ADC_REF_INTERNAL_MV / ADC_MAX)


static const struct device *adc_dev;
static int16_t adc_buffer[NUM_OF_CH];

static struct adc_sequence sequence = {
	.options	= NULL,
	.channels	= BIT(ADC_CHANNEL_ID),
	.buffer		= adc_buffer,
	.buffer_size	= sizeof(adc_buffer),
	.resolution	= ADC_RESOLUTION,
	.oversampling	= ADC_OVERSAMPLING,
	.calibrate	= false,
};
static struct adc_sequence sequence_calibrate = {
	.options	= NULL,
	.channels	= BIT(ADC_CHANNEL_ID),
	.buffer		= adc_buffer,
	.buffer_size	= sizeof(adc_buffer),
	.resolution	= ADC_RESOLUTION,
	.oversampling	= ADC_OVERSAMPLING,
	.calibrate	= true,
};

static bool calibrated;

void adc_sample_sync(void)
{
	int err;

	if (NUM_OF_CH == 2)
	{
		sequence.channels = BIT(ADC_CHANNEL_ID) | BIT(ADC_CHANNEL_ID2);
		sequence_calibrate.channels = BIT(ADC_CHANNEL_ID) | BIT(ADC_CHANNEL_ID2);
		sequence.oversampling  = NRF_SAADC_OVERSAMPLE_DISABLED;
		sequence_calibrate.oversampling  = NRF_SAADC_OVERSAMPLE_DISABLED;
	}
	if (likely(calibrated)) {		
		err = adc_read(adc_dev, &sequence);
	} else {
		err = adc_read(adc_dev, &sequence_calibrate);
		calibrated = true;
	}
	if (err) {
		LOG_WRN("ADC read err %d", err);
	} else {
		for (int i = 0; i < NUM_OF_CH; i++)
		{
			uint32_t voltage = BATTERY_VOLTAGE(adc_buffer[i]);
			LOG_INF("Voltage%d: %u mV / %d", i, voltage, adc_buffer[i]);
		}
	}			
}

int init_adc(void)
{
	int err;

	adc_dev = device_get_binding(ADC_DEVICE_NAME);
	if (!adc_dev) {
		LOG_ERR("Cannot get ADC device");
		return -ENXIO;
	}

	struct adc_channel_cfg channel_cfg = {
		.gain             = ADC_GAIN,
		.reference        = ADC_REFERENCE,
		.acquisition_time = ADC_ACQUISITION_TIME,
		.channel_id       = ADC_CHANNEL_ID,
		.differential	  = 0,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
		.input_positive   = ADC_CHANNEL_INPUT,
#endif
	};
	err = adc_channel_setup(adc_dev, &channel_cfg);
	if (err) {
		LOG_ERR("Setting up the ADC channel%d failed", ADC_CHANNEL_ID);
		return err;
	}

	if (NUM_OF_CH == 2)
	{
		channel_cfg.channel_id = ADC_CHANNEL_ID2;
		channel_cfg.input_positive = ADC_CHANNEL_INPUT2;
		err = adc_channel_setup(adc_dev, &channel_cfg);
		if (err) {
			LOG_ERR("Setting up the ADC channel%d failed", ADC_CHANNEL_ID2);
			return err;
		}		
	}

	return 0;
}


void adc_thread(void)
{	
	
	LOG_INF("**ADC sampling example");
	init_adc();

	while (1) {                
        LOG_INF("ADC thread");
        adc_sample_sync();
        k_sleep(K_SECONDS(2)); 
	}
}

K_THREAD_DEFINE(adc_thread_id, 1024, adc_thread, NULL, NULL,
		NULL, 9, 0, 0);