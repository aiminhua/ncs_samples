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
#include "rpc_app_api.h"
#include <logging/log.h>
#include <drivers/uart.h>

#define LOG_MODULE_NAME rpc_thread1
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

extern struct k_sem sem_rpc_tx;

#ifdef CONFIG_RPC_NUS_DEDICATE

#define BUFFER_LENGTH 64
static uint8_t buffer[BUFFER_LENGTH];	

static void bt_recv_cb(uint8_t *buffer, uint16_t length)
{
	LOG_INF("received ble data from netcore");
	LOG_HEXDUMP_INF(buffer, length, "data:");
}

static void app2net_tx1(void)
{
	static uint8_t cnt;
	int err;

	snprintf(buffer, 12, "HelloApp%d", cnt++);		
	buffer[11] = 0;		
	err = rpc_app_bt_nus_send(buffer, 12);
	if (err) {
		LOG_ERR("rpc_app_bt_nus_send failed: %d\n", err);			
	}
	else
	{
		LOG_HEXDUMP_INF(buffer, 12, "sent by app core:");
	}

}
#endif

static void app2net_tx2(void)
{
	static uint8_t cnt;
	int err;
	uint8_t buffer[20];


	snprintf(buffer, 10, "RPC_app_%d", cnt++);		
	buffer[9] = 0;		
	err = app2net_send_nus(buffer, 10);
	if (err) {
		LOG_ERR("app2net nus send err %d", err);			
	}
	else
	{
		LOG_HEXDUMP_INF(buffer, 10, "app2net nus:");
	}	
}

void rpc_thread1(void)
{
	LOG_INF("**dual core communication example by RPC encapsulated API");

#ifdef CONFIG_RPC_NUS_DEDICATE	
    rpc_app_register_bt_recv_cb(bt_recv_cb);
#endif	
	k_sem_take(&sem_rpc_tx, K_FOREVER);	

	while (1) {	
		LOG_INF("RPC thread @ appcore");
#ifdef CONFIG_RPC_NUS_DEDICATE				
		app2net_tx1();
		k_sleep(K_SECONDS(2));
#endif			
		app2net_tx2();
		k_sleep(K_SECONDS(2));
	}
}

K_THREAD_DEFINE(rpc_thread_id1, 1024, rpc_thread1, NULL, NULL,
		NULL, 6, 0, 0);