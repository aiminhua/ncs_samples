/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */
#include <uart_async_adapter.h>

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <soc.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <host/hci_core.h>
#include <bluetooth/services/nus.h>
#include <zephyr/settings/settings.h>

#include <stdio.h>
#include <string.h>

#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME peripheral_uart
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define STACKSIZE CONFIG_BT_NUS_THREAD_STACK_SIZE
#define PRIORITY 7

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)

#define UART_BUF_SIZE CONFIG_BT_NUS_UART_BUFFER_SIZE
#define UART_WAIT_FOR_BUF_DELAY K_MSEC(50)
#define UART_WAIT_FOR_RX CONFIG_BT_NUS_UART_RX_WAIT_TIME

// static struct bt_conn *current_conn;
// static struct bt_conn *auth_conn;

static const struct device *uart = DEVICE_DT_GET(DT_CHOSEN(nordic_nus_uart));
static struct k_work_delayable uart_work;
struct k_work_delayable adv_work;
enum CON_ADV_TYPE {
	CON_ADV_INVALID = 0,	
	CON_ADV_COMMON = 1,
	CON_ADV_MATTER = 2	
};
// int conn_type;
int restart_adv;
static struct bt_le_ext_adv *adv_common;
static struct bt_le_ext_adv *adv_matter;
struct bt_conn *conn_id[CONFIG_BT_MAX_CONN];  //0: common connection/adv  1: matter connection/adv

struct uart_data_t {
	void *fifo_reserved;
	uint8_t data[UART_BUF_SIZE];
	uint16_t len;
};

static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);

static uint8_t mfg_data[] = { 0xFF, 0xFF, 0x30,0x31,0x32,0x33};
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, "Common", sizeof("Common") - 1),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)),
};

#ifdef CONFIG_UART_ASYNC_ADAPTER
UART_ASYNC_ADAPTER_INST_DEFINE(async_adapter);
#else
#define async_adapter NULL
#endif

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);

	static size_t aborted_len;
	struct uart_data_t *buf;
	static uint8_t *aborted_buf;
	static bool disable_req;

	switch (evt->type) {
	case UART_TX_DONE:
		LOG_DBG("UART_TX_DONE");
		if ((evt->data.tx.len == 0) ||
		    (!evt->data.tx.buf)) {
			return;
		}

		if (aborted_buf) {
			buf = CONTAINER_OF(aborted_buf, struct uart_data_t,
					   data[0]);
			aborted_buf = NULL;
			aborted_len = 0;
		} else {
			buf = CONTAINER_OF(evt->data.tx.buf, struct uart_data_t,
					   data[0]);
		}

		k_free(buf);

		buf = k_fifo_get(&fifo_uart_tx_data, K_NO_WAIT);
		if (!buf) {
			return;
		}

		if (uart_tx(uart, buf->data, buf->len, SYS_FOREVER_MS)) {
			printk("Failed to send data over UART");
		}

		break;

	case UART_RX_RDY:
		LOG_DBG("UART_RX_RDY");
		buf = CONTAINER_OF(evt->data.rx.buf, struct uart_data_t, data[0]);
		buf->len += evt->data.rx.len;

		if (disable_req) {
			return;
		}

		if ((evt->data.rx.buf[buf->len - 1] == '\n') ||
		    (evt->data.rx.buf[buf->len - 1] == '\r')) {
			disable_req = true;
			uart_rx_disable(uart);
		}

		break;

	case UART_RX_DISABLED:
		LOG_DBG("UART_RX_DISABLED");
		disable_req = false;

		buf = k_malloc(sizeof(*buf));
		if (buf) {
			buf->len = 0;
		} else {
			printk("Not able to allocate UART receive buffer");
			k_work_reschedule(&uart_work, UART_WAIT_FOR_BUF_DELAY);
			return;
		}

		uart_rx_enable(uart, buf->data, sizeof(buf->data),
			       UART_WAIT_FOR_RX);

		break;

	case UART_RX_BUF_REQUEST:
		LOG_DBG("UART_RX_BUF_REQUEST");
		buf = k_malloc(sizeof(*buf));
		if (buf) {
			buf->len = 0;
			uart_rx_buf_rsp(uart, buf->data, sizeof(buf->data));
		} else {
			printk("Not able to allocate UART receive buffer");
		}

		break;

	case UART_RX_BUF_RELEASED:
		LOG_DBG("UART_RX_BUF_RELEASED");
		buf = CONTAINER_OF(evt->data.rx_buf.buf, struct uart_data_t,
				   data[0]);

		if (buf->len > 0) {
			k_fifo_put(&fifo_uart_rx_data, buf);
		} else {
			k_free(buf);
		}

		break;

	case UART_TX_ABORTED:
		LOG_DBG("UART_TX_ABORTED");
		if (!aborted_buf) {
			aborted_buf = (uint8_t *)evt->data.tx.buf;
		}

		aborted_len += evt->data.tx.len;
		buf = CONTAINER_OF((void *)aborted_buf, struct uart_data_t,
				   data);

		uart_tx(uart, &buf->data[aborted_len],
			buf->len - aborted_len, SYS_FOREVER_MS);

		break;

	default:
		break;
	}
}

