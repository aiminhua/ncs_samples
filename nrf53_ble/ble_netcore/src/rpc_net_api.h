/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RPC_NET_API_H_
#define RPC_NET_API_H_

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

int net2app_send_bt_addr(void);
int net2app_send_nus(uint8_t *data, uint16_t len);
int net2app_send_conn_status(uint8_t connected);

int rpc_net2app_send(int type, uint8_t *buffer, size_t length);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* RPC_NET_API_H_ */
