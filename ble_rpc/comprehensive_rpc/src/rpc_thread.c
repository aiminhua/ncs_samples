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
#include <logging/log.h>
#include <drivers/uart.h>

#define LOG_MODULE_NAME rpc_thread
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

extern struct k_sem sem_rpc_tx;

static void app2net_send_example(void)
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

void rpc_thread(void)
{
	LOG_INF("**dual core communication example by RPC encapsulated API");

	k_sem_take(&sem_rpc_tx, K_FOREVER);	

	while (1) {	
		LOG_INF("RPC thread @ appcore");		
		app2net_send_example();
		k_sleep(K_SECONDS(2));
	}
}

K_THREAD_DEFINE(rpc_thread_id, 1024, rpc_thread, NULL, NULL,
		NULL, 6, 0, 0);