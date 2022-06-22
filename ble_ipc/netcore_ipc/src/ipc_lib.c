/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <nrfx_ipc.h>
#include <stdio.h>
#include <logging/log.h>
#include "ipc_lib.h"
#ifdef PM__SRAM_PRIMARY_END_ADDRESS
#include "pm_config.h"
#endif

LOG_MODULE_REGISTER(ipc_lib);

#ifdef PM__SRAM_PRIMARY_END_ADDRESS
#define SHARE_RAM_END_ADDR PM__SRAM_PRIMARY_END_ADDRESS
#else
#define SHARE_RAM_END_ADDR 0x2007e000
#endif
#define IPC_DATA_MAX_SIZE 0x1000/2  ////4kB RAM should be enough
#define SHARE_RAM_BASE_ADDR (SHARE_RAM_END_ADDR - IPC_DATA_MAX_SIZE * 2 )

#ifdef CONFIG_IPC_SMP_BT
#define MAGIC_REQUEST 0x20220408
#define MAGIC_RESPONSE 0x20220616   
#define CH_NO_SEND 1
#define CH_NO_RECEIVE 0
#define IPC_DATA_HEADER_LEN 16
#define IPC_RESPONSE_TIMEOUT 100

typedef struct
{
	uint32_t requst; 
	uint32_t response;      
    uint32_t len; 
    uint8_t * data;             
} nrfx_ipc_data_t;

nrfx_ipc_data_t * ipc_tx_buf = (nrfx_ipc_data_t *) (SHARE_RAM_BASE_ADDR+IPC_DATA_MAX_SIZE);
nrfx_ipc_data_t * ipc_rx_buf = (nrfx_ipc_data_t *) SHARE_RAM_BASE_ADDR;
static ipc_rx_callback_t ipc_rx_callback;
K_SEM_DEFINE(sem_ipc_respone, 0, 1);

static void nrfx_ipc_handler(uint32_t event_mask, void *p_context)
{
	if (event_mask == (1 << CH_NO_RECEIVE)) {

		if (ipc_rx_buf->response == MAGIC_RESPONSE)
		{
			LOG_DBG("ipc isr response %x", ipc_tx_buf->data[0]);
			ipc_rx_buf->response = 0;
			k_sem_give(&sem_ipc_respone);
		}
		else if (ipc_rx_buf->requst == MAGIC_REQUEST)
		{
			LOG_DBG("ipc isr requst %x", ipc_rx_buf->data[0]);
			if (ipc_rx_callback)
			{
				ipc_rx_callback(ipc_rx_buf->data, ipc_rx_buf->len);
			}
			ipc_tx_buf->response = MAGIC_RESPONSE;
			nrfx_ipc_signal(CH_NO_SEND);	
		}
		else
		{
			LOG_WRN("invalid ipc data");
		}
	}
}

int nrfx_ipc_send(const uint8_t *data, uint16_t size)
{
	int ret;
	
	LOG_DBG("ipc send %x", data[0]);
	if (size > (IPC_DATA_MAX_SIZE - IPC_DATA_HEADER_LEN) )
	{
		return -EINVAL;
	}
	if (ipc_tx_buf->requst == MAGIC_REQUEST)
	{
		LOG_ERR("ipc is busy");
		return -EBUSY;
	}
	ipc_tx_buf->requst = MAGIC_REQUEST;
	ipc_tx_buf->len = size;
	memcpy(ipc_tx_buf->data, data, size);
	nrfx_ipc_signal(CH_NO_SEND);
	ret = k_sem_take(&sem_ipc_respone, K_MSEC(IPC_RESPONSE_TIMEOUT));
	if (ret)
	{
		return -EBUSY;
	}
	ipc_tx_buf->requst = 0;	
	return 0;
}

