/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RPC_APP_SMP_H_
#define RPC_APP_SMP_H_

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

int smp_rpc_init(void);
int rpc_app_bt_nus_send(uint8_t *buffer, uint16_t length);
int rpc_app_register_bt_recv_cb(void (*callback)(uint8_t *buffer,
							     uint16_t length));

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* RPC_APP_SMP_H_ */
