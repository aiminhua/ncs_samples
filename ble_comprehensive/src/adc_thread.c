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
#define ADC_REF_INTERNAL_MV	600UL

#define ADC_SAMPLE_INTERVAL	20  //unit:s

#define BATTERY_VOLTAGE(sample) (sample * BATTERY_MEAS_VOLTAGE_GAIN	\
				 * ADC_REF_INTERNAL_MV / ADC_MAX)


static const struct device *adc_dev;
static int16_t adc_buffer[NUM_OF_CH];
static bool adc_async_read_pending;
static struct k_work_delayable adc_work;

static struct k_poll_signal async_sig = K_POLL_SIGNAL_INITIALIZER(async_sig);
static struct k_poll_event  async_evt =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
				 K_POLL_MODE_NOTIFY_ONLY,
				 &async_sig);

static const struct adc_channel_cfg ch0_cfg_dt =
    ADC_CHANNEL_CFG_DT(DT_CHILD(DT_NODELABEL(adc), channel_0));
static const struct adc_channel_cfg ch1_cfg_dt =
    ADC_CHANNEL_CFG_DT(DT_CHILD(DT_NODELABEL(adc), channel_1));

static struct adc_sequence sequence = {
	.options	= NULL,
	.buffer		= adc_buffer,
	.buffer_size	= sizeof(adc_buffer),
	.resolution	= ADC_RESOLUTION,
	.oversampling	= ADC_OVERSAMPLING,
	.calibrate	= false,
};
static struct adc_sequence sequence_calibrate = {
	.options	= NULL,
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

	sequence.channels = BIT(ch0_cfg_dt.channel_id);
	sequence_calibrate.channels = BIT(ch0_cfg_dt.channel_id);
	if (NUM_OF_CH == 2)
	{
		sequence.channels = BIT(ch0_cfg_dt.channel_id) | BIT(ch1_cfg_dt.channel_id);
		sequence_calibrate.channels = BIT(ch0_cfg_dt.channel_id) | BIT(ch1_cfg_dt.channel_id);
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
#ifdef CONFIG_ADC_ASYNC
static void adc_sample_async(struct k_work *work)
{
	int err;

	if (NUM_OF_CH == 2)
	{
		sequence.channels = BIT(ch0_cfg_dt.channel_id) | BIT(ch1_cfg_dt.channel_id);
		sequence_calibrate.channels = BIT(ch0_cfg_dt.channel_id) | BIT(ch1_cfg_dt.channel_id);
		sequence.oversampling  = NRF_SAADC_OVERSAMPLE_DISABLED;
		sequence_calibrate.oversampling  = NRF_SAADC_OVERSAMPLE_DISABLED;		
	}
	if (!adc_async_read_pending) {
		if (likely(calibrated)) {
			err = adc_read_async(adc_dev, &sequence, &async_sig);
		} else {
			err = adc_read_async(adc_dev, &sequence_calibrate,
					     &async_sig);
			calibrated = true;
		}

		if (err) {
			LOG_WRN("adc_read_async err %d", err);
		} else {
			adc_async_read_pending = true;
		}
	} else {
		err = k_poll(&async_evt, 1, K_NO_WAIT);
		if (err) {
			LOG_WRN("ADC sampling timeout");
		} else {
			adc_async_read_pending = false;			
			for (int i = 0; i < NUM_OF_CH; i++)
			{
				uint32_t voltage = BATTERY_VOLTAGE(adc_buffer[i]);
				LOG_INF("Voltage%d: %u mV / %d async", i, voltage, adc_buffer[i]);
			}			
		}
	}

	k_work_reschedule(&adc_work, K_SECONDS(ADC_SAMPLE_INTERVAL));

}
#endif

static int init_adc(void)
{
	int err;

	adc_dev = device_get_binding(ADC_DEVICE_NAME);
	if (!adc_dev) {
		LOG_ERR("Cannot get ADC device");
		return -EIO;
	}

	err = adc_channel_setup(adc_dev, &ch0_cfg_dt);
	if (NUM_OF_CH == 2)
	{
		err |= adc_channel_setup(adc_dev, &ch1_cfg_dt);	
	}
	if (err) {
		LOG_ERR("ADC channel setup err:%d", err);
		return err;
	}	

#ifdef CONFIG_ADC_ASYNC
	k_work_init_delayable(&adc_work, adc_sample_async);
	k_work_reschedule(&adc_work, K_MSEC(10));
#endif //CONFIG_ADC_ASYNC

	return 0;
}


void adc_thread(void)
{	
	
	LOG_INF("**ADC sampling example");
	init_adc();

	while (1) {                
        LOG_INF("ADC thread");
        adc_sample_sync();
        k_sleep(K_SECONDS(ADC_SAMPLE_INTERVAL)); 
	}
}

K_THREAD_DEFINE(adc_thread_id, 1024, adc_thread, NULL, NULL,
		NULL, 8, 0, 0);