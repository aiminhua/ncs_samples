/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <stdio.h>
#include <logging/log.h>
#include <drivers/uart.h>
#include "uart_hs.h"


#define LOG_MODULE_NAME uart_hs
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define UART_DEVICE_NAME         DT_LABEL(DT_NODELABEL(uart0))

#define UART_BUF_SIZE 255
#define UART_WAIT_FOR_BUF_DELAY K_MSEC(100)
#define UART_WAIT_FOR_RX 50

static const struct device *uart;
static rx_callback_t rx_callback;

struct uart_data_t {
	void *fifo_reserved;
	uint8_t data[UART_BUF_SIZE];
	uint16_t len;
};

static uint8_t uart_rx_buf[2][UART_BUF_SIZE];
static uint8_t *next_buf = uart_rx_buf[1];

static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);


static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);
	int err;

	switch (evt->type) {
	case UART_TX_DONE:
	{
		struct uart_data_t *buf;
		struct uart_data_t *buf2;
		
		buf = CONTAINER_OF(evt->data.tx.buf, struct uart_data_t,  data);		
		LOG_INF("UART_TX_DONE %d", evt->data.tx.len);
		k_free(buf);

		buf2 = k_fifo_get(&fifo_uart_tx_data, K_NO_WAIT);
		if (!buf2) {
			return;
		}

		if (uart_tx(uart, buf2->data, buf2->len, SYS_FOREVER_MS)) {
			LOG_WRN("uart_tx fail @ cb");
		}
	}
		break;

	case UART_RX_RDY:
	{
		struct uart_data_t *buf = k_malloc(sizeof(struct uart_data_t));
		memcpy(buf->data, &evt->data.rx.buf[evt->data.rx.offset], evt->data.rx.len);	
		buf->len = evt->data.rx.len;		
		k_fifo_put(&fifo_uart_rx_data, buf);
		LOG_INF("UART_RX_RDY %d", buf->len);
	}
		break;

	case UART_RX_DISABLED:
		LOG_INF("UART_RX_DISABLED");
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
		LOG_INF("UART_RX_BUF_RELEASED");
		next_buf = evt->data.rx_buf.buf;
		break;

	case UART_TX_ABORTED:
		LOG_INF("UART_TX_ABORTED");
		break;

	default:
		break;
	}
}

int uart_init(rx_callback_t cb)
{
	int err;
	
	uart = device_get_binding(UART_DEVICE_NAME);
	if (!uart) {
		return -ENXIO;
	}

	err = uart_callback_set(uart, uart_cb, NULL);
	if (err) {
		return err;
	}

	rx_callback = cb;

	return uart_rx_enable(uart, uart_rx_buf[0], sizeof(uart_rx_buf[0]), UART_WAIT_FOR_RX);

}

int uart_send(const uint8_t *buf, uint16_t len)
{
	int err;
	struct uart_data_t *tx = k_malloc(sizeof(*tx));

	if (!tx) {
		LOG_WRN("Not able to allocate UART send data buffer");
		return -ENOMEM; 
	}

	memcpy(tx->data, buf, len);	
	tx->len = len;
	err = uart_tx(uart, tx->data, tx->len, SYS_FOREVER_MS);
	if (err) {
		LOG_WRN("buffer uart tx data for later retry");
		k_fifo_put(&fifo_uart_tx_data, tx);
	}
	return err;		
}

void uart_receive()
{
	/* Wait indefinitely*/
	struct uart_data_t *buf = k_fifo_get(&fifo_uart_rx_data,
							K_FOREVER);
	rx_callback(buf->data,buf->len);							
	k_free(buf);		
}

// void uart_thread(void)
// {    
// 	static uint32_t uart_len;	

// 	LOG_INF("**high speed UART example");
// 	uart_init();

// 	while (1) {

// 		/* Send the UART data back to the peer. Wait indefinitely*/
// 		struct uart_data_t *buf = k_fifo_get(&fifo_uart_rx_data,
// 						     K_FOREVER);

// 		uart_send(buf->data,buf->len);

// 		uart_len += buf->len;
// 		LOG_INF("uart rx total len %d and %d", uart_len, buf->len);
// 		k_free(buf);					
// 	}
// }

