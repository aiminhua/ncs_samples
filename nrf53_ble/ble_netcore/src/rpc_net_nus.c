/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <errno.h>
#include <init.h>

#include <tinycbor/cbor.h>

#include <nrf_rpc_cbor.h>

#include "../../common_ids.h"
#include <bluetooth/services/nus.h>

#define CBOR_BUF_SIZE 16


NRF_RPC_GROUP_DEFINE(rpc_nus, "rpc_nus", NULL, NULL, NULL);

static void rpc_net_bt_nus_send(CborValue *packet, void *handler_data)
{
	CborError cbor_err;
	int err;
	size_t length;
	uint8_t buf[CONFIG_BT_L2CAP_TX_MTU];

	length = sizeof(buf);
	cbor_err = cbor_value_copy_byte_string(packet, buf, &length,
					       NULL);
	if (cbor_err != CborNoError || length < 0 || length > sizeof(buf)) {
		err = -EBADMSG;		
	}
	else
	{
		err = bt_nus_send(NULL, buf, length);
	}

	nrf_rpc_cbor_decoding_done(packet);

	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE);

	cbor_encode_int(&ctx.encoder, err);	

	nrf_rpc_cbor_rsp_no_err(&ctx);

}

NRF_RPC_CBOR_CMD_DECODER(rpc_nus, rpc_net_bt_nus_send_name,
			 RPC_COMMAND_APP_BT_NUS_SEND,
			 rpc_net_bt_nus_send, NULL);

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

int rpc_net_bt_nus_receive_cb(const uint8_t *buffer, uint16_t length)
{
	int err;
	int result;
	struct nrf_rpc_cbor_ctx ctx;

	if (!buffer || length < 1) {
		return -NRF_EINVAL;
	}	

	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE + length);
	
	cbor_encode_byte_string(&ctx.encoder, buffer, length);	

	err = nrf_rpc_cbor_cmd(&rpc_nus, RPC_COMMAND_NET_BT_NUS_RECEIVE_CB, &ctx,
			       rsp_error_code_handle, &result);
	if (err < 0) {
		return err;
	}

	return result;
}
