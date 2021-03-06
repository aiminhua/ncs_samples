/**
 * Copyright (c) 2016 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "nrf_dfu.h"
#include "nrf_dfu_transport.h"
#include "nrf_dfu_req_handler.h"
#include <zephyr/sys/printk.h>
#include <zephyr/dfu/mcuboot.h>
#if CONFIG_SECURE_BOOT
#include <fw_info.h>
#endif

static nrf_dfu_observer_t m_user_observer;                          //<! Observer callback set by the user.

/**
 * @brief This function calls the user's observer (@ref m_observer) after it is done handling the event.
 */
static void dfu_observer(nrf_dfu_evt_type_t event)
{
    switch (event)
    {
        case NRF_DFU_EVT_DFU_COMPLETED:
        case NRF_DFU_EVT_DFU_ABORTED:
#ifndef NRF_DFU_NO_TRANSPORT
            UNUSED_RETURN_VALUE(nrf_dfu_transports_close(NULL));
#endif
            break;
        default:
            break;
    }

    /* Call user's observer if present. */
    if (m_user_observer)
    {
        m_user_observer(event);
    }
}



uint32_t nrf_dfu_init(nrf_dfu_observer_t observer)
{
    uint32_t ret_val;

    m_user_observer = observer;

    printk("initialize nrf_dfu\n");

#ifdef CONFIG_BOOTLOADER_MCUBOOT
    boot_write_img_confirmed();
#endif

#if CONFIG_SECURE_BOOT
	if ((uint32_t)(uintptr_t)nrf_dfu_init < PM_S1_IMAGE_ADDRESS) {
		printk("*** S0 image is running. Please upload S1 image for DFU ***\n");
	}
    else
    {
        printk("** S1 image is running. Please upload S0 image for DFU ***\n");
    }    
#endif

    dfu_observer(NRF_DFU_EVT_DFU_INITIALIZED);

    // Initializing transports
    ret_val = nrf_dfu_transports_init(dfu_observer);
    if (ret_val != NRF_SUCCESS)
    {
        printk("Could not initalize DFU transport: 0x%08x", ret_val);
        return ret_val;
    }

    ret_val = nrf_dfu_req_handler_init(dfu_observer);

    return ret_val;
}
