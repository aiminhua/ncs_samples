/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <stdio.h>
#include "rpc_app_nus.h"
#include "nrf_rpc_tr.h"
#include <logging/log.h>
#include <drivers/uart.h>

#define LOG_MODULE_NAME rpc_thread2
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

extern struct k_sem sem_rpc_tx;

#define BUFFER_LENGTH 64

static uint8_t buffer[BUFFER_LENGTH];

static bool m_is_connected;
bool get_ble_connection_status(void)
{
	return m_is_connected;
}

void set_ble_connection_status(bool is_connected)
{
	m_is_connected = is_connected;
}


#ifdef CONFIG_EXAMPLE_HS_UART
extern int my_uart_send(const uint8_t *buf, size_t len);
#endif
/*
* format: 0x55 CMD LEN Payload 0xAA (Checksum)
* 0x55 0x01 0x01 0x00 0xAA  //connected
* 0x55 0x01 0x01 0x01 0xAA  //disconnected
*/
/* Callback from transport layer that handles incoming. */
static void rpc_receive_handler(const uint8_t *packet, size_t len)
{
	
	LOG_INF("received data from net core by RPC");
	LOG_HEXDUMP_INF(packet, len, "packet:");

#ifdef CONFIG_EXAMPLE_HS_UART
	my_uart_send(packet, len);
#endif
	if(packet[0] == 0x55)
	{
		if (packet[1] == 0x01)
		{
			if (packet[2] == 0x01)
			{
				set_ble_connection_status(packet[3] == 0);
			}
		}		
	}
	if (!NRF_RPC_TR_AUTO_FREE_RX_BUF) {
		nrf_rpc_tr_free_rx_buf(packet);
	}
}


static void rpc_init(void)
{
	int err = nrf_rpc_tr_init(rpc_receive_handler);
	if (err < 0) {
		LOG_ERR("rpc tr init error %d \n", err);
		return;
	}	

}

static void rpc_tx_app(void)
{
	static uint8_t cnt;
	int err;

	snprintf(buffer, 12, "HelloApp%d", cnt++);		
	buffer[11] = 0;		
	err = nrf_rpc_tr_send(buffer, 12);
	if (err) {
		LOG_ERR("nrf_rpc_tr_send failed: %d\n", err);			
	}
	else
	{
		LOG_HEXDUMP_INF(buffer, 12, "Sent by app core:");
	}

}

void rpc_thread2(void)
{
	LOG_INF("**dual core interactions by RPC raw API");
    rpc_init();

	while (1) {        
		k_sem_take(&sem_rpc_tx, K_FOREVER);	
		LOG_INF("RPC thread\n");	
		rpc_tx_app();
		k_sleep(K_SECONDS(2));
	}
}

K_THREAD_DEFINE(rpc_thread_id2, 1024, rpc_thread2, NULL, NULL,
		NULL, 6, 0, 0);