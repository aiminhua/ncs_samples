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

#include <errno.h>
#include <zephyr.h>
#include <init.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <stddef.h>
#include "nrf_dfu_transport.h"
#include "nrf_dfu_req_handler.h"
#include "nrf_dfu_handling_error.h"
#include "app_util.h"
#include <logging/log.h>

#define MODULE nrf_dfu_ble
LOG_MODULE_REGISTER(MODULE, CONFIG_NRF_DFU_LOG_LEVEL);

#define GATT_HEADER_LEN                     3                                                       /**< GATT header length. */
#define GATT_PAYLOAD(mtu)                   ((mtu) - GATT_HEADER_LEN)                               /**< Length of the ATT payload for a given ATT MTU. */
#define MAX_DFU_PKT_LEN                     (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - GATT_HEADER_LEN)       /**< Maximum length (in bytes) of the DFU Packet characteristic (3 bytes are used for the GATT opcode and handle). */
#define MAX_RESPONSE_LEN                    17                                                      /**< Maximum length (in bytes) of the response to a Control Point command. */
#define RESPONSE_HEADER_LEN                 3                                                       /**< The length of the header of a response. I.E. the index of the opcode-specific payload. */

#define DFU_BLE_FLAG_INITIALIZED            (1 << 0)                                                /**< Flag to check if the DFU service was initialized by the application.*/
 
uint32_t ble_dfu_transport_init(nrf_dfu_observer_t observer);
uint32_t ble_dfu_transport_close(nrf_dfu_transport_t const * p_exception);

#ifndef CONFIG_NRF_DFU_RPC_NET
DFU_TRANSPORT_REGISTER(nrf_dfu_transport_t const ble_dfu_transport) =
{
    .init_func  = ble_dfu_transport_init,
    .close_func = ble_dfu_transport_close,
};
#else
// nrf_dfu_transport_t const ble_dfu_transport;
#endif


static uint32_t           m_flags;
static uint16_t           m_pkt_notif_target;                                                       /**< Number of packets of firmware data to be received before transmitting the next Packet Receipt Notification to the DFU Controller. */
static uint16_t           m_pkt_notif_target_cnt;                                                   /**< Number of packets of firmware data received after sending last Packet Receipt Notification or since the receipt of a @ref BLE_DFU_PKT_RCPT_NOTIF_ENABLED event from the DFU service, which ever occurs later.*/
              /**< Advertising handle used to identify an advertising set. */
static nrf_dfu_observer_t m_observer;                                                               /**< Observer function called on certain events. */

static int response_send(uint8_t * data, uint16_t len);


/**@brief Function for encoding the beginning of a response.
 *
 * @param[inout] p_buffer  The buffer to encode into.
 * @param[in]    op_code   The opcode of the response.
 * @param[in]    result    The result of the operation.
 *
 * @return The length added to the buffer.
 */
static uint32_t response_prepare(uint8_t * p_buffer, uint8_t op_code, uint8_t result)
{
    __ASSERT_NO_MSG(p_buffer);
    p_buffer[0] = NRF_DFU_OP_RESPONSE;
    p_buffer[1] = op_code;
    p_buffer[2] = result;
    return RESPONSE_HEADER_LEN;
}


/**@brief Function for encoding a select object response into a buffer.
 *
 * The select object response consists of a maximum object size, a firmware offset, and a CRC value.
 *
 * @param[inout] p_buffer   The buffer to encode the response into.
 * @param[in]    max_size   The maximum object size value to encode.
 * @param[in]    fw_offset  The firmware offset value to encode.
 * @param[in]    crc        The CRC value to encode.
 *
 * @return The length added to the buffer.
 */
static uint32_t response_select_obj_add(uint8_t  * p_buffer,
                                        uint32_t   max_size,
                                        uint32_t   fw_offset,
                                        uint32_t   crc)
{
    uint16_t offset = uint32_encode(max_size,  &p_buffer[RESPONSE_HEADER_LEN]);
    offset         += uint32_encode(fw_offset, &p_buffer[RESPONSE_HEADER_LEN + offset]);
    offset         += uint32_encode(crc,       &p_buffer[RESPONSE_HEADER_LEN + offset]);
    return offset;
}


/**@brief Function for encoding a CRC response into a buffer.
 *
 * The CRC response consists of a firmware offset and a CRC value.
 *
 * @param[inout] p_buffer   The buffer to encode the response into.
 * @param[in]    fw_offset  The firmware offset value to encode.
 * @param[in]    crc        The CRC value to encode.
 *
 * @return The length added to the buffer.
 */
