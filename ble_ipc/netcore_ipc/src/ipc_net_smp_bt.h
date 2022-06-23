/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RPC_NET_SMP_BT_H_
#define RPC_NET_SMP_BT_H_

#include <zephyr/types.h>
#include <zephyr/bluetooth/bluetooth.h>

/**
 * @file
 * @defgroup entropy_ser Entropy driver serialization
 * @{
 * @brief Entropy serialization API.
 */

#ifdef __cplusplus
extern "C" {
#endif

int smp_bt_register_ipc(void);

int smp_bt_unregister_ipc(void);

void ipc_net_bt_smp_send(struct bt_conn *conn, void *data, uint16_t len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* RPC_NET_SMP_BT_H_ */
