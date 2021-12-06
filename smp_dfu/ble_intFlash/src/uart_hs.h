/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UART_HS_H_
#define UART_HS_H_

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

typedef void (*rx_callback_t)(uint8_t *data, uint16_t len);

int uart_init(rx_callback_t cb);
int uart_send(const uint8_t *buf, uint16_t len);
void uart_receive();

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* UART_HS_H_ */
