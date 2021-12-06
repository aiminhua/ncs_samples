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
#include "rpc_net_api.h"
#include <bluetooth/services/nus.h>
#include <logging/log.h>

#define LOG_MODULE_NAME rpc_net_api
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define CBOR_BUF_SIZE 16


NRF_RPC_GROUP_DEFINE(rpc_api, "rpc_api", NULL, NULL, NULL);

static void rpc_app2net_send(CborValue *packet, void *handler_data)
{
	CborError cbor_err;
	int err;
	size_t length;
	uint8_t buf[256];
	int type;

	err = cbor_value_get_int(packet, &type);
	if (err != CborNoError) {
		goto cbor_error_exit;
	}

	err = cbor_value_advance(packet);
	if (err != CborNoError) {
		goto cbor_error_exit;
	}

	err = cbor_value_get_int(packet, &length);
	if (err != CborNoError) {
		goto cbor_error_exit;
	}

	err = cbor_value_advance(packet);
	if (err != CborNoError) {
		goto cbor_error_exit;
	}	

	length = 256;
	cbor_err = cbor_value_copy_byte_string(packet, buf, &length,
					       NULL);
	if (cbor_err != CborNoError || length < 0 || length > sizeof(buf)) {
		goto cbor_error_exit;	
	}

	nrf_rpc_cbor_decoding_done(packet);

	switch (type)
	{
		case APP2NET_BT_NUS_SEND:
			err = bt_nus_send(NULL, buf, length);
			break;
		case APP2NET_BT_SMP_SEND:
			
			break;
		default:
			break;
	}
	struct nrf_rpc_cbor_ctx ctx;
	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE);
	cbor_encode_int(&ctx.encoder, err);	
	nrf_rpc_cbor_rsp_no_err(&ctx);
	return;

cbor_error_exit:
	nrf_rpc_cbor_decoding_done(packet);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_api, rpc_app2net_send_name,
			 RPC_COMMAND_APP_SEND,
			 rpc_app2net_send, NULL);

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

int rpc_net2app_send(int type, uint8_t *buffer, size_t length)
{
	int err;
	int result;
	struct nrf_rpc_cbor_ctx ctx;

	if (!buffer || length < 1) {
		return -NRF_EINVAL;
	}	

	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE + length);

	
	cbor_encode_int(&ctx.encoder, type);

	cbor_encode_int(&ctx.encoder, length);

	cbor_encode_byte_string(&ctx.encoder, buffer, length);	

	err = nrf_rpc_cbor_cmd(&rpc_api, RPC_COMMAND_NET_SEND, &ctx,
			       rsp_error_code_handle, &result);
	if (err < 0) {
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

	err = nrf_rpc_init(err_handler);
	if (err) {
		return -NRF_EINVAL;
	}
	printk("netcore handshake done");

	return 0;
}


SYS_INIT(serialization_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