static uint32_t response_crc_add(uint8_t * p_buffer, uint32_t fw_offset, uint32_t crc)
{
    uint16_t offset = uint32_encode(fw_offset, &p_buffer[RESPONSE_HEADER_LEN]);
    offset         += uint32_encode(crc,       &p_buffer[RESPONSE_HEADER_LEN + offset]);
    return offset;
}


/**@brief Function for appending an extended error code to the response buffer.
 *
 * @param[inout] p_buffer    The buffer to append the extended error code to.
 * @param[in]    result      The error code to append.
 * @param[in]    buf_offset  The current length of the buffer.
 *
 * @return The length added to the buffer.
 */
static uint32_t response_ext_err_payload_add(uint8_t * p_buffer, uint8_t result, uint32_t buf_offset)
{
    p_buffer[buf_offset] = ext_error_get();
    (void) ext_error_set(NRF_DFU_EXT_ERROR_NO_ERROR);
    return 1;
}


void ble_dfu_req_handler_callback(nrf_dfu_response_t * p_res, void * p_context)
{
    __ASSERT_NO_MSG(p_res);
    // __ASSERT_NO_MSG(p_context);

    uint8_t len = 0;
    uint8_t buffer[MAX_RESPONSE_LEN] = {0};

    if (p_res->request == NRF_DFU_OP_OBJECT_WRITE)
    {
        --m_pkt_notif_target_cnt;
        if ((m_pkt_notif_target == 0) || (m_pkt_notif_target_cnt && m_pkt_notif_target > 0))
        {
            return;
        }

        /* Reply with a CRC message and reset the packet counter. */
        m_pkt_notif_target_cnt = m_pkt_notif_target;

        p_res->request = NRF_DFU_OP_CRC_GET;
    }

    len += response_prepare(buffer, p_res->request, p_res->result);

    if (p_res->result != NRF_DFU_RES_CODE_SUCCESS)
    {
        LOG_WRN("DFU request %d failed with error: 0x%x", p_res->request, p_res->result);

        if (p_res->result == NRF_DFU_RES_CODE_EXT_ERROR)
        {
            len += response_ext_err_payload_add(buffer, p_res->result, len);
        }

        (void) response_send(buffer, len);
        return;
    }

    switch (p_res->request)
    {
        case NRF_DFU_OP_OBJECT_CREATE:
        case NRF_DFU_OP_OBJECT_EXECUTE:
            break;

        case NRF_DFU_OP_OBJECT_SELECT:
        {
            len += response_select_obj_add(buffer,
                                           p_res->select.max_size,
                                           p_res->select.offset,
                                           p_res->select.crc);
        } break;

        case NRF_DFU_OP_OBJECT_WRITE:
        {
            len += response_crc_add(buffer, p_res->write.offset, p_res->write.crc);
        } break;

        case NRF_DFU_OP_CRC_GET:
        {
            len += response_crc_add(buffer, p_res->crc.offset, p_res->crc.crc);
        } break;

        default:
        {
            // No action.
        } break;
    }

    (void) response_send(buffer, len);
}


/**@brief     Function for handling a Write event on the Control Point characteristic.

 */
static ssize_t on_ctrl_pt_write(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  const void *buf,
			  uint16_t len,
			  uint16_t offset,
			  uint8_t flags)
{
    static bool conn_updated = false;
    uint8_t * data = (uint8_t *)buf;
    //lint -save -e415 -e416 : Out-of-bounds access on p_ble_write_evt->data
    nrf_dfu_request_t request =
    {
        .request           = (nrf_dfu_op_t)data[0],
        .p_context         = conn,
        .callback.response = ble_dfu_req_handler_callback,
        .write.len         = 0,
    };

    if (!conn_updated)
    {
        conn_updated = true;
		struct bt_le_conn_param param = {
			.interval_min = CONFIG_NRF_DFU_BLE_MIN_CONN_INTERVAL,
			.interval_max = CONFIG_NRF_DFU_BLE_MAX_CONN_INTERVAL,
			.latency = 0,
			.timeout = CONFIG_NRF_DFU_BLE_CONN_SUP_TIMEOUT,
		};
        LOG_INF("Initiate conn update min=%d max=%d", param.interval_min, param.interval_max);
		bt_conn_le_param_update(conn, &param);    
    }

    switch (request.request)
    {
        case NRF_DFU_OP_OBJECT_SELECT:
        {
            /* Set object type to read info about */
            request.select.object_type = data[1];
        } break;

        case NRF_DFU_OP_OBJECT_CREATE:
        {
            /* Reset the packet receipt notification on create object */
            m_pkt_notif_target_cnt = m_pkt_notif_target;

            request.create.object_type = data[1];
            request.create.object_size = uint32_decode(&(data[2]));
#ifndef CONFIG_NRF_DFU_RPC_NET
            if (request.create.object_type == NRF_DFU_OBJ_TYPE_COMMAND)
            {                
                /* Activity on the current transport. Close all except the current one. */
                (void) nrf_dfu_transports_close(&ble_dfu_transport);
            }
#endif            
        } break;

        case NRF_DFU_OP_RECEIPT_NOTIF_SET:
        {
            LOG_DBG("Set receipt notif");

            m_pkt_notif_target     = uint16_decode(&(data[1]));
            m_pkt_notif_target_cnt = m_pkt_notif_target;
        } break;

        default:
            break;
    }
    //lint -restore : Out-of-bounds access

    nrf_dfu_req_handler_on_req(&request);

	// uint8_t *value = attr->user_data;

	// memcpy(value + offset, buf, len);

	return len;    
}