static void uart_work_handler(struct k_work *item)
{
	struct uart_data_t *buf;

	buf = k_malloc(sizeof(*buf));
	if (buf) {
		buf->len = 0;
	} else {
		printk("Not able to allocate UART receive buffer");
		k_work_reschedule(&uart_work, UART_WAIT_FOR_BUF_DELAY);
		return;
	}

	uart_rx_enable(uart, buf->data, sizeof(buf->data), UART_WAIT_FOR_RX);
}

static bool uart_test_async_api(const struct device *dev)
{
	const struct uart_driver_api *api =
			(const struct uart_driver_api *)dev->api;

	return (api->callback_set != NULL);
}

static int uart_init(void)
{
	int err;
	int pos;
	struct uart_data_t *rx;
	struct uart_data_t *tx;

	if (!device_is_ready(uart)) {
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_USB_DEVICE_STACK)) {
		err = usb_enable(NULL);
		if (err && (err != -EALREADY)) {
			printk("Failed to enable USB");
			return err;
		}
	}

	rx = k_malloc(sizeof(*rx));
	if (rx) {
		rx->len = 0;
	} else {
		return -ENOMEM;
	}

	k_work_init_delayable(&uart_work, uart_work_handler);


	if (IS_ENABLED(CONFIG_UART_ASYNC_ADAPTER) && !uart_test_async_api(uart)) {
		/* Implement API adapter */
		uart_async_adapter_init(async_adapter, uart);
		uart = async_adapter;
	}

	err = uart_callback_set(uart, uart_cb, NULL);
	if (err) {
		k_free(rx);
		printk("Cannot initialize UART callback");
		return err;
	}

	if (IS_ENABLED(CONFIG_UART_LINE_CTRL)) {
		printk("Wait for DTR");
		while (true) {
			uint32_t dtr = 0;

			uart_line_ctrl_get(uart, UART_LINE_CTRL_DTR, &dtr);
			if (dtr) {
				break;
			}
			/* Give CPU resources to low priority threads. */
			k_sleep(K_MSEC(100));
		}
		printk("DTR set");
		err = uart_line_ctrl_set(uart, UART_LINE_CTRL_DCD, 1);
		if (err) {
			printk("Failed to set DCD, ret code %d", err);
		}
		err = uart_line_ctrl_set(uart, UART_LINE_CTRL_DSR, 1);
		if (err) {
			printk("Failed to set DSR, ret code %d", err);
		}
	}

	tx = k_malloc(sizeof(*tx));

	if (tx) {
		pos = snprintf(tx->data, sizeof(tx->data),
			       "Starting Nordic UART service example\r\n");

		if ((pos < 0) || (pos >= sizeof(tx->data))) {
			k_free(rx);
			k_free(tx);
			printk("snprintf returned %d", pos);
			return -ENOMEM;
		}

		tx->len = pos;
	} else {
		k_free(rx);
		return -ENOMEM;
	}

	err = uart_tx(uart, tx->data, tx->len, SYS_FOREVER_MS);
	if (err) {
		k_free(rx);
		k_free(tx);
		printk("Cannot display welcome message (err: %d)", err);
		return err;
	}

	err = uart_rx_enable(uart, rx->data, sizeof(rx->data), UART_WAIT_FOR_RX);
	if (err) {
		printk("Cannot enable uart reception (err: %d)", err);
		/* Free the rx buffer only because the tx buffer will be handled in the callback */
		k_free(rx);
	}

	return err;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	
	if (err) {
		printk("Connection failed (err %u)", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Connected %s\n", addr);

	// current_conn = bt_conn_ref(conn);

}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	k_msleep(2);
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason %u)\n", addr, reason);

	if (conn_id[0] == conn)
	{		
		restart_adv = CON_ADV_COMMON;
		k_work_reschedule(&adv_work, K_MSEC(100));		
		conn_id[0] = NULL;
	}
}

