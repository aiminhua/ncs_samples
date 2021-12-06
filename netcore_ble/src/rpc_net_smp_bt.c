/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 * @brief Bluetooth transport for the mcumgr SMP protocol.
 */

#include <errno.h>

#include <zephyr.h>
#include <init.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <tinycbor/cbor.h>
#include <nrf_rpc_cbor.h>
#include "common_ids.h"
#include <logging/log.h>

LOG_MODULE_REGISTER(rpc_smp_bt, 2);

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

	cbor_encode_int(&ctx.encoder, err_code);

	nrf_rpc_cbor_rsp_no_err(&ctx);
}

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

int rpc_net_bt_smp_receive_cb(const uint8_t *buffer, uint16_t length)
{
	int err;
	int result;
	struct nrf_rpc_cbor_ctx ctx;

	if (!buffer || length < 1) {
		LOG_ERR("net rpc rx len error");
		return -NRF_EINVAL;
	}	

	NRF_RPC_CBOR_ALLOC(ctx, CBOR_BUF_SIZE + length);
	
	cbor_encode_byte_string(&ctx.encoder, buffer, length);	

	err = nrf_rpc_cbor_cmd(&rpc_smp, RPC_COMMAND_NET_BT_SMP_RECEIVE_CB, &ctx,
			       rsp_error_code_handle, &result);
	if (err < 0) {
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
	LOG_HEXDUMP_INF(buf, len, "rpc smp bt rx:");
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
static void rpc_net_bt_smp_send(CborValue *packet, void *handler_data)
{
	CborError cbor_err;
	int err;
	size_t length;
#ifdef CONFIG_BT_L2CAP_TX_MTU	
	uint8_t buf[CONFIG_BT_L2CAP_TX_MTU];
#else
	uint8_t buf[260];
#endif

	length = sizeof(buf);
	cbor_err = cbor_value_copy_byte_string(packet, buf, &length,
					       NULL);
	if (cbor_err != CborNoError || length < 0 || length > sizeof(buf)) {
		LOG_ERR("net rpc send len err");
		err = -EBADMSG;		
	}
	else
	{
		LOG_HEXDUMP_INF(buf, length, "rpc smp bt tx rsp:");
		err = bt_gatt_notify(current_conn, smp_bt_attrs + 2, buf, length);
	}

	nrf_rpc_cbor_decoding_done(packet);

	rsp_error_code_send(err);

}

NRF_RPC_CBOR_CMD_DECODER(rpc_smp, rpc_net_bt_smp_send_xx,
			 RPC_COMMAND_APP_BT_SMP_SEND,
			 rpc_net_bt_smp_send, NULL);

static void rpc_net_bt_smp_get_mtu(CborValue *packet, void *handler_data)
{
	uint16_t mtu;

	nrf_rpc_cbor_decoding_done(packet);

	mtu = bt_gatt_get_mtu(current_conn) - 3;

	rsp_error_code_send(mtu);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_smp, rpc_net_bt_smp_get_mtu_xx,
			 RPC_COMMAND_APP_BT_SMP_GET_MTU,
			 rpc_net_bt_smp_get_mtu, NULL);

int smp_bt_register_rpc(void)
{
	LOG_INF("register smp bt");
	return bt_gatt_service_register(&smp_bt_svc);
}

int smp_bt_unregister_rpc(void)
{
	return bt_gatt_service_unregister(&smp_bt_svc);
}
