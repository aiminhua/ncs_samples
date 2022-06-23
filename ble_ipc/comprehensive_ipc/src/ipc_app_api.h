/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IPC_NET_API_H_
#define IPC_NET_API_H_

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

int app2net_test(uint8_t *data, uint16_t len);
int app2net_smp_get_mtu(void);
int app2net_smp_send(uint8_t *data, uint16_t len);
int app2net_nus_send(uint8_t *data, uint16_t len);
bool is_ble_connected(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* IPC_NET_API_H_ */
