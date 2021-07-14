/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RPC_NET_NUS_H_
#define RPC_NET_NUS_H_

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

int rpc_net_bt_nus_receive_cb(const uint8_t *buffer, uint16_t length);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* RPC_NET_NUS_H_ */
