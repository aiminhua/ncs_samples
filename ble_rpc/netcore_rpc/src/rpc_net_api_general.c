/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <errno.h>
#include <zephyr/init.h>
#include <string.h>
#include <nrf_rpc/nrf_rpc_ipc.h>
#include <nrf_rpc_cbor.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include "common_ids.h"
#include "rpc_net_api.h"
#include <bluetooth/services/nus.h>
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME rpc_net_api
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define CBOR_BUF_SIZE 16

NRF_RPC_IPC_TRANSPORT(rpc_api_tr, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "nrf_rpc_ept_2");
NRF_RPC_GROUP_DEFINE(rpc_api, "RPC_API", &rpc_api_tr, NULL, NULL, NULL);

static void rsp_error_code_send(const struct nrf_rpc_group *group, int err_code)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(group, ctx, CBOR_BUF_SIZE);
	zcbor_int32_put(ctx.zs, err_code);

	nrf_rpc_cbor_rsp_no_err(group, &ctx);
}

static void rpc_app2net_send(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct zcbor_string zst;
	int err = -NRF_EINVAL;
	size_t length;
	uint8_t buf[256];
	int type;

	if (!zcbor_int32_decode(ctx->zs, &type)) {
		goto cbor_error_exit;
	}

	if (!zcbor_int32_decode(ctx->zs, &length) || length > ARRAY_SIZE(buf)) {
		goto cbor_error_exit;
	}
	
	if (!zcbor_bstr_decode(ctx->zs, &zst) || zst.len > ARRAY_SIZE(buf)) {
		goto cbor_error_exit;
	}
	length = zst.len;
	memcpy(buf, zst.value, zst.len);

	switch (type)
	{
		case APP2NET_BT_NUS_SEND:
			LOG_HEXDUMP_INF(buf, length, "nus data to send:");
			err = bt_nus_send(NULL, buf, length);
			break;
		default:
			break;
	}

cbor_error_exit:
	nrf_rpc_cbor_decoding_done(group, ctx);
	rsp_error_code_send(group, err);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_api, rpc_app2net_send_name,
			 RPC_COMMAND_APP_SEND,
			 rpc_app2net_send, NULL);

static void rsp_error_code_handle(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int32_t val;

	if (zcbor_int32_decode(ctx->zs, &val)) {
		*(int *)handler_data = (int)val;
	} else {
		*(int *)handler_data = -NRF_EINVAL;
	}
}

int rpc_net2app_send(int type, uint8_t *buffer, size_t length)
{
	int err;
	int result;
	struct nrf_rpc_cbor_ctx ctx;

	if (!buffer || length < 1) {
		return -NRF_EINVAL;
	}	

	NRF_RPC_CBOR_ALLOC(&rpc_api, ctx, CBOR_BUF_SIZE + length);
	zcbor_int32_put(ctx.zs, type);
	zcbor_int32_put(ctx.zs, length);
	zcbor_bstr_encode_ptr(ctx.zs, buffer, length);	

	err = nrf_rpc_cbor_cmd(&rpc_api, RPC_COMMAND_NET_SEND, &ctx,
			       rsp_error_code_handle, &result);
	if (err) {
		return err;
	}

	return result;
}


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