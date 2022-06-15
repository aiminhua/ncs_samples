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
#include "rpc_net_api.h"
#include "nrf_rpc_tr.h"
#include <logging/log.h>
#include <drivers/uart.h>
#include "common_ids.h"
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#define LOG_MODULE_NAME rpc_net_example
LOG_MODULE_REGISTER(LOG_MODULE_NAME);


int net2app_send_bt_addr(void)
{

	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	size_t count = ARRAY_SIZE(addrs);
	char addr_s[BT_ADDR_LE_STR_LEN];

	bt_id_get(addrs, &count);

	bt_addr_le_to_str(&addrs[0], addr_s, sizeof(addr_s));

	printk("===BT dev addr: %s ====\n", addr_s);

	return rpc_net2app_send(NET2APP_BT_ADDR_SEND, (uint8_t *)&addrs[0], 7);
	 
}

int net2app_send_nus(uint8_t *data, uint16_t len)
{
	return rpc_net2app_send(NET2APP_BT_NUS_RECV, data, len);
}

int net2app_send_conn_status(uint8_t connected)
{
	return rpc_net2app_send(NET2APP_BT_CONN_STATUS, (uint8_t *)&connected, 1);
}