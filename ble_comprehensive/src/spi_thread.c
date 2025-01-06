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

#define TEST_STRING "Nordic"
extern struct k_sem sem_spi_txrx;
#define SPI_OP     SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_WORD_SET(8) | SPI_LINES_SINGLE
static struct spi_dt_spec spi_dev0 = SPI_DT_SPEC_GET(DT_NODELABEL(spi_dev_0), SPI_OP, 0);
static struct spi_dt_spec spi_dev1 = SPI_DT_SPEC_GET(DT_NODELABEL(spi_dev_1), SPI_OP, 0);

static int spi_data_exchange(void)
{
    int err;  
	uint8_t tx_data[] = TEST_STRING;
    uint8_t rx_data[7];

	struct spi_buf tx_buf[2] = {
		{
			.buf = tx_data,
			.len = sizeof(tx_data),
		},
		{
			.buf = tx_data,
			.len = sizeof(tx_data)
		}
	};
	struct spi_buf rx_buf = {		
		.buf = rx_data,
		.len = sizeof(tx_data)		
	};  	    
	struct spi_buf_set tx_set = {
		.buffers = tx_buf,
		.count = 1
	};
	struct spi_buf_set rx_set = {
		.buffers = &rx_buf,
		.count = 1
	}; 
	
	err = spi_transceive_dt(&spi_dev0, &tx_set, &rx_set);
    
    if (err)
    {
        LOG_ERR("SPI dev0 transcation failed err %d", err);
		return err;
    }
    else
    {
        LOG_HEXDUMP_INF(rx_set.buffers->buf, rx_set.buffers->len, "Received SPI dev0 data: ");
    }

	tx_set.count = 2;
	err = spi_write_dt(&spi_dev1, &tx_set);    
    if (err)
    {
        LOG_ERR("SPI dev1 write failed err %d", err);
		return err;
    }
    else
    {
        LOG_INF("SPI dev1 write success");
    }

    return 0;

}

void spi_thread(void)
{	
	
	LOG_INF("** SPI master example **");
	LOG_INF("This example is ported from nRF5_SDK\\examples\\peripheral\\spi");
	LOG_INF("The related spis example is from nRF5_SDK\\examples\\peripheral\\spis");
	
	if (!spi_is_ready_dt(&spi_dev0)) {
		LOG_ERR("SPI bus %s dev0 not ready", spi_dev0.bus->name);
		return;
	}

	if (!spi_is_ready_dt(&spi_dev1)) {
		LOG_ERR("SPI bus %s dev1 not ready", spi_dev1.bus->name);
		return;
	}

    k_sem_take(&sem_spi_txrx, K_FOREVER);
	while (1) {                
        LOG_INF("spi master thread");
        spi_data_exchange();
        k_sleep(K_MSEC(5000)); 
	}
}

K_THREAD_DEFINE(spi_thread_id, 1024, spi_thread, NULL, NULL,
		NULL, 7, 0, 0);