/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ble_nrf_dfu_event.h>


EVENT_TYPE_DEFINE(ble_nrf_dfu_event,
		  IS_ENABLED(CONFIG_NRF_DFU),
		  NULL,
		  NULL);
