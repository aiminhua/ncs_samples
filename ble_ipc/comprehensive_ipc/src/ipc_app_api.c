/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include "ipc_lib.h"
#include "../../ipc_cmd_ids.h"
#include "ipc_app_api.h"
#include "ipc_app_smp_bt.h"

#define LOG_MODULE_NAME ipc_api
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
static bool connected;
struct ipc_data_t {
	uint8_t *data;
	uint16_t len;
};
static struct ipc_data_t ipc_temp;
static K_SEM_DEFINE(sem_ipc_rx, 0, 1);

static void ipc_rx_handler(uint8_t *data, uint16_t len)
{
	switch (data[0])
	{
		case NET2APP_BT_ADDR_SEND:
			LOG_HEXDUMP_INF(&data[1], len - 1, "Bluetooth device address:");
		break;

		case NET2APP_BT_NUS_RECV:
			LOG_HEXDUMP_INF(&data[1], len - 1, "NUS data:");			
		break;

		case NET2APP_BT_CONN_STATUS:
			connected = data[1];
			LOG_INF("Bluetooth connection status: %d", data[1]);			
		break;

		case NET2APP_BT_SMP_SEND:
			smp_receive_data(&data[1], len - 1);			
		break;	

		case NET2APP_BT_SEND_MTU:
			set_smp_mtu(*((uint16_t *) &data[1]));			
		break;

		case NET2APP_TEST:
			LOG_HEXDUMP_INF(&data[1], len-1, "IPC test:");		
		break;									

		default:
			LOG_ERR("undefined IPC request");
		break;
	}

}

static void ipc_app_rx_cb(uint8_t *data, uint16_t len)
{
	ipc_temp.len = len;
	ipc_temp.data = data;
	k_sem_give(&sem_ipc_rx);
}

bool is_ble_connected(void)
{
	return connected;
}

int app2net_smp_send(uint8_t *data, uint16_t len)
{
	uint8_t buf[280];

	LOG_HEXDUMP_DBG(data, len, "smp response:");
	if (len > 279)
	{
		LOG_ERR("###SMP response too long!");
		return -EBADMSG;
	}
	buf[0] = APP2NET_SMP_SEND;
	memcpy(&buf[1], data, len);

	return nrfx_ipc_send(buf, len+1);	
}

int app2net_nus_send(uint8_t *data, uint16_t len)
{
	uint8_t buf[250];

	if (len > 249)
	{
		LOG_ERR("NUS pakcet too long!");
	}
	buf[0] = APP2NET_NUS_SEND;
	memcpy(&buf[1], data, len);

	return nrfx_ipc_send(buf, len+1);	
}

int app2net_smp_get_mtu(void)
{
	uint8_t data;

	data = APP2NET_SMP_GET_MTU;

	return nrfx_ipc_send(&data, 1);	
}

int app2net_test(uint8_t *data, uint16_t len)
{
	uint8_t buf[250];

	if (len > 249) len = 249;
	buf[0] = APP2NET_TEST;
	memcpy(&buf[1], data, len);

	return nrfx_ipc_send(buf, len+1);	
}

static void ipc_api_thread(void)
{
	LOG_INF("Dual core communication by nrfx_ipc API");
	init_ipc(ipc_app_rx_cb);

	while (1) {
		k_sem_take(&sem_ipc_rx, K_FOREVER);
		ipc_rx_handler(ipc_temp.data, ipc_temp.len);
	}

}

K_THREAD_DEFINE(ipc_api_thread_id, 1024, ipc_api_thread, NULL, NULL,
		NULL, 6, 0, 0);