/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IPC_CMD_IDS_H_
#define IPC_CMD_IDS_H_

#include <zephyr/types.h>


/**
 * @file
 * @defgroup entropy_ser Entropy driver serialization
 * @{
 * @brief Entropy serialization API.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define NET2APP_TEST 1
#define NET2APP_BT_ADDR_SEND 2
#define NET2APP_BT_CONN_STATUS 3
#define NET2APP_BT_NUS_RECV 4
#define NET2APP_BT_SMP_SEND 5
#define NET2APP_BT_SEND_MTU 6

#define APP2NET_TEST 0x81
#define APP2NET_SMP_GET_MTU 0x82
#define APP2NET_SMP_SEND 0x83
#define APP2NET_NUS_SEND 0x84

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* IPC_CMD_IDS_H_ */
