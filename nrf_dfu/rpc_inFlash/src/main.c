/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */

#include <zephyr/types.h>
#include <zephyr.h>
#include <drivers/uart.h>

#include <device.h>
#include <soc.h>

#include <dk_buttons_and_leds.h>

#include <settings/settings.h>

#include <stdio.h>

#include "nrf_dfu_settings.h"
#include "nrf_dfu.h"

#include "uart_hs.h"
#include "power/reboot.h"
#include "nrf_dfu.h"
#include "nrf_dfu_validation.h"
#include <drivers/nrfx_errors.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>

#define LOG_MODULE_NAME main
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define RUN_STATUS_LED DK_LED1
#define RUN_LED_BLINK_INTERVAL 1000


#define UART_BUF_SIZE CONFIG_BT_NUS_UART_BUFFER_SIZE
#define UART_WAIT_FOR_BUF_DELAY K_MSEC(50)
#define UART_WAIT_FOR_RX CONFIG_BT_NUS_UART_RX_WAIT_TIME

static void configure_gpio(void)
{
	int err;

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}
}

/**@brief Function for handling DFU events.
 */
static void dfu_observer(nrf_dfu_evt_type_t evt_type)
{
    switch (evt_type)
    {
        case NRF_DFU_EVT_DFU_STARTED:
        case NRF_DFU_EVT_OBJECT_RECEIVED:

            break;
        case NRF_DFU_EVT_DFU_COMPLETED:
        case NRF_DFU_EVT_DFU_ABORTED:
			LOG_INF("resetting...");
			while(log_process(false));
            sys_reboot(SYS_REBOOT_WARM);
            break;
        case NRF_DFU_EVT_TRANSPORT_DEACTIVATED:
            // Reset the internal state of the DFU settings to the last stored state.
			LOG_INF("NRF_DFU_EVT_TRANSPORT_DEACTIVATED");
            nrf_dfu_settings_reinit();
            break;
        default:
            break;
    }

}

int dfu_init(void)
{
    int ret_val;

    ret_val = nrf_dfu_settings_init(true);
    if (ret_val != NRF_SUCCESS)
	{
		LOG_WRN("dfu settings init err %d", ret_val);
	}

    ret_val = nrf_dfu_init(dfu_observer);
    if (ret_val != NRF_SUCCESS)
	{
		LOG_WRN("dfu init err %d", ret_val);
	}

    return ret_val;
}

void uart_rx_cb(uint8_t *data, uint16_t len)
{
	// if (bt_nus_send(NULL,data, len)) {
	// 	LOG_WRN("Failed to send data over BLE connection");
	// }	
}
static void error(void)
{
	dk_set_leds_state(DK_ALL_LEDS_MSK, DK_NO_LEDS_MSK);

	while (true) {
		/* Spin for ever */
		k_sleep(K_MSEC(1000));
	}
}

void main(void)
{
	int blink_status = 0;
	int err = 0;

	LOG_INF("### nRF5 SDK DFU example %s %s\n", __TIME__, __DATE__);

	configure_gpio();

	err = uart_init(uart_rx_cb);
	if (err) {
		error();
	}

	err = dfu_init();
	if (err) {
		LOG_ERR("dfu service init err %d", err);
		return;
	}

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);		
		uart_receive();
	}
}