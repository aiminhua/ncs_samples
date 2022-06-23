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
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include "ipc_lib.h"
#include "../../ipc_cmd_ids.h"
#include "ipc_net_api.h"
#include "ipc_net_smp_bt.h"

#define LOG_MODULE_NAME ipc_api
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

struct ipc_data_t {	
	uint8_t *data;
	uint16_t len;
};
static struct ipc_data_t ipc_temp;
extern struct bt_conn *current_conn;
static K_SEM_DEFINE(sem_ipc_rx, 0, 1);

static void ipc_rx_handler(uint8_t *data, uint16_t len)
{
	switch (data[0])
	{
#ifdef CONFIG_IPC_SMP_BT		
		case APP2NET_SMP_GET_MTU:
			net2app_send_bt_mtu();
		break;

		case APP2NET_SMP_SEND:
			ipc_net_bt_smp_send(current_conn, &data[1], len-1);			
		break;
#endif
		case APP2NET_TEST:
			LOG_HEXDUMP_INF(&data[1], len-1, "IPC test:");
		break;		

		default:
			LOG_ERR("undefined IPC request");
		break;
	}	
}

static void ipc_net_rx_cb(uint8_t *data, uint16_t len)
{
	ipc_temp.len = len;
	ipc_temp.data = data;
	k_sem_give(&sem_ipc_rx);
}

int net2app_send_bt_addr(void)
{

	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	size_t count = ARRAY_SIZE(addrs);
	char addr_s[BT_ADDR_LE_STR_LEN];
	char data[8];
	int ret;

	bt_id_get(addrs, &count);
	bt_addr_le_to_str(&addrs[0], addr_s, sizeof(addr_s));
	printk("===BT dev addr: %s ====\n", addr_s);

	data[0] = NET2APP_BT_ADDR_SEND;
	memcpy(&data[1], (uint8_t *)&addrs[0], 7);

	ret = nrfx_ipc_send(data, sizeof(data));
	if (ret) {
		LOG_ERR("BT dev addr sent err %d", ret);
	}	
	return ret;
	 
}

int net2app_send_nus(uint8_t *data, uint16_t len)
{
	uint8_t buf[CONFIG_BT_L2CAP_TX_MTU+1];

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

#ifdef CONFIG_IPC_SMP_BT
int net2app_send_bt_mtu(void)
{
	uint8_t data[3];
	uint16_t mtu = bt_gatt_get_mtu(current_conn) - 3;
	LOG_DBG("smp BT MTU:%d", mtu);	

	data[0] = NET2APP_BT_SEND_MTU;
	*((uint16_t *) &data[1]) = mtu;

	return nrfx_ipc_send(data, sizeof(data));	
}

int net2app_bt_smp_send(uint8_t *data, uint16_t len)
{
	uint8_t buf[CONFIG_BT_L2CAP_TX_MTU+1];
	LOG_HEXDUMP_DBG(data, len, "smp cmd:");
	buf[0] = NET2APP_BT_SMP_SEND;
	memcpy(&buf[1], data, len);

	return nrfx_ipc_send(buf, len+1);	
}
#endif

int net2app_test(uint8_t *data, uint16_t len)
{
	uint8_t buf[250];

	if (len > 249) len = 249;

	buf[0] = NET2APP_TEST;
	memcpy(&buf[1], data, len);

	return nrfx_ipc_send(buf, len+1);	
}

void ipc_thread(void)
{

	LOG_INF("Dual core communication by nrfx_ipc API");

	init_ipc(ipc_net_rx_cb);
	k_sleep(K_SECONDS(3)); //wait for BT stack init
	net2app_send_bt_addr();

	while (1) {
		k_sem_take(&sem_ipc_rx, K_FOREVER);
		ipc_rx_handler(ipc_temp.data, ipc_temp.len);
	}

}

K_THREAD_DEFINE(ipc_thread_id, 1024, ipc_thread, NULL, NULL,
		NULL, 6, 0, 0);