static void flash_op_done(void * p_buf)
{

}


/**@brief   Function for handling the @ref BLE_GATTS_EVT_WRITE event from the SoftDevice.
 
 */
static ssize_t on_pkt_write(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  const void *buf,
			  uint16_t len,
			  uint16_t offset,
			  uint8_t flags)
{   
    /* Set up the request. */
    nrf_dfu_request_t request =
    {
        .request      = NRF_DFU_OP_OBJECT_WRITE,
        .p_context    = conn,
        .callback     =
        {
            .response = ble_dfu_req_handler_callback,
            .write    = flash_op_done,
        }
    };

    /* Set up the request buffer. */
    request.write.p_data   = (uint8_t *) buf;
    request.write.len      = len;
    
    /* Schedule handling of the request. */
    ret_code_t rc = nrf_dfu_req_handler_on_req(&request);
    if (rc != NRF_SUCCESS)
    {
        LOG_ERR("req handle err %d", rc);
    }
    return len;
}

uint32_t ble_dfu_transport_init(nrf_dfu_observer_t observer)
{
    uint32_t err_code = NRF_SUCCESS;

    if (m_flags & DFU_BLE_FLAG_INITIALIZED)
    {
        return err_code;
    }

    m_observer = observer;

    m_flags |= DFU_BLE_FLAG_INITIALIZED;

    LOG_DBG("BLE DFU transport initialized.");

    return NRF_SUCCESS;
}


uint32_t ble_dfu_transport_close(nrf_dfu_transport_t const * p_exception)
{
    uint32_t err_code = NRF_SUCCESS;

    if (m_flags & DFU_BLE_FLAG_INITIALIZED)
    {
        m_flags = 0;
    }
    return err_code;
}


// This is a 16-bit UUID.
#define BLE_DFU_SERVICE_UUID                 0xFE59                       //!< UUID of the DFU Service.

#define BT_UUID_DFU_SERVICE \
	BT_UUID_DECLARE_16(BLE_DFU_SERVICE_UUID)


#define BT_UUID_CTRL_PT   BT_UUID_DECLARE_128(0x50, 0xEA, 0xDA, 0x30, 0x88, 0x83, 0xB8, 0x9F, 0x60, 0x4F, 0x15, 0xF3, 0x01, 0x00, 0xC9, 0x8E)


#define BT_UUID_PKT   BT_UUID_DECLARE_128(0x50, 0xEA, 0xDA, 0x30, 0x88, 0x83, 0xB8, 0x9F, 0x60, 0x4F, 0x15, 0xF3, 0x02, 0x00, 0xC9, 0x8E)

/* UART Service Declaration */
BT_GATT_SERVICE_DEFINE(dfu_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_DFU_SERVICE),
	BT_GATT_CHARACTERISTIC(BT_UUID_CTRL_PT,
			       BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       NULL, on_ctrl_pt_write, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_PKT,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       NULL, on_pkt_write, NULL),
);


static int response_send(uint8_t * data, uint16_t len)
{
	struct bt_gatt_notify_params params = {0};
	const struct bt_gatt_attr *attr = &dfu_svc.attrs[2];

	params.attr = attr;
	params.data = data;
	params.len = len;
	params.func = NULL;

	return bt_gatt_notify_cb(NULL, &params);	
}
