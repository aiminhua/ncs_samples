/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <nrfx.h>
#include <nrfx_timer.h>
#include <zephyr/logging/log.h>

#include "dtm_uart_wait.h"
static uint32_t           m_current_time = 0;                                 /**< Counter for interrupts from timer to ensure that the 2 bytes forming a DTM command are received within the time window. */

// LOG_MODULE_REGISTER(dtm_wait, CONFIG_DTM_TRANSPORT_LOG_LEVEL);

/* Timer used for measuring UART poll cycle wait time. */
#if defined(CONFIG_SOC_SERIES_NRF54H)
	#define WAIT_TIMER_INSTANCE        021
#elif defined(CONFIG_SOC_SERIES_NRF54L)
	#define WAIT_TIMER_INSTANCE        21
#else
	#define WAIT_TIMER_INSTANCE        1
#endif /* defined(CONFIG_SOC_SERIES_NRF54H) */

#define WAIT_TIMER_IRQ             NRFX_CONCAT_3(TIMER,			 \
						 WAIT_TIMER_INSTANCE,    \
						 _IRQn)

/* 本版 nrfx 使用统一的 nrfx_timer_irq_handler(实例指针作参数)，timer 实例
 * 由 devicetree 节点(timer21 status=okay)使能，故不再需要 CONFIG_NRFX_TIMERxx。
 */

#if DT_NODE_HAS_PROP(DTM_UART, current_speed)
/* UART Baudrate used to communicate with the DTM library. */
// #define DTM_UART_BAUDRATE DT_PROP(DTM_UART, current_speed)
#define DTM_UART_BAUDRATE 19200

/* The UART poll cycle in micro seconds.
 * A baud rate of e.g. 19200 bits / second, and 8 data bits, 1 start/stop bit,
 * no flow control, give the time to transmit a byte:
 * 10 bits * 1/19200 = approx: 520 us. To ensure no loss of bytes,
 * the UART should be polled every 260 us.
 */
#define DTM_UART_POLL_CYCLE ((uint32_t) (10 * 1e6 / DTM_UART_BAUDRATE / 2))
#else
#error "DTM UART node not found"
#endif /* DT_NODE_HAS_PROP(DTM_UART, currrent_speed) */

/* Timer to be used for measuring UART poll cycle wait time.
 * 不能用 const：本版 nrfx 的实例结构含可变控制块(.cb)，nrfx_timer_init 会写入。
 */
static nrfx_timer_t wait_timer =
	NRFX_TIMER_INSTANCE(NRF_TIMER_INST_GET(WAIT_TIMER_INSTANCE));

/* Semaphore for synchronizing UART poll cycle wait time.*/
// static K_SEM_DEFINE(wait_sem, 0, 1);
static volatile bool b_run;

static void wait_timer_handler(nrf_timer_event_t event_type, void *context)
{
	nrfx_timer_disable(&wait_timer);
	nrfx_timer_clear(&wait_timer);

	// k_sem_give(&wait_sem);
	b_run = true;
}

int dtm_uart_wait_init(void)
{
	int err;
	nrfx_timer_config_t timer_cfg = {
		.frequency = NRFX_MHZ_TO_HZ(1),
		.mode      = NRF_TIMER_MODE_TIMER,
		.bit_width = NRF_TIMER_BIT_WIDTH_16,
	};

	err = nrfx_timer_init(&wait_timer, &timer_cfg, wait_timer_handler);
	if (err != 0) {
		printk("nrfx_timer_init failed with: %d", err);
		return -EAGAIN;
	}

	IRQ_CONNECT(WAIT_TIMER_IRQ, CONFIG_DTM_TIMER_IRQ_PRIORITY,
		    nrfx_timer_irq_handler, &wait_timer, 0);

	/* nrfx_timer 不会使能 NVIC 线，需手动开启，否则轮询节拍中断不触发，
	 * dtm_uart_wait() 会在 while(!b_run) 处死等。
	 */
	irq_enable(WAIT_TIMER_IRQ);

    // nrfy_timer_event_clear(wait_timer.p_reg,
    //                           nrfy_timer_compare_event_get(NRF_TIMER_CC_CHANNEL0));			
	nrfx_timer_compare(&wait_timer,
		NRF_TIMER_CC_CHANNEL0,
		nrfx_timer_us_to_ticks(&wait_timer, DTM_UART_POLL_CYCLE),
		true);

	// nrfx_timer_enable(&wait_timer);

	return 0;
}

uint32_t dtm_uart_wait(void)
{
	// int err;

	nrfx_timer_enable(&wait_timer);

	// err = k_sem_take(&wait_sem, K_FOREVER);
	// if (err) {
	// 	printk("UART wait error: %d", err);
	// }

	b_run = false;
	while (!b_run);
	return ++m_current_time;  

	// while(1) {

    //     if (nrfy_timer_event_check(wait_timer.p_reg,
    //                     nrfy_timer_compare_event_get(NRF_TIMER_CC_CHANNEL0)))
    //     {
    //         // Reset timeout event flag for next iteration.
    //         nrfy_timer_event_clear(wait_timer.p_reg,
    //                           nrfy_timer_compare_event_get(NRF_TIMER_CC_CHANNEL0));
	// 		// nrfx_timer_disable(&wait_timer);
	// 		nrfx_timer_clear(&wait_timer);					  
	// 		return ++m_current_time;            
    //     }
	// }


}
