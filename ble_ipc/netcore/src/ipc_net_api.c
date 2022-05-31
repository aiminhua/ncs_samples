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
#include <logging/log.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include "ipc_lib.h"

#define LOG_MODULE_NAME ipc_api
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define NET2APP_BT_ADDR_SEND 1
#define NET2APP_BT_NUS_RECV 2
#define NET2APP_BT_CONN_STATUS 3

int net2app_send_bt_addr(void)
{

	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	size_t count = ARRAY_SIZE(addrs);
	char addr_s[BT_ADDR_LE_STR_LEN];
	char data[8];

	bt_id_get(addrs, &count);
	bt_addr_le_to_str(&addrs[0], addr_s, sizeof(addr_s));
	printk("===BT dev addr: %s ====\n", addr_s);

	data[0] = NET2APP_BT_ADDR_SEND;
	memcpy(&data[1], (uint8_t *)&addrs[0], 7);

	return nrfx_ipc_send(data, sizeof(data));
	 
}

int net2app_send_nus(uint8_t *data, uint16_t len)
{
	uint8_t buf[250];

	buf[0] = NET2APP_BT_NUS_RECV;
	memcpy(&buf[1], data, len);

	return nrfx_ipc_send(buf, len+1);	
}

int net2app_send_conn_status(uint8_t connected)
{
	uint8_t data[2];

	data[0] = NET2APP_BT_CONN_STATUS;
	data[1] = connected;

	return nrfx_ipc_send(data, sizeof(data));	
}
