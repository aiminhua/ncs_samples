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
#include <mgmt/mcumgr/buf.h>
#include <mgmt/mcumgr/smp.h>
#include <net/buf.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(rpc_app_smp, 2);

#define CBOR_BUF_SIZE 16
static struct zephyr_smp_transport smp_rpc_transport;

NRF_RPC_GROUP_DEFINE(rpc_smp, "rpc_smp", NULL, NULL, NULL);

static void rsp_error_code_send(int err_code)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE);

	cbor_encode_int(&ctx.encoder, err_code);

	nrf_rpc_cbor_rsp_no_err(&ctx);
}

static int smp_receive_data(const void *buf, uint16_t len)
{
	struct net_buf *nb;

	LOG_HEXDUMP_INF(buf, len, "rpc app smp rx:");
	nb = mcumgr_buf_alloc();
	if (!nb)
	{
		LOG_ERR("net buf alloc failed");
		return -ENOMEM;
	}
	net_buf_add_mem(nb, buf, len);
	zephyr_smp_rx_req(&smp_rpc_transport, nb);	

	return len;
}

static void rpc_app_bt_smp_receive_cb(CborValue *value, void *handler_data)
{	
	CborError cbor_err;
	int err;
	size_t length;
	uint8_t buf[270];

	LOG_INF("rpc rx data");
	err = 0;
	length = sizeof(buf);
	cbor_err = cbor_value_copy_byte_string(value, buf, &length, NULL);

	if (cbor_err != CborNoError || length < 0 || length > sizeof(buf)) {
		err = -NRF_EBADMSG;
		LOG_ERR("cbor copy err in smp rx");		
	}
	else
	{		
		err = smp_receive_data(buf, length);
	}

	nrf_rpc_cbor_decoding_done(value);

	rsp_error_code_send(err);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_smp, rpc_app_bt_smp_receive,
			 RPC_COMMAND_NET_BT_SMP_RECEIVE_CB,
			 rpc_app_bt_smp_receive_cb, NULL);

/*

static uint16_t m_rpc_mtu = 23;

static uint16_t smp_rpc_get_mtu(const struct net_buf *nb)
{
	return m_rpc_mtu;
}

static void rpc_app_bt_mtu_size_cb(CborValue *value, void *handler_data)
{	
	CborError cbor_err;
	int err;
	uint16_t mtu;

	err = 0;
	cbor_err = cbor_value_get_int(value, &mtu);
	if (cbor_err != CborNoError) {
		err = -NRF_EBADMSG;
	}

	m_rpc_mtu = mtu;

	nrf_rpc_cbor_decoding_done(value);

	rsp_error_code_send(err);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_smp, rpc_app_bt_mtu_size,
			 RPC_COMMAND_NET_BT_MTU_SIZE_CB,
			 rpc_app_bt_mtu_size_cb, NULL);

*/

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

int rpc_app_bt_smp_send(uint8_t *buffer, uint16_t length)
{
	int err;
	int result;
	struct nrf_rpc_cbor_ctx ctx;

	if (!buffer || length < 1) {
		LOG_ERR("rpc app send err");
		return -NRF_EINVAL;
	}	

	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE + length);
	
	cbor_encode_byte_string(&ctx.encoder, buffer, length);	

	err = nrf_rpc_cbor_cmd(&rpc_smp, RPC_COMMAND_APP_BT_SMP_SEND, &ctx,
			       rsp_error_code_handle, &result);
	if (err < 0) {
		return err;
	}

	return result;
}
/**
 * Transmits the specified SMP response.
 */
static int smp_rpc_tx_pkt(struct zephyr_smp_transport *zst, struct net_buf *nb)
{	
	int rc;
	LOG_HEXDUMP_INF(nb->data, nb->len, "rpc app send");
	rc = rpc_app_bt_smp_send(nb->data, nb->len);
	mcumgr_buf_free(nb);
	return rc;
}

int rpc_app_bt_smp_get_mtu(void)
{
	int err;
	int result;
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE);

	err = nrf_rpc_cbor_cmd(&rpc_smp, RPC_COMMAND_APP_BT_SMP_GET_MTU, &ctx,
			       rsp_error_code_handle, &result);
	if (err < 0) {
		return err;
	}

	return result;
}

static uint16_t smp_rpc_get_mtu(const struct net_buf *nb)
{
	int mtu = rpc_app_bt_smp_get_mtu();
	return mtu < 0 ? 0: mtu;     
}

int smp_rpc_init(const struct device *dev)
{	
	ARG_UNUSED(dev);
	
	zephyr_smp_transport_init(&smp_rpc_transport, smp_rpc_tx_pkt,
				  smp_rpc_get_mtu, NULL,
				  NULL);
	return 0;
}

SYS_INIT(smp_rpc_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
