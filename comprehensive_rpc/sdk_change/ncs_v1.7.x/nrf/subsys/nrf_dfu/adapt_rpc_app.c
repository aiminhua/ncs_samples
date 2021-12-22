/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <errno.h>
#include <init.h>
#include <tinycbor/cbor.h>
#include <nrf_rpc_cbor.h>
#include "common_ids.h"
#include <logging/log.h>
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

	cbor_encode_int(&ctx.encoder, err_code);

	nrf_rpc_cbor_rsp_no_err(&ctx);
}

static void rpc_nrf_dfu_req_handler_on_req(CborValue *packet, void *handler_data)
{	
	CborError cbor_err;
	int err;
	size_t len;
	uint8_t buf[300];
    nrf_dfu_request_t req;
	
	err = 0;
	len = sizeof(buf);
	cbor_err = cbor_value_copy_byte_string(packet, buf, &len, NULL);	
	if (cbor_err != CborNoError || len < sizeof(nrf_dfu_request_t)) {		
		goto cbor_error_exit;	
	}

	memcpy((uint8_t *)&req, buf, sizeof(nrf_dfu_request_t));

	if (req.request == NRF_DFU_OP_OBJECT_WRITE)
	{	
		req.write.p_data = &buf[sizeof(nrf_dfu_request_t)];		
	}

	nrf_rpc_cbor_decoding_done(packet);

    req.callback.response =  rpc_dfu_req_handler_callback;
    req.callback.write = NULL;   	
		
	LOG_DBG("req_op %d req_type %d", req.request, req.select.object_type);
	err = nrf_dfu_req_handler_on_req(&req);
	
	rsp_error_code_send(err);

	return;

cbor_error_exit:
	LOG_ERR("rpc app err");
	nrf_rpc_cbor_decoding_done(packet);	    
}

NRF_RPC_CBOR_CMD_DECODER(rpc_dfu, rpc_nrf_dfu_req_handler_on_req_cb,
			 RPC_CMD_NRF_DFU_REQ_HANDLER_ON_REQ,
			 rpc_nrf_dfu_req_handler_on_req, NULL);


static void rsp_error_code_handle(CborValue *packet, void *handler_data)
{
	CborError cbor_err;

	if (!cbor_value_is_integer(packet)) {
		*(int *)handler_data = -NRF_EINVAL;
	}

	cbor_err = cbor_value_get_int(packet, (int *)handler_data);
	if (cbor_err != CborNoError) {
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
	
	cbor_encode_byte_string(&ctx.encoder, (const uint8_t *)p_res, sizeof(nrf_dfu_response_t));	

	err = nrf_rpc_cbor_cmd(&rpc_dfu, RPC_CMD_DFU_REQ_HANDLER_CALLBACK, &ctx,
			       rsp_error_code_handle, &result);
	
    LOG_DBG("req callback err=%d, ret=%d", err, result);
 
}
