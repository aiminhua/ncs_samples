/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IPC_LIB_H_
#define IPC_LIB_H_

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

typedef void (*ipc_rx_callback_t)(uint8_t *data, uint16_t len);

int init_ipc(ipc_rx_callback_t cb);
int nrfx_ipc_send(const uint8_t *data, uint16_t size);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* IPC_LIB_H_ */
