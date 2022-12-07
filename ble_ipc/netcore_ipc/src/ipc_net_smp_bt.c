/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 * @brief Bluetooth transport for the mcumgr SMP protocol.
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include "ipc_net_api.h"

LOG_MODULE_REGISTER(ipc_smp_bt, 3);

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

/**
 * Write handler for the SMP characteristic; processes an incoming SMP request.
 */
static ssize_t smp_bt_chr_write(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	LOG_HEXDUMP_DBG(buf, len, "smp bt rx:");
	net2app_bt_smp_send((uint8_t *)buf, len);
	return len;
}

static void smp_bt_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	net2app_send_bt_mtu();
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
void ipc_net_bt_smp_send(struct bt_conn *conn, void *data, uint16_t len)
{	
	LOG_HEXDUMP_DBG(data, len, "bt notify: ");
	bt_gatt_notify(conn, smp_bt_attrs + 2, data, len);	
}

int smp_bt_register_ipc(void)
{
	printk("register smp bt\r");
	return bt_gatt_service_register(&smp_bt_svc);
}

int smp_bt_unregister_ipc(void)
{
	return bt_gatt_service_unregister(&smp_bt_svc);
}
