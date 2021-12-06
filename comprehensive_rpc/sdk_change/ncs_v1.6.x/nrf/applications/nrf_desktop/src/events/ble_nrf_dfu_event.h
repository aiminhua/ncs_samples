/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_NRF_DFU_EVENT_H_
#define _BLE_NRF_DFU_EVENT_H_

/**
 * @brief Bluetooth LE SMP Event
 * @defgroup ble_smp_event Bluetooth LE SMP Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Bluetooth LE SMP transfer event. */
struct ble_nrf_dfu_event {
	struct event_header header;
};
EVENT_TYPE_DECLARE(ble_nrf_dfu_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _BLE_NRF_DFU_EVENT_H_ */
