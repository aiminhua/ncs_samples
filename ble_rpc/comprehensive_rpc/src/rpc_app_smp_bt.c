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
#include <zephyr/mgmt/mcumgr/buf.h>
#include <zephyr/mgmt/mcumgr/smp.h>
#include "smp_reassembly.h"
#include <zephyr/net/buf.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rpc_app_smp, 3);

#define CBOR_BUF_SIZE 16
static struct zephyr_smp_transport smp_rpc_transport;

NRF_RPC_GROUP_DEFINE(rpc_smp, "rpc_smp", NULL, NULL, NULL);

static void rsp_error_code_send(int err_code)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE);
	zcbor_int32_put(ctx.zs, err_code);

	nrf_rpc_cbor_rsp_no_err(&ctx);
}

static int smp_receive_data(const void *buf, uint16_t len)
{
	LOG_HEXDUMP_DBG(buf, len, "rpc app smp rx:");

#ifdef CONFIG_RPC_REASSEMBLY_BT
	int ret;
	bool started;

	started = (zephyr_smp_reassembly_expected(&smp_rpc_transport) >= 0);
	ret = zephyr_smp_reassembly_collect(&smp_rpc_transport, buf, len);

	LOG_DBG("collect = %d", ret);

	/*
	 * Collection can fail only due to failing to allocate memory or by receiving
	 * more data than expected.
	 */
	if (ret == -ENOMEM) {
		/* Failed to collect the buffer */
		return -ENOMEM;
	} else if (ret < 0) {
		zephyr_smp_reassembly_drop(&smp_rpc_transport);
		return ret;
	}

	/* No more bytes are expected for this packet */
	if (ret == 0) {
		zephyr_smp_reassembly_complete(&smp_rpc_transport, false);
	}

	return 0;
#else
	struct net_buf *nb;	

	nb = mcumgr_buf_alloc();
	if (!nb)
	{
		LOG_ERR("net buf alloc failed");
		return -ENOMEM;
	}
	net_buf_add_mem(nb, buf, len);
	zephyr_smp_rx_req(&smp_rpc_transport, nb);	

	return 0;
#endif	
}

static void rpc_app_bt_smp_receive_cb(struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{	
	struct zcbor_string zst = {0};
	int err;	
	uint8_t buf[512];

	if (!zcbor_bstr_decode(ctx->zs, &zst) || zst.len > sizeof(buf)) {
		err = -NRF_EBADMSG;
		LOG_ERR("cbor decode err len %x", zst.len);		
	}
	else
	{	
		LOG_DBG("decoded done len %x", zst.len);
		memcpy(buf, zst.value, zst.len);	
		err = smp_receive_data(buf, zst.len);
	}

	nrf_rpc_cbor_decoding_done(ctx);

	rsp_error_code_send(err);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_smp, rpc_app_bt_smp_receive,
			 RPC_COMMAND_NET_BT_SMP_RECEIVE_CB,
			 rpc_app_bt_smp_receive_cb, NULL);


static void rsp_error_code_handle(struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int32_t val;

	if (zcbor_int32_decode(ctx->zs, &val)) {
		*(int *)handler_data = (int)val;
	} else {
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
	
	zcbor_bstr_encode_ptr(ctx.zs, buffer, length);	

	err = nrf_rpc_cbor_cmd(&rpc_smp, RPC_COMMAND_APP_BT_SMP_SEND, &ctx,
			       rsp_error_code_handle, &result);
	if (err) {
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
	LOG_HEXDUMP_DBG(nb->data, nb->len, "rpc app send");
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
