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
#include "rpc_app_api.h"
#include "nrf_rpc_tr.h"
#include <logging/log.h>
#include <drivers/uart.h>
#include "common_ids.h"

#define LOG_MODULE_NAME rpc_app_api
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

uint8_t bt_addr[6];
static bool m_connected = false;

int app2net_send_nus(uint8_t *data, uint16_t len)
{
	return rpc_app2net_send(APP2NET_BT_NUS_SEND, data, len);
}


int net2app_receive_bt_addr(uint8_t *buf, uint16_t len)
{	
	memcpy(bt_addr, &buf[1], 6);
	
	LOG_HEXDUMP_INF(&buf[1], 6, "===bt dev addr: ===");
	LOG_INF("addr type %d", buf[0]);

	return 0;
}

#ifdef CONFIG_EXAMPLE_HS_UART
extern int my_uart_send(const uint8_t *buf, size_t len);
#endif

int net2app_receive_nus(uint8_t *buf, uint16_t len)
{
	LOG_HEXDUMP_INF(buf, len, "===nus data: ===");

#ifdef CONFIG_EXAMPLE_HS_UART
	my_uart_send(buf, len);
#endif	
	return 0;
}

int net2app_receive_conn_status(uint8_t *data, uint16_t len)
{
	if (data[0] == 1)
	{
		m_connected = true;
	}
	else
	{
		m_connected = false;
	}

	return 0;
}

bool is_ble_connected(void)
{
	return m_connected;
}

int net2app_receive(int type, uint8_t *data, size_t len)
{
	int err = 0; 

	switch (type)
	{
		case NET2APP_BT_ADDR_SEND:
			err = net2app_receive_bt_addr(data, len);
			break;
		case NET2APP_BT_NUS_RECV:
			err = net2app_receive_nus(data, len);
			break;
		case NET2APP_BT_CONN_STATUS:
			err = net2app_receive_conn_status(data, len);
			break;			
		default:
			break;
	}

	return err;	
}
