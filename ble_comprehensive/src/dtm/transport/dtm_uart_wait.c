/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include "dtm_uart_wait.h"
static uint32_t m_current_time = 0;

#if DT_NODE_HAS_PROP(DTM_UART, current_speed)
#define DTM_UART_BAUDRATE 19200
#define DTM_UART_POLL_CYCLE ((uint32_t) (10 * 1e6 / DTM_UART_BAUDRATE / 2))
#else
#error "DTM UART node not found"
#endif

/* Busy-wait for ~260 us at 128 MHz (nRF54L15).
 * Timer-based wait not used because nrfx_timer IRQ does not fire
 * reliably at PRE_KERNEL_1 init stage.
 */
#define BUSY_WAIT_COUNT 2600

int dtm_uart_wait_init(void)
{
	return 0;
}

uint32_t dtm_uart_wait(void)
{
	volatile uint32_t i;
	for (i = 0; i < BUSY_WAIT_COUNT; i++) {
		__asm__ volatile("nop");
	}
	return ++m_current_time;
}
