/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <hal/nrf_gpio.h>
#include "transport/dtm_transport.h"

#define DTM_INIT_PRIORITY 25 //(CONFIG_CONSOLE_INIT_PRIORITY + 4)

extern struct k_thread _thread_dummy;
extern struct z_kernel _kernel;	
extern void z_sched_init(void);
bool b_dtm_mode;

static inline void z_dummy_thread_init(struct k_thread *dummy_thread)
{
	dummy_thread->base.thread_state = _THREAD_DUMMY;
#ifdef CONFIG_SCHED_CPU_MASK
	dummy_thread->base.cpu_mask = -1;
#endif /* CONFIG_SCHED_CPU_MASK */
	dummy_thread->base.user_options = K_ESSENTIAL;
#ifdef CONFIG_THREAD_STACK_INFO
	dummy_thread->stack_info.start = 0U;
	dummy_thread->stack_info.size = 0U;
#endif /* CONFIG_THREAD_STACK_INFO */
#ifdef CONFIG_USERSPACE
	dummy_thread->mem_domain_info.mem_domain = &k_mem_domain_default;
#endif /* CONFIG_USERSPACE */
#if (K_HEAP_MEM_POOL_SIZE > 0)
	k_thread_system_pool_assign(dummy_thread);
#else
	dummy_thread->resource_pool = NULL;
#endif /* K_HEAP_MEM_POOL_SIZE */

#ifdef CONFIG_TIMESLICE_PER_THREAD
	dummy_thread->base.slice_ticks = 0;
#endif /* CONFIG_TIMESLICE_PER_THREAD */

	z_sched_init();	
	z_current_thread_set(dummy_thread);
	_kernel.ready_q.cache = dummy_thread;
}

int dtm_main(void)
{
	int err;
	union dtm_tr_packet cmd;
	uint32_t i;

	//read GPIO1.08 (Button2) to determine if DTM mode is enabled
	nrf_gpio_cfg_input(40, NRF_GPIO_PIN_PULLUP); // Configure GPIO pin 40 (Button2) as input with pull-up resistor
	if (nrf_gpio_pin_read(40) == 1) {
		printk("DTM mode not enabled by Button2\n");
		return 0; // Exit if DTM mode is not enabled
	}

	//delay for 100ms to allow the system to stabilize
	i = 0;
	while (i < 1000000) {
		i++;
	}

	if (nrf_gpio_pin_read(40) == 0) {
		printk("DTM mode enabled by Button2\n");
	} else {
		printk("DTM mode not enabled by Button2\n");
		return 0; // Exit if DTM mode is not enabled
	}	
	
	printk("Starting Direct Test Mode sample\n");

	//unlock interrupts pri > 6
	__set_BASEPRI(0xC0);
	__set_PSP(0x2000ce00);
	// z_dummy_thread_init(&_thread_dummy);

	err = dtm_tr_init();
	if (err) {
		printk("Error initializing DTM transport: %d\n", err);
		return err;
	}

	b_dtm_mode = true;

	for (;;) {
		cmd = dtm_tr_get();
		err = dtm_tr_process(cmd);
		if (err) {
			printk("Error processing command: %d\n", err);
			return err;
		}
	}
}

SYS_INIT(dtm_main, PRE_KERNEL_1, DTM_INIT_PRIORITY);