#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u", addr, level);
	} else {
		printk("Security failed: %s level %u err %d", addr,
			level, err);
	}
}
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected    = connected,
	.disconnected = disconnected,
#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
	.security_changed = security_changed,
#endif
};

#if defined(CONFIG_BT_NUS_SECURITY_ENABLED)
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u", addr, passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	auth_conn = bt_conn_ref(conn);

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u", addr, passkey);
	printk("Press Button 1 to confirm, Button 2 to reject.");
}


static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s", addr);
}


static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing completed: %s, bonded: %d", addr, bonded);
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing failed conn: %s, reason %d", addr, reason);
}


static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_display = auth_passkey_display,
	.passkey_confirm = auth_passkey_confirm,
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};
#else
static struct bt_conn_auth_cb conn_auth_callbacks;
static struct bt_conn_auth_info_cb conn_auth_info_callbacks;
#endif

static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data,
			  uint16_t len)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN] = {0};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));

	printk("Received data from: %s", addr);

	for (uint16_t pos = 0; pos != len;) {
		struct uart_data_t *tx = k_malloc(sizeof(*tx));

		if (!tx) {
			printk("Not able to allocate UART send data buffer");
			return;
		}

		/* Keep the last byte of TX buffer for potential LF char. */
		size_t tx_data_size = sizeof(tx->data) - 1;

		if ((len - pos) > tx_data_size) {
			tx->len = tx_data_size;
		} else {
			tx->len = (len - pos);
		}

		memcpy(tx->data, &data[pos], tx->len);

		pos += tx->len;

		/* Append the LF character when the CR character triggered
		 * transmission from the peer.
		 */
		if ((pos == len) && (data[len - 1] == '\r')) {
			tx->data[tx->len] = '\n';
			tx->len++;
		}

		err = uart_tx(uart, tx->data, tx->len, SYS_FOREVER_MS);
		if (err) {
			k_fifo_put(&fifo_uart_tx_data, tx);
		}
	}
}

static struct bt_nus_cb nus_cb = {
	.received = bt_receive_cb,
};

#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
static void num_comp_reply(bool accept)
{
	if (accept) {
		bt_conn_auth_passkey_confirm(auth_conn);
		printk("Numeric Match, conn %p", (void *)auth_conn);
	} else {
		bt_conn_auth_cancel(auth_conn);
		printk("Numeric Reject, conn %p", (void *)auth_conn);
	}

	bt_conn_unref(auth_conn);
	auth_conn = NULL;
}

void button_changed(uint32_t button_state, uint32_t has_changed)
{
	uint32_t buttons = button_state & has_changed;

	if (auth_conn) {
		if (buttons & KEY_PASSKEY_ACCEPT) {
			num_comp_reply(true);
		}

		if (buttons & KEY_PASSKEY_REJECT) {
			num_comp_reply(false);
		}
	}
}
#endif /* CONFIG_BT_NUS_SECURITY_ENABLED */


