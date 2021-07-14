/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RPC_APP_API_H_
#define RPC_APP_API_H_

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

int app2net_send_nus(uint8_t *data, uint16_t len);
int rpc_app2net_send(int type, uint8_t *buffer, size_t length);
int net2app_receive(int type, uint8_t *data, size_t len);
bool is_ble_connected(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* RPC_APP_API_H_ */
