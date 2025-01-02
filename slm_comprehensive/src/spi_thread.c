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
#include <zephyr/drivers/spi.h>

#define LOG_MODULE_NAME spi_thread
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define DELAY_SPI_CS_ACTIVE_US 2
#define TEST_STRING "Nordic"

const struct device *spi_dev;

static int spi_data_exchange(void)
{
    int err;
    struct spi_config spi_cfg = {0};

	spi_cfg.frequency = 8000000U;
	spi_cfg.operation = SPI_WORD_SET(8);

	spi_cfg.cs.gpio.port = DEVICE_DT_GET(DT_GPIO_CTLR(DT_ALIAS(myspi), cs_gpios));
	if (!spi_cfg.cs.gpio.port) {
        LOG_ERR("cannot find CS GPIO device");
		return -ENODEV;
	}

	spi_cfg.cs.gpio.pin = DT_GPIO_PIN(DT_ALIAS(myspi), cs_gpios);
	spi_cfg.cs.gpio.dt_flags = DT_GPIO_FLAGS(DT_ALIAS(myspi), cs_gpios);

	spi_cfg.cs.delay = DELAY_SPI_CS_ACTIVE_US;

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
	
	spi_dev = DEVICE_DT_GET(DT_ALIAS(myspi));
	if (!spi_dev) {
		LOG_ERR("SPIM driver not found.\n");
		return;
	}

	while (1) {                
        LOG_INF("spi master thread");
        spi_data_exchange();
        k_sleep(K_MSEC(2000)); 
	}
}

K_THREAD_DEFINE(spi_thread_id, 1024, spi_thread, NULL, NULL,
		NULL, 7, 0, 0);