void adv_common_connected_cb(struct bt_le_ext_adv *extadv,
			  struct bt_le_ext_adv_connected_info *info)
{
	conn_id[0] = info->conn;	
	__ASSERT(extadv == adv_common, "Common adv wrong");
	printk("## Common adv connected %p ##\n", info->conn);

	struct bt_le_conn_param param = {
		.interval_min = 500,
		.interval_max = 600,
		.latency = 0,
		.timeout = 400,
	};
	printk("Initiate conn update min=%d max=%d\n", param.interval_min, param.interval_max);
	bt_conn_le_param_update(info->conn, &param); 		
	
}

void adv_matter_connected_cb(struct bt_le_ext_adv *extadv,
			  struct bt_le_ext_adv_connected_info *info)
{
	conn_id[1] = info->conn;	
	__ASSERT(extadv == adv_matter, "Matter adv wrong");
	printk("## Matter adv connected %p ##\n", info->conn);
	
}

struct bt_le_ext_adv_cb adv_common_cb = {
	.connected = adv_common_connected_cb
};
struct bt_le_ext_adv_cb adv_matter_cb = {
	.connected = adv_matter_connected_cb
};

const uint8_t MAC_ADDR[6] = {0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC};
int matter_set_public_addr(void)
{
	int err = 0;
	struct net_buf *buf;
	struct bt_hci_cp_vs_write_bd_addr *cmd_params;

	buf = bt_hci_cmd_create(BT_HCI_OP_VS_WRITE_BD_ADDR, sizeof(*cmd_params));
	if (!buf)
	{
		printk("Could not allocate command buffer\n");
		return -ENOMEM;
	}

	cmd_params = net_buf_add(buf, sizeof(*cmd_params));
	memcpy((void *)cmd_params->bdaddr.val, (void *)MAC_ADDR, 6);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_WRITE_BD_ADDR, buf, NULL);
	if (err)
	{
		printk("bt_hci_cmd_send_sync err: %d \n", err);
		return err;
	}

	err = bt_setup_public_id_addr();
	if (err)
	{
		printk("bt_setup_public_id_addr err: %d\n", err);
		return err;
	}

	printk("Successfully set public addr \n");
	
	return 0;
}

int stop_matter_adv(void) 
{
    int err;

	if (adv_matter->cb == &adv_matter_cb)
	{
		err = bt_le_ext_adv_stop(adv_matter);
		if (err) {
			printk("Stop Matter adv (err %d)\n", err);
			return err;			
		}
	}
    
    return 0;
}

int start_matter_adv(struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len)
{
    int err;
	
	if (adv_matter->cb != &adv_matter_cb)
	{	
		param->id = 1;
		param->sid = 0;
		err = bt_le_ext_adv_create(param, NULL, &adv_matter);
		if (err) {
			printk("Create Matter adv (err %d)\n", err);
			return err;		
		}	
		adv_matter->cb = &adv_matter_cb;		
	}		

	// err = bt_le_ext_adv_stop(adv_matter);
	// if (err) {
	// 	printk("Stop Matter adv (err %d)\n", err);
	// 	return err;			
	// }

	err = bt_le_ext_adv_set_data(adv_matter, ad , ad_len, sd, sd_len);
	if (err) {
		printk("Set Matter adv data (err %d)\n", err);
		return err;			
	}	

	err = bt_le_ext_adv_update_param(adv_matter, param);
	if (err) {
		printk("Update Matter adv param (err %d)\n", err);
		return err;			
	}			

    err = bt_le_ext_adv_start(adv_matter, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Start Matter adv (err %d)\n", err);
		return err;			
	}

	printk("## Matter adv started ##\n");
	return 0;
}


