/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <nrfx_ipc.h>
#include <stdio.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#ifdef CONFIG_NRF_DFU
#include "nrf_dfu_settings.h"
#include "nrf_dfu.h"
#include <sys/reboot.h>
#include "nrf_dfu.h"
#include "nrf_dfu_validation.h"
#include <drivers/nrfx_errors.h>
#include "pm_config.h"
#endif

LOG_MODULE_REGISTER(ipc_app, LOG_LEVEL_INF);

#ifdef PM_SRAM_PRIMARY_END_ADDRESS
#define SHARE_RAM_END_ADDR PM_SRAM_PRIMARY_END_ADDRESS
#else
#define SHARE_RAM_END_ADDR 0x2007e000
#endif
#define IPC_DATA_MAX_SIZE 0x1000/2  ////4kB RAM should be enough
#define SHARE_RAM_BASE_ADDR (SHARE_RAM_END_ADDR - IPC_DATA_MAX_SIZE * 2 )
#define MAGIC_VALID 0x20220408  
#define CH_NO_SEND 0
#define CH_NO_RECEIVE 1
#define IPC_DATA_HEADER_LEN 16

typedef struct
{
    uint32_t valid;
	uint32_t busy;     
    uint32_t len; 
    uint8_t * data;             
} nrfx_ipc_data_t;

nrfx_ipc_data_t * ipc_tx_buf = (nrfx_ipc_data_t *) SHARE_RAM_BASE_ADDR;
nrfx_ipc_data_t * ipc_rx_buf = (nrfx_ipc_data_t *) (SHARE_RAM_BASE_ADDR+IPC_DATA_MAX_SIZE);

#define NET2APP_BT_ADDR_SEND 1
#define NET2APP_BT_NUS_RECV 2
#define NET2APP_BT_CONN_STATUS 3

static void nrfx_ipc_handler(uint32_t event_mask, void *p_context)
{
	LOG_INF("event_mask %d", event_mask);
	if (event_mask == (1 << CH_NO_RECEIVE)) {
		// we just print out the data
		if (ipc_rx_buf->valid != MAGIC_VALID)
		{
			LOG_WRN("invalid ipc data %x", ipc_rx_buf->valid);			
		}
		else
		{
			if (ipc_rx_buf->data[0] == NET2APP_BT_ADDR_SEND)
			{
				LOG_HEXDUMP_INF(&(ipc_rx_buf->data[1]), ipc_rx_buf->len - 1, "Bluetooth device address:");
			}
			else if (ipc_rx_buf->data[0] == NET2APP_BT_NUS_RECV)
			{
				LOG_HEXDUMP_INF(&(ipc_rx_buf->data[1]), ipc_rx_buf->len - 1, "NUS data:");
			}
			else if (ipc_rx_buf->data[0] == NET2APP_BT_CONN_STATUS)
			{
				LOG_INF("Bluetooth connection status: %d", ipc_rx_buf->data[1]);
			}
			else
			{
				LOG_HEXDUMP_INF(ipc_rx_buf->data, ipc_rx_buf->len, "Undefined data: ");
			}
			
			/* after processe is done, you must reset the buffer to prepare for next receive. 
			 otherwise, you cannot get the next receive */
			ipc_rx_buf->valid = 0;
			ipc_rx_buf->busy = 0;
		}
	}
}

int nrfx_ipc_send(const void *data, int size)
{
	if (size > (IPC_DATA_MAX_SIZE - IPC_DATA_HEADER_LEN) )
	{
		return -EINVAL;
	}
	if (ipc_tx_buf->valid == MAGIC_VALID && ipc_tx_buf->busy == 1)
	{
		LOG_ERR("ipc is busy");
		return EBUSY;
	}
	ipc_tx_buf->valid = MAGIC_VALID;
	ipc_tx_buf->busy = 1;
	ipc_tx_buf->len = size;
	memcpy(ipc_tx_buf->data, data, size);
	nrfx_ipc_signal(CH_NO_SEND);
	return 0;
}

void send_to_net(void)
{
	int ret;
	static uint8_t cnt;
	char test_str[20];

	snprintf(test_str, 16, "I am from APP %c", cnt++);
	ret = nrfx_ipc_send(test_str, 16);
	if (ret)
	{
		LOG_ERR("nrfx_ipc_send error %d", ret);
	}
	else
	{
		LOG_INF("sent successfully %x", cnt-1);
	}
}

#ifdef CONFIG_NRF_DFU
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
#endif

void main(void)
{
	int err;

	LOG_INF("DFU over IPC sample at %s %s", __TIME__, __DATE__);

	nrfx_ipc_init(0, nrfx_ipc_handler, NULL);
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

	LOG_INF("ipc init done");

#ifdef CONFIG_NRF_DFU
	err = dfu_init();
	if (err) {
		LOG_ERR("dfu service init err %d", err);
		return;
	}		
#endif

	while (1) {
		k_sleep(K_SECONDS(3));                 
        LOG_INF("app core start to send");
        send_to_net();        
	}
}
