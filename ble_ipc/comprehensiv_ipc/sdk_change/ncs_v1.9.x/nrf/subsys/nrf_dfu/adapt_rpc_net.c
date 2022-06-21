/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 * @brief Bluetooth transport for the mcumgr SMP protocol.
 */

#include <errno.h>

#include <zephyr.h>
#include <init.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <tinycbor/cbor.h>
#include <nrf_rpc_cbor.h>
#include "common_ids.h"
#include <logging/log.h>
#include "nrf_dfu_transport.h"
#include "nrf_dfu_req_handler.h"
#include "nrf_dfu_handling_error.h"
#include "app_util.h"

#define LOG_MODULE_NAME rpc_dfu_net
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_NRF_DFU_LOG_LEVEL);

typedef struct {
	void *fifo_reserved;
	nrf_dfu_request_t req;	
}dfu_data_t;

static K_FIFO_DEFINE(fifo_dfu_data);

#define CBOR_BUF_SIZE 16

NRF_RPC_GROUP_DEFINE(rpc_dfu, "rpc_dfu", NULL, NULL, NULL);

static void rsp_error_code_send(int err_code)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE);

	cbor_encode_int(&ctx.encoder, err_code);

	nrf_rpc_cbor_rsp_no_err(&ctx);
}

static void rsp_error_code_handle(CborValue *value, void *handler_data)
{
	CborError cbor_err;

	if (!cbor_value_is_integer(value)) {
		*(int *)handler_data = -NRF_EINVAL;
	}

	cbor_err = cbor_value_get_int(value, (int *)handler_data);
	if (cbor_err != CborNoError) {
		*(int *)handler_data = -NRF_EINVAL;
	}
}


uint32_t nrf_dfu_req_handler_on_req(nrf_dfu_request_t * p_req)
{
	int err;
	int result = 0;
	struct nrf_rpc_cbor_ctx ctx;
	uint8_t buf[300];
	uint16_t len;
	
	__ASSERT_NO_MSG(p_req);

	len = sizeof(nrf_dfu_request_t);
	memcpy(buf, p_req, len);	

	LOG_DBG("req_op %d req_type %x len %d", p_req->request, p_req->select.object_type, p_req->write.len);
	
	if(p_req->request == NRF_DFU_OP_OBJECT_WRITE)
	{		
		memcpy(buf + sizeof(nrf_dfu_request_t), p_req->write.p_data, p_req->write.len);
		len += p_req->write.len;		
	}

	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE + len);

	cbor_encode_byte_string(&ctx.encoder, buf, len);	

	err = nrf_rpc_cbor_cmd(&rpc_dfu, RPC_CMD_NRF_DFU_REQ_HANDLER_ON_REQ, &ctx,
			       rsp_error_code_handle, &result);
	
	if (err < 0) {
		LOG_ERR("net rpc cbor cmd err %d", err);
		return err;
	}
	
	return result;
}


static void rpc_dfu_req_handler_callback(CborValue *packet, void *handler_data)
{
	CborError cbor_err;
	int err = 0;
	size_t length;
	nrf_dfu_response_t response;

	length = sizeof(nrf_dfu_response_t);
	cbor_err = cbor_value_copy_byte_string(packet, (uint8_t *)&response, &length,
					       NULL);
	if (cbor_err != CborNoError || length != sizeof(nrf_dfu_response_t)) {
		LOG_ERR("net rpc cbor err");
		err = -EBADMSG;		
	}
	else
	{		
		ble_dfu_req_handler_callback(&response, NULL);
	}

	nrf_rpc_cbor_decoding_done(packet);

	rsp_error_code_send(err);

}

NRF_RPC_CBOR_CMD_DECODER(rpc_dfu, rpc_dfu_req_handler_cb,
			 RPC_CMD_DFU_REQ_HANDLER_CALLBACK,
			 rpc_dfu_req_handler_callback, NULL);
