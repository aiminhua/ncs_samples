/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RPC_NET_SMP_BT_H_
#define RPC_NET_SMP_BT_H_

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

int smp_bt_register_rpc(void);

int smp_bt_unregister_rpc(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* RPC_NET_SMP_BT_H_ */
