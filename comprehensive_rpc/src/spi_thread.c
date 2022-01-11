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
#include <drivers/spi.h>

#define LOG_MODULE_NAME spi_thread
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define SPI_DEVICE_NAME         "SPI_3"
#define PIN_SPIM_CS	12  //P1.12
#define DELAY_SPI_CS_ACTIVE_US 2
#define TEST_STRING "Nordic"

const struct device *spi_dev;
extern struct k_sem sem_spi_txrx;

static int spi_data_exchange(void)
{
    int err;
    struct spi_cs_control cs_ctrl;
    struct spi_config spi_cfg = {0};

	spi_cfg.frequency = 8000000U;
	spi_cfg.operation = SPI_WORD_SET(8);	

	cs_ctrl.gpio_dev =
		device_get_binding("GPIO_1");
	if (!cs_ctrl.gpio_dev) {
        LOG_ERR("cannot find GPIO_1 device");
		return -ENODEV;
	}

	cs_ctrl.gpio_pin = PIN_SPIM_CS;
	cs_ctrl.gpio_dt_flags = GPIO_ACTIVE_LOW;
	cs_ctrl.delay = DELAY_SPI_CS_ACTIVE_US;

	spi_cfg.cs = &cs_ctrl;

	uint8_t buf[] = TEST_STRING;
    uint8_t data[] = TEST_STRING;

	struct spi_buf spi_buf[2] = {
		{
			.buf = buf,
			.len = sizeof(buf),
		},
		{
			.buf = data,
			.len = sizeof(data)
		}
	};
    
	const struct spi_buf_set tx_set = {
		.buffers = spi_buf,
		.count = 1
	};

	const struct spi_buf_set rx_set = {
		.buffers = spi_buf,
		.count = 1
	};

	// const struct spi_buf_set tx_set = {
	// 	.buffers = spi_buf,
	// 	.count = 2
	// };

	// const struct spi_buf_set rx_set = {
	// 	.buffers = spi_buf,
	// 	.count = 2
	// };    

	err = spi_transceive(spi_dev, &spi_cfg, &tx_set, &rx_set);
    
    if (err)
    {
        LOG_ERR("SPI transcation failed err %d", err);
    }
    else
    {
        LOG_HEXDUMP_INF(rx_set.buffers->buf, rx_set.buffers->len, "Received SPI data: ");
    }

    return err;

}

void spi_thread(void)
{	
	
	LOG_INF("** SPI master example **");
	LOG_INF("This example is ported from nRF5_SDK\\examples\\peripheral\\spi");
	LOG_INF("The related spis example is from nRF5_SDK\\examples\\peripheral\\spis");	

	spi_dev = device_get_binding(SPI_DEVICE_NAME);
	if (!spi_dev) {
		LOG_ERR("SPIM driver not found.\n");
		return;
	}
    k_sem_take(&sem_spi_txrx, K_FOREVER);
	while (1) {                
        LOG_INF("spi master thread");
        spi_data_exchange();
        k_sleep(K_MSEC(200)); 
	}
}

K_THREAD_DEFINE(spi_thread_id, 1024, spi_thread, NULL, NULL,
		NULL, 7, 0, 0);