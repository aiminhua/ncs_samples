/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 * @brief Bluetooth transport for the mcumgr SMP protocol.
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <nrf_rpc_cbor.h>
#include "common_ids.h"
#include <zephyr/logging/log.h>
#include "nrf_dfu_transport.h"
#include "nrf_dfu_req_handler.h"
#include "nrf_dfu_handling_error.h"
#include "app_util.h"

#define LOG_MODULE_NAME rpc_dfu_net
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_NRF_DFU_LOG_LEVEL);

#define CBOR_BUF_SIZE 16

NRF_RPC_GROUP_DEFINE(rpc_dfu, "rpc_dfu", NULL, NULL, NULL);

static void rsp_error_code_send(int err_code)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE);
	zcbor_int32_put(ctx.zs, err_code);

	nrf_rpc_cbor_rsp_no_err(&ctx);
}

static void rsp_error_code_handle(struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int32_t val;

	if (zcbor_int32_decode(ctx->zs, &val)) {
		*(int *)handler_data = (int)val;
	} else {
		*(int *)handler_data = -NRF_EINVAL;
	}
}

uint32_t nrf_dfu_req_handler_on_req(nrf_dfu_request_t * p_req)
{
	int err;
	int result = 0;
	struct nrf_rpc_cbor_ctx ctx;
	uint8_t buf[512];
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

	zcbor_bstr_encode_ptr(ctx.zs, buf, len);	

	err = nrf_rpc_cbor_cmd(&rpc_dfu, RPC_CMD_NRF_DFU_REQ_HANDLER_ON_REQ, &ctx,
			       rsp_error_code_handle, &result);
	
	if (err) {
		LOG_ERR("net rpc cbor cmd err %d", err);
		return err;
	}	
	return result;
}


static void rpc_dfu_req_handler_callback(struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct zcbor_string zst = {0};
	int err = 0;

	nrf_dfu_response_t response;

	if (!zcbor_bstr_decode(ctx->zs, &zst) || zst.len != sizeof(nrf_dfu_response_t)) {
		err = -NRF_EBADMSG;
		LOG_ERR("cbor decode err len %x", zst.len);			
	}	
	else
	{
		memcpy(&response, zst.value, zst.len);		
		ble_dfu_req_handler_callback(&response, NULL);
		err = 0;
	}

	nrf_rpc_cbor_decoding_done(ctx);
	rsp_error_code_send(err);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_dfu, rpc_dfu_req_handler_cb,
			 RPC_CMD_DFU_REQ_HANDLER_CALLBACK,
			 rpc_dfu_req_handler_callback, NULL);
