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
#include <drivers/uart.h>
#include <device.h>
#include "nrf_dfu_serial.h"
#include <string.h>
#include "nrf_dfu_transport.h"
#include "nrf_dfu_req_handler.h"
#include "slip.h"
#include "nrf_dfu_handling_error.h"
#include "app_util.h"
#include <logging/log.h>

#define MODULE nrf_dfu_serial_uart
LOG_MODULE_REGISTER(MODULE, CONFIG_NRF_DFU_LOG_LEVEL);

/**@file
 *
 * @defgroup nrf_dfu_serial_uart DFU Serial UART transport
 * @ingroup  nrf_dfu
 * @brief    Device Firmware Update (DFU) transport layer using UART.
 */

#define NRF_SERIAL_OPCODE_SIZE          (sizeof(uint8_t))
#define NRF_UART_MAX_RESPONSE_SIZE_SLIP (2 * NRF_SERIAL_MAX_RESPONSE_SIZE + 1)
#define RX_BUF_SIZE                     (64) //to get 64bytes payload
#define OPCODE_OFFSET                   (sizeof(uint32_t) - NRF_SERIAL_OPCODE_SIZE)
#define DATA_OFFSET                     (OPCODE_OFFSET + NRF_SERIAL_OPCODE_SIZE)
#define UART_SLIP_MTU                   (2 * (RX_BUF_SIZE + 1) + 1)

#define UART_WAIT_FOR_RX 50

static const struct device *uart;
static uint8_t uart_rx_buf[2][CONFIG_NRF_DFU_UART_BUF_SIZE];
static uint8_t *next_buf = uart_rx_buf[1];

static nrf_dfu_serial_t m_serial;
static slip_t m_slip;
static uint8_t m_rsp_buf[NRF_UART_MAX_RESPONSE_SIZE_SLIP];
static bool m_active;

static nrf_dfu_observer_t m_observer;

static uint32_t uart_dfu_transport_init(nrf_dfu_observer_t observer);
static uint32_t uart_dfu_transport_close(nrf_dfu_transport_t const * p_exception);

DFU_TRANSPORT_REGISTER(nrf_dfu_transport_t const uart_dfu_transport) =
{
    .init_func  = uart_dfu_transport_init,
    .close_func = uart_dfu_transport_close,
};

static void payload_free(void * p_buf)
{

}

static ret_code_t rsp_send(uint8_t const * p_data, uint32_t length)
{
    uint32_t slip_len;
    (void) slip_encode(m_rsp_buf, (uint8_t *)p_data, length, &slip_len);

    return uart_tx(uart, m_rsp_buf, slip_len, SYS_FOREVER_MS);
}

static __INLINE void on_rx_complete(nrf_dfu_serial_t * p_transport, uint8_t * p_data, uint8_t len)
{
    ret_code_t ret_code = NRF_ERROR_TIMEOUT;

    // Check if there is byte to process. Zero length transfer means that RXTO occured.
    for (int i = 0; i < len; i++) 
    {
        ret_code = slip_decode_add_byte(&m_slip, p_data[i]);

		if (ret_code == NRF_SUCCESS)
		{
			nrf_dfu_serial_on_packet_received(p_transport,
											 (uint8_t const *)m_slip.p_buffer,
											 m_slip.current_index);
			m_slip.current_index = 0;
			m_slip.state         = SLIP_STATE_DECODING;
		}

    }
}

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);
	int err;

	switch (evt->type) {
	case UART_TX_DONE:
	{		
		//LOG_DBG("UART_TX_DONE %d", evt->data.tx.len);	
	}
		break;

	case UART_RX_RDY:
	{	
        on_rx_complete((nrf_dfu_serial_t*)user_data,
                        &evt->data.rx.buf[evt->data.rx.offset],
                        evt->data.rx.len);
		//LOG_DBG("UART_RX_RDY %d", evt->data.rx.len);
	}
		break;

	case UART_RX_DISABLED:
		//LOG_DBG("UART_RX_DISABLED");
		err = uart_rx_enable(uart, uart_rx_buf[0], sizeof(uart_rx_buf[0]), UART_WAIT_FOR_RX);
		if (err) {
			LOG_ERR("UART RX enable failed: %d", err);			
		}
		break;

	case UART_RX_BUF_REQUEST:
		err = uart_rx_buf_rsp(uart, next_buf,
			sizeof(uart_rx_buf[0]));
		if (err) {
			LOG_WRN("UART RX buf rsp: %d", err);
		}		
		break;

	case UART_RX_BUF_RELEASED:
		//LOG_DBG("UART_RX_BUF_RELEASED");
		next_buf = evt->data.rx_buf.buf;
		break;

	case UART_TX_ABORTED:
		//LOG_DBG("UART_TX_ABORTED");
		break;

	default:
		break;
	}
}

static int uart_init()
{
	int err;
	
	uart = device_get_binding(CONFIG_NRF_DFU_UART_DEV_NAME);
	if (!uart) {
		LOG_ERR("DFU UART get binding error");
		return -ENXIO;
	}

	err = uart_callback_set(uart, uart_cb, &m_serial);
	if (err) {
		return err;
	}

	return uart_rx_enable(uart, uart_rx_buf[0], sizeof(uart_rx_buf[0]), UART_WAIT_FOR_RX);

}


static uint32_t uart_dfu_transport_init(nrf_dfu_observer_t observer)
{
    uint32_t err_code = NRF_SUCCESS;

    if (m_active)
    {
        return err_code;
    }

    LOG_DBG("serial_dfu_transport_init()");
    m_observer = observer;

	static uint8_t p_rx_buf[UART_SLIP_MTU + 1];

    m_slip.p_buffer      =  &p_rx_buf[OPCODE_OFFSET];
    m_slip.current_index = 0;
    m_slip.buffer_len    = UART_SLIP_MTU;
    m_slip.state         = SLIP_STATE_DECODING;

    m_serial.rsp_func           = rsp_send;
    m_serial.payload_free_func  = payload_free;
    m_serial.mtu                = UART_SLIP_MTU;
    m_serial.p_rsp_buf          = &m_rsp_buf[NRF_UART_MAX_RESPONSE_SIZE_SLIP -
                                            NRF_SERIAL_MAX_RESPONSE_SIZE];
    m_serial.p_low_level_transport = &uart_dfu_transport;

    err_code = uart_init();    
    LOG_DBG("serial_dfu_transport_init() completed");

    m_active = true;

    if (m_observer)
    {
        m_observer(NRF_DFU_EVT_TRANSPORT_ACTIVATED);
    }

    return err_code;
}


static uint32_t uart_dfu_transport_close(nrf_dfu_transport_t const * p_exception)
{
    uint32_t err = NRF_SUCCESS;
    
    if ((m_active == true) && (p_exception != &uart_dfu_transport))
    {
    #ifdef CONFIG_PM_DEVICE        
		((const struct uart_driver_api *)uart->api)->rx_disable(uart);		
		err = pm_device_state_set(uart,	PM_DEVICE_STATE_LOW_POWER);
        if (err) {
            LOG_ERR("Disabling DFU UART failed %d", err);
        }
    #endif    	
        m_active = false;
    }

    return err;
}

