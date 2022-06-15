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

int net2app_send_bt_addr(void);
int net2app_send_nus(uint8_t *data, uint16_t len);
int net2app_send_conn_status(uint8_t connected);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* IPC_NET_API_H_ */
