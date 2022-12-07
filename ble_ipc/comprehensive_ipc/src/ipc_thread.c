/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <nrfx_ipc.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include "ipc_app_api.h"
#include "ipc_lib.h"

LOG_MODULE_REGISTER(ipc_app, LOG_LEVEL_INF);

extern struct k_sem sem_ipc_tx;

static int send_to_net(void)
{	
	int ret;
	static uint8_t cnt;
	char test_str[20];

	snprintf(test_str, 16, "I am from APP %c", cnt++);
	ret = app2net_test(test_str, 15);
	if (ret)
	{
		LOG_ERR("ipc test error %d", ret);
	}
	else
	{
		LOG_DBG("ipc test done %x", cnt-1);
	}

	return ret;	
}

static void ipc_thread(void)
{

	LOG_INF("Dual core communication example by nrfx_ipc API");

	k_sem_take(&sem_ipc_tx, K_FOREVER);

	while (1) {				                
        LOG_INF("app core start to send");
        send_to_net();
		k_sleep(K_SECONDS(5));        
	}
}

K_THREAD_DEFINE(ipc_thread_id, 1024, ipc_thread, NULL, NULL,
		NULL, 7, 0, 0);