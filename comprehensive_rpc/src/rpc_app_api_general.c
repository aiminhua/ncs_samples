/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <errno.h>
#include <zephyr/init.h>
#include <string.h>
#include <nrf_rpc_cbor.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include "common_ids.h"
#include <logging/log.h>
#include "rpc_app_api.h"

#define LOG_MODULE_NAME rpc_app_api_general
LOG_MODULE_REGISTER(LOG_MODULE_NAME);


#define CBOR_BUF_SIZE 16


NRF_RPC_GROUP_DEFINE(rpc_api, "rpc_api", NULL, NULL, NULL);

static void rsp_error_code_handle(struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int32_t val;

	if (zcbor_int32_decode(ctx->zs, &val)) {
		*(int *)handler_data = (int)val;
	} else {
		*(int *)handler_data = -NRF_EINVAL;
	}
}

static void rsp_error_code_send(int err_code)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE);
	zcbor_int32_put(ctx.zs, err_code);

	nrf_rpc_cbor_rsp_no_err(&ctx);
}

int rpc_app2net_send(int32_t type, uint8_t *buffer, int32_t length)
{
	int err;
	int32_t result = 0;
	struct nrf_rpc_cbor_ctx ctx;

	printk("buf:%p, len:%d \r", buffer, length);
	if (!buffer || length < 1) {
		return -NRF_EINVAL;
	}	

	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE + length);
	
	zcbor_int32_put(ctx.zs, type);
	zcbor_int32_put(ctx.zs, length);
	zcbor_bstr_encode_ptr(ctx.zs, buffer, length);	

	err = nrf_rpc_cbor_cmd(&rpc_api, RPC_COMMAND_APP_SEND, &ctx,
			       rsp_error_code_handle, &result);
	printk("err %d result %d\r", err, result);
	if (err) {
		return err;
	}

	return result;
}

static void rpc_net2app_send(struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{	
	struct zcbor_string zst;
	int err_code = -NRF_EINVAL;
	size_t length;
	uint8_t buf[256];
	int type;
	

	if (!zcbor_int32_decode(ctx->zs, &type)) {
		goto cbor_error_exit;
	}

	if (!zcbor_int32_decode(ctx->zs, &length) || length > ARRAY_SIZE(buf)) {
		goto cbor_error_exit;
	}
	
	if (!zcbor_bstr_decode(ctx->zs, &zst) || zst.len != length) {
		goto cbor_error_exit;
	}
	memcpy(buf, zst.value, zst.len);
	err_code = net2app_receive(type, buf, length);

cbor_error_exit:
	nrf_rpc_cbor_decoding_done(ctx);
	rsp_error_code_send(err_code);
}


NRF_RPC_CBOR_CMD_DECODER(rpc_api, rpc_net2app_send_name,
			 RPC_COMMAND_NET_SEND,
			 rpc_net2app_send, NULL);


static void err_handler(const struct nrf_rpc_err_report *report)
{
	printk("nRF RPC error %d ocurred. See nRF RPC logs for more details.",
	       report->code);
	k_oops();
}


static int serialization_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	int err;

	printk("Init begin\n");

	err = nrf_rpc_init(err_handler);
	if (err) {
		return -NRF_EINVAL;
	}

	printk("Init done\n");

	return 0;
}


SYS_INIT(serialization_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