int start_common_adv(void)
{

	struct bt_le_adv_param adv_param0 = 
		{
			.id = 0,
			.sid = 0U, /* Supply unique SID when creating advertising set */
			.secondary_max_skip = 0U,
			.options = (BT_LE_ADV_OPT_USE_IDENTITY | BT_LE_ADV_OPT_CONNECTABLE),
			.interval_min = 600,
			.interval_max = 800,
			.peer = NULL,
		};

	int err;

	err = bt_le_ext_adv_create(&adv_param0, NULL, &adv_common);
	if (err) {
		printk("Create common adv (err %d)\n", err);
		return err;
	}

	err = bt_le_ext_adv_set_data(adv_common, ad , ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Set common adv data (err %d)\n", err);
		return err;
	}
	adv_common->cb = &adv_common_cb;

	/* Start extended advertising set */
	err = bt_le_ext_adv_start(adv_common,
				BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Start common adv (err %d)\n", err);
		return err;
	}

	printk("## Common adv started ##\n");

	return 0;
}

static void adv_work_handler(struct k_work *work)
{
	int err;
	
	if (restart_adv == CON_ADV_COMMON)
	{		
		// atomic_set_bit(adv_common->flags, BT_ADV_ENABLED);
		// err = bt_le_ext_adv_stop(adv_common);
		// if (err) {
		// 	printk("Stop common adv (err %d)\n", err);					
		// }					
		err = bt_le_ext_adv_start(adv_common,
					BT_LE_EXT_ADV_START_DEFAULT);
		if (err) {
			printk("Restart common adv (err %d)\n", err);			
		}
		else
		{
			printk("## Common adv Restarted ##\n");
		}
	}
	// else if (restart_adv == CON_ADV_MATTER)
	// {
	// 	err = bt_le_ext_adv_stop(adv_matter);
	// 	if (err) {
	// 		printk("Stop Matter adv (err %d)\n", err);
	// 		return err;			
	// 	}		
	// 	err = bt_le_ext_adv_start(adv_matter,
	// 				BT_LE_EXT_ADV_START_DEFAULT);
	// 	if (err) {
	// 		printk("Restart Matter adv (err %d)\n", err);			
	// 	}
	// 	else
	// 	{
	// 		printk("\n## Matter adv Restarted ##\n");
	// 	}
	// }
	else
	{
		start_common_adv();
	}

	restart_adv = CON_ADV_INVALID;

}


void ble_write_thread(void)
{
	int err = 0;
	struct uart_data_t nus_data = {
		.len = 0,
	};

	err = uart_init();
	if (err) {
		printk("uart init err %d\n", err);
	}

	if (IS_ENABLED(CONFIG_BT_NUS_SECURITY_ENABLED)) {
		err = bt_conn_auth_cb_register(&conn_auth_callbacks);
		if (err) {
			printk("Failed to register authorization callbacks.\n");
			return;
		}

		err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
		if (err) {
			printk("Failed to register authorization info callbacks.\n");
			return;
		}
	}

	err = bt_nus_init(&nus_cb);
	if (err) {
		printk("Failed to initialize UART service (err: %d)\n", err);
		return;
	}

	k_work_init_delayable(&adv_work, adv_work_handler);
	k_work_schedule(&adv_work, K_MSEC(200));	

	for (;;) {
		/* Wait indefinitely for data to be sent over bluetooth */
		struct uart_data_t *buf = k_fifo_get(&fifo_uart_rx_data,
						     K_FOREVER);

		int plen = MIN(sizeof(nus_data.data) - nus_data.len, buf->len);
		int loc = 0;

		while (plen > 0) {
			memcpy(&nus_data.data[nus_data.len], &buf->data[loc], plen);
			nus_data.len += plen;
			loc += plen;

			if (nus_data.len >= sizeof(nus_data.data) ||
			   (nus_data.data[nus_data.len - 1] == '\n') ||
			   (nus_data.data[nus_data.len - 1] == '\r')) {
				if (bt_nus_send(NULL, nus_data.data, nus_data.len)) {
					printk("Failed to send data over BLE connection");
				}
				nus_data.len = 0;
			}

			plen = MIN(sizeof(nus_data.data), buf->len - loc);
		}

		k_free(buf);
	}
}

K_THREAD_DEFINE(ble_write_thread_id, STACKSIZE, ble_write_thread, NULL, NULL,
		NULL, PRIORITY, 0, 0);
