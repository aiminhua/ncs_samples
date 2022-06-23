/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 * @brief Bluetooth transport for the mcumgr SMP protocol.
 */

#include <errno.h>

#include <zephyr/zephyr.h>
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

LOG_MODULE_REGISTER(rpc_smp_bt, 3);

extern struct bt_conn *current_conn;

/* SMP service.
 * {8D53DC1D-1DB7-4CD3-868B-8A527460AA84}
 */
static struct bt_uuid_128 smp_bt_svc_uuid = BT_UUID_INIT_128(
	0x84, 0xaa, 0x60, 0x74, 0x52, 0x8a, 0x8b, 0x86,
	0xd3, 0x4c, 0xb7, 0x1d, 0x1d, 0xdc, 0x53, 0x8d);

/* SMP characteristic; used for both requests and responses.
 * {DA2E7828-FBCE-4E01-AE9E-261174997C48}
 */
static struct bt_uuid_128 smp_bt_chr_uuid = BT_UUID_INIT_128(
	0x48, 0x7c, 0x99, 0x74, 0x11, 0x26, 0x9e, 0xae,
	0x01, 0x4e, 0xce, 0xfb, 0x28, 0x78, 0x2e, 0xda);

#define CBOR_BUF_SIZE 16

NRF_RPC_GROUP_DEFINE(rpc_smp, "rpc_smp", NULL, NULL, NULL);

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

int rpc_net_bt_smp_receive_cb(const uint8_t *buffer, uint16_t length)
{
	int err;
	int result;
	struct nrf_rpc_cbor_ctx ctx;

	if (!buffer || length < 1) {
		LOG_ERR("net rpc rx len error");
		return -NRF_EINVAL;
	}	
	printk("smp rec len %x\r", length);
	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE + length);
	zcbor_bstr_encode_ptr(ctx.zs, buffer, length);
	
	err = nrf_rpc_cbor_cmd(&rpc_smp, RPC_COMMAND_NET_BT_SMP_RECEIVE_CB, &ctx,
			       rsp_error_code_handle, &result);
	if (err) {
		LOG_ERR("net rpc cbor cmd err %d", err);
		return err;
	}

	return result;
}

/**
 * Write handler for the SMP characteristic; processes an incoming SMP request.
 */
static ssize_t smp_bt_chr_write(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	LOG_HEXDUMP_DBG(buf, len, "rpc smp bt rx:");
	rpc_net_bt_smp_receive_cb(buf, len);
	return len;
}

static void smp_bt_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
}

static struct bt_gatt_attr smp_bt_attrs[] = {
	/* SMP Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&smp_bt_svc_uuid),

	BT_GATT_CHARACTERISTIC(&smp_bt_chr_uuid.uuid,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP |
			       BT_GATT_CHRC_NOTIFY,
#ifdef CONFIG_MCUMGR_SMP_BT_AUTHEN
			       BT_GATT_PERM_WRITE_AUTHEN,
#else
			       BT_GATT_PERM_WRITE,
#endif
			       NULL, smp_bt_chr_write, NULL),
	BT_GATT_CCC(smp_bt_ccc_changed,
#ifdef CONFIG_MCUMGR_SMP_BT_AUTHEN
			       BT_GATT_PERM_READ_AUTHEN |
			       BT_GATT_PERM_WRITE_AUTHEN),
#else
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
#endif
};

static struct bt_gatt_service smp_bt_svc = BT_GATT_SERVICE(smp_bt_attrs);

/**
 * Transmits an SMP response over the specified Bluetooth connection.
 */
static void rpc_net_bt_smp_send(struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct zcbor_string zst;
	int err;

#ifdef CONFIG_BT_L2CAP_TX_MTU	
	uint8_t buf[CONFIG_BT_L2CAP_TX_MTU];
#else
	uint8_t buf[256];
#endif

	if (!zcbor_bstr_decode(ctx->zs, &zst) || zst.len > sizeof(buf)) {
		LOG_ERR("net rpc send len err %d", zst.len);
		err = -EBADMSG;
	}
	else
	{
		memcpy(buf, zst.value, zst.len);
		LOG_HEXDUMP_DBG(buf, zst.len, "rpc smp bt tx rsp:");
		err = bt_gatt_notify(current_conn, smp_bt_attrs + 2, buf, zst.len);		
	}
	
	nrf_rpc_cbor_decoding_done(ctx);

	rsp_error_code_send(err);

}

NRF_RPC_CBOR_CMD_DECODER(rpc_smp, rpc_net_bt_smp_send_xx,
			 RPC_COMMAND_APP_BT_SMP_SEND,
			 rpc_net_bt_smp_send, NULL);

static void rpc_net_bt_smp_get_mtu(struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint16_t mtu;

	nrf_rpc_cbor_decoding_done(ctx);

	mtu = bt_gatt_get_mtu(current_conn) - 3;
	LOG_INF("smp MTU:%d", mtu);

	rsp_error_code_send(mtu);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_smp, rpc_net_bt_smp_get_mtu_name,
			 RPC_COMMAND_APP_BT_SMP_GET_MTU,
			 rpc_net_bt_smp_get_mtu, NULL);

int smp_bt_register_rpc(void)
{
	printk("register smp bt\r");
	return bt_gatt_service_register(&smp_bt_svc);
}

int smp_bt_unregister_rpc(void)
{
	return bt_gatt_service_unregister(&smp_bt_svc);
}
