/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <errno.h>
#include <zephyr/init.h>
#include <nrf_rpc_cbor.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include "common_ids.h"
#include <zephyr/logging/log.h>
#include "nrf_dfu_transport.h"
#include "nrf_dfu_req_handler.h"
#include "nrf_dfu_handling_error.h"
#include "app_util.h"

#define LOG_MODULE_NAME rpc_dfu_app
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_NRF_DFU_LOG_LEVEL);

#define CBOR_BUF_SIZE 16

void rpc_dfu_req_handler_callback(nrf_dfu_response_t * p_res, void * p_context);

NRF_RPC_GROUP_DEFINE(rpc_dfu, "rpc_dfu", NULL, NULL, NULL);

static void rsp_error_code_send(int err_code)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE);
	zcbor_int32_put(ctx.zs, err_code);

	nrf_rpc_cbor_rsp_no_err(&ctx);
}

static void rpc_nrf_dfu_req_handler_on_req(struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{	
	struct zcbor_string zst = {0};
	int err;
	uint8_t buf[512];
    nrf_dfu_request_t req;
	
	if (!zcbor_bstr_decode(ctx->zs, &zst) || zst.len > sizeof(buf)) {
		err = -NRF_EBADMSG;
		LOG_ERR("cbor decode err len %d", zst.len);			
	}
	else
	{
		memcpy(buf, zst.value, zst.len);
		memcpy((uint8_t *)&req, buf, sizeof(nrf_dfu_request_t));
		if (req.request == NRF_DFU_OP_OBJECT_WRITE)
		{	
			req.write.p_data = &buf[sizeof(nrf_dfu_request_t)];		
		}
		req.callback.response =  rpc_dfu_req_handler_callback;
		req.callback.write = NULL;   	
			
		LOG_DBG("req_op %d req_type %d", req.request, req.select.object_type);
		err = nrf_dfu_req_handler_on_req(&req);
	}

	nrf_rpc_cbor_decoding_done(ctx);	
	rsp_error_code_send(err);
  
}

NRF_RPC_CBOR_CMD_DECODER(rpc_dfu, rpc_nrf_dfu_req_handler_on_req_cb,
			 RPC_CMD_NRF_DFU_REQ_HANDLER_ON_REQ,
			 rpc_nrf_dfu_req_handler_on_req, NULL);


static void rsp_error_code_handle(struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int32_t val;

	if (zcbor_int32_decode(ctx->zs, &val)) {
		*(int *)handler_data = (int)val;
	} else {
		*(int *)handler_data = -NRF_EINVAL;
	}
}

void rpc_dfu_req_handler_callback(nrf_dfu_response_t * p_res, void * p_context)
{
	int err;
	int result;
	struct nrf_rpc_cbor_ctx ctx;

    __ASSERT_NO_MSG(p_res);

	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE + sizeof(nrf_dfu_response_t));
	
	zcbor_bstr_encode_ptr(ctx.zs, (const uint8_t *)p_res, sizeof(nrf_dfu_response_t));		

	err = nrf_rpc_cbor_cmd(&rpc_dfu, RPC_CMD_DFU_REQ_HANDLER_CALLBACK, &ctx,
			       rsp_error_code_handle, &result);
	
    LOG_DBG("req callback err=%d, ret=%d", err, result);
	// if (err) {
	// 	return err;
	// }
	// return result; 
}