int init_ipc(ipc_rx_callback_t cb)
{
	int ret;

	ret = nrfx_ipc_init(0, nrfx_ipc_handler, NULL);
	if (ret != NRFX_SUCCESS)
	{
		LOG_ERR("ipc init err %x", ret);
		return -EIO;
	}
	ipc_rx_callback = cb;
	IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_IPC), 4,
		    nrfx_isr, nrfx_ipc_irq_handler, 0);
	ipc_tx_buf->data = (void *)((uint32_t) ipc_tx_buf + IPC_DATA_HEADER_LEN);
	ipc_rx_buf->data = (void *)((uint32_t) ipc_rx_buf + IPC_DATA_HEADER_LEN);
	ipc_tx_buf->requst = 0;
	ipc_tx_buf->response = 0;
	ipc_rx_buf->requst = 0;
	ipc_rx_buf->response = 0;		

	nrf_ipc_send_config_set(NRF_IPC, CH_NO_SEND, 1 << CH_NO_SEND);
	nrf_ipc_receive_config_set(NRF_IPC, CH_NO_RECEIVE, 1 << CH_NO_RECEIVE);
	nrf_ipc_int_enable(NRF_IPC, 1 << CH_NO_RECEIVE);

	return 0;	
}

#else
#define MAGIC_VALID 0x20220408  
#define CH_NO_SEND 1
#define CH_NO_RECEIVE 0
#define IPC_DATA_HEADER_LEN 16

typedef struct
{
    uint32_t valid;
	uint32_t busy;     
    uint32_t len; 
    uint8_t * data;             
} nrfx_ipc_data_t;

nrfx_ipc_data_t * ipc_tx_buf = (nrfx_ipc_data_t *) (SHARE_RAM_BASE_ADDR+IPC_DATA_MAX_SIZE);
nrfx_ipc_data_t * ipc_rx_buf = (nrfx_ipc_data_t *) SHARE_RAM_BASE_ADDR;
static ipc_rx_callback_t ipc_rx_callback;

static void nrfx_ipc_handler(uint32_t event_mask, void *p_context)
{
	LOG_DBG("event_mask %d", event_mask);
	if (event_mask == (1 << CH_NO_RECEIVE)) {
		// we just print out the data
		if (ipc_rx_buf->valid != MAGIC_VALID)
		{
			LOG_WRN("invalid ipc data %x", ipc_rx_buf->valid);			
		}
		else
		{
			// LOG_HEXDUMP_INF(ipc_rx_buf->data, ipc_rx_buf->len, "Received: ");
			/* after processe is done, you must reset the buffer to prepare for next receive. 
			 otherwise, you cannot get the next receive */
			if (ipc_rx_callback)
			{
				ipc_rx_callback(ipc_rx_buf->data, ipc_rx_buf->len);
			}
			ipc_rx_buf->valid = 0;
			ipc_rx_buf->busy = 0;
		}
	}
}

int nrfx_ipc_send(const uint8_t *data, uint16_t size)
{
	if (size > (IPC_DATA_MAX_SIZE - IPC_DATA_HEADER_LEN) )
	{
		return -EINVAL;
	}
	if (ipc_tx_buf->valid == MAGIC_VALID && ipc_tx_buf->busy == 1)
	{
		LOG_ERR("##ipc is busy");
		ipc_tx_buf->busy = 0; //temp use only
		return -EBUSY;
	}
	ipc_tx_buf->valid = MAGIC_VALID;
	ipc_tx_buf->busy = 1;
	ipc_tx_buf->len = size;
	memcpy(ipc_tx_buf->data, data, size);
	nrfx_ipc_signal(CH_NO_SEND);
	return 0;
}

int init_ipc(ipc_rx_callback_t cb)
{
	int ret;

	ret = nrfx_ipc_init(0, nrfx_ipc_handler, NULL);
	if (ret != NRFX_SUCCESS)
	{
		LOG_ERR("ipc init err %x", ret);
		return ret;
	}
	ipc_rx_callback = cb;
	IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_IPC), 4,
		    nrfx_isr, nrfx_ipc_irq_handler, 0);
	ipc_tx_buf->data = (void *)((uint32_t) ipc_tx_buf + IPC_DATA_HEADER_LEN);
	ipc_rx_buf->data = (void *)((uint32_t) ipc_rx_buf + IPC_DATA_HEADER_LEN);
	ipc_tx_buf->valid = 0;
	ipc_tx_buf->busy = 0;
	ipc_rx_buf->valid = 0;
	ipc_rx_buf->busy = 0;	

	nrf_ipc_send_config_set(NRF_IPC, CH_NO_SEND, 1 << CH_NO_SEND);
	nrf_ipc_receive_config_set(NRF_IPC, CH_NO_RECEIVE, 1 << CH_NO_RECEIVE);
	nrf_ipc_int_enable(NRF_IPC, 1 << CH_NO_RECEIVE);

	return 0;	
}
#endif
