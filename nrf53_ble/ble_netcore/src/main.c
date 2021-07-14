/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */

#include <zephyr/types.h>
#include <zephyr.h>
#include <drivers/uart.h>

#include <device.h>
#include <soc.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>

#include <bluetooth/services/nus.h>

// #include <dk_buttons_and_leds.h>

#include <settings/settings.h>

#include <stdio.h>
#include <logging/log.h>
#include "rpc_net_nus.h"
#include "rpc_net_smp_bt.h"
#include "rpc_net_api.h"

#ifdef CONFIG_RPC_SIMULATE_UART
#include "nrf_rpc_tr.h"
#endif

#include <drivers/gpio.h>

#ifdef CONFIG_EXAMPLE_DFU_OTA
#include "rpc_net_smp_bt.h"
#endif

#define LOG_MODULE_NAME peripheral_uart
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define STACKSIZE CONFIG_BT_NUS_THREAD_STACK_SIZE
#define PRIORITY 7

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)

#define KEY_PASSKEY_ACCEPT DK_BTN1_MSK
#define KEY_PASSKEY_REJECT DK_BTN2_MSK


static K_SEM_DEFINE(ble_conn_ok, 0, 1);

struct bt_conn *current_conn;


static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/*
static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};
*/


K_SEM_DEFINE(sem_nus_op, 0, 1);

const struct device *gpio_dev;
#define EXT_INT_IO	9  //p0.09
#define EXT_INT_CONF	(GPIO_INPUT | GPIO_PULL_UP)
static struct gpio_callback ext_int_cb_data;

#ifdef CONFIG_BT_GATT_CLIENT
static struct bt_gatt_exchange_params exchange_params;

static void exchange_func(struct bt_conn *conn, uint8_t att_err,
			  struct bt_gatt_exchange_params *params)
{
	if (att_err)
	{
		LOG_ERR("MTU exchange failed");
	}
	else
	{
		LOG_INF("MTU updated to %d", bt_gatt_get_mtu(conn)); 
	}
	
}
#endif

/*
* format: 0x55 CMD LEN Payload 0xAA (Checksum)
* 0x55 0x01 0x01 0x00 0xAA  //connected
* 0x55 0x01 0x01 0x01 0xAA  //disconnected
*/

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];	

	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected %s", log_strdup(addr));

	current_conn = bt_conn_ref(conn);

	k_sem_give(&ble_conn_ok);

#ifdef CONFIG_RPC_REMOTE_API
	err = net2app_send_conn_status(1);
	if (err) {
		LOG_ERR("send connected err %d", err);
	}	
#endif
#ifdef CONFIG_RPC_SIMULATE_UART
	int ret;
	uint8_t data[] = {0x55, 0x01, 0x01, 0x00, 0xAA};
	size_t len = 5;

	ret = nrf_rpc_tr_send(data, len);
	if (ret) {
		printk("connected @ rpc failed: %d\n", ret);			
	}
#endif

#ifdef CONFIG_BT_GATT_CLIENT		
	exchange_params.func = exchange_func;
	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err) {
		LOG_ERR("MTU exchange failed");
	} 		
#endif
	
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason %u)", log_strdup(addr), reason);

	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;		
	}

#ifdef CONFIG_RPC_REMOTE_API
	int err = net2app_send_conn_status(0);
	if (err) {
		LOG_ERR("send disconnected err %d", err);
	}	
#endif

#ifdef CONFIG_RPC_SIMULATE_UART
	int ret;
	uint8_t data[] = {0x55, 0x01, 0x01, 0x01, 0xAA};
	size_t len = 5;

	ret = nrf_rpc_tr_send(data, len);
	if (ret) {
		printk("disconnected @ rpc failed: %d\n", ret);			
	}
#endif		
}


static struct bt_conn_cb conn_callbacks = {
	.connected    = connected,
	.disconnected = disconnected,
};


static void bt_nus_receive_cb(struct bt_conn *conn, const uint8_t *const data,
			  uint16_t len)
{	
	char addr[BT_ADDR_LE_STR_LEN] = {0};
	int err;	

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));

	LOG_INF("Received data from: %s", log_strdup(addr));
	LOG_HEXDUMP_INF(data, len, "data:");

#ifdef CONFIG_RPC_NUS_DEDICATE
	rpc_net_bt_nus_receive_cb(data, len);
#endif
#ifdef CONFIG_RPC_REMOTE_API	
	err = net2app_send_nus((uint8_t *)data, len);
	if (err) {
		LOG_ERR("network to application core nus send err %d", err);
	}	
#endif

#ifdef CONFIG_RPC_SIMULATE_UART
		uint8_t * data1 = (uint8_t *)data;
		err = nrf_rpc_tr_send(data1, len);
		if (err) {
			printk("nrf_rpc_tr_send netcore failed: %d\n", err);			
		}
		else
		{
			printk("nrf_rpc_tr_send success netcore\n");
		}
#endif

}

static struct bt_nus_cb nus_cb = {
	.received = bt_nus_receive_cb,
};

#ifdef CONFIG_RPC_SIMULATE_UART
/* Callback from transport layer that handles incoming. */
static void rpc_receive_handler(const uint8_t *packet, size_t len)
{
	uint16_t i;
	int err;
	uint16_t nus_len;

	printk("received data from app core by RPC \n");
	for (i = 0; i < len; i++) {
		printk("%x", packet[i]);
	}
	printk("\n");

	if (!NRF_RPC_TR_AUTO_FREE_RX_BUF) {
		nrf_rpc_tr_free_rx_buf(packet);
	}

	nus_len = len;

	if (nus_len > (bt_gatt_get_mtu(current_conn)-3))
	{
		LOG_WRN("RPC data length is greater than MTU size");
		nus_len = bt_gatt_get_mtu(current_conn)-3;
	}
	err = bt_nus_send(current_conn, packet, nus_len);
	if (err) {
		LOG_WRN("bt_nus_send @ rpc err %d", err);
	}
}
#endif

void ext_int_isr(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	gpio_pin_interrupt_configure(gpio_dev, EXT_INT_IO, GPIO_INT_DISABLE);
	LOG_INF("button4 pressed and going to send nus packet\n");
    k_sem_give(&sem_nus_op);    
}

void setup_ext_int(void)
{
	int err;

	gpio_dev = device_get_binding("GPIO_0");
	if (gpio_dev == NULL) {
		printk("GPIO_0 bind error");
		return;
	}
	err = gpio_pin_configure(gpio_dev, EXT_INT_IO,
				EXT_INT_CONF);
	if (err) {
		printk("GPIO_0 config error: %d", err);
		return;
	}

	err = gpio_pin_interrupt_configure(gpio_dev, EXT_INT_IO,
					   GPIO_INT_EDGE_TO_INACTIVE);
	if (err) {
		printk("GPIO_0 enable callback error: %d", err);
	}

	gpio_init_callback(&ext_int_cb_data, ext_int_isr,
			BIT(EXT_INT_IO));
	err = gpio_add_callback(gpio_dev, &ext_int_cb_data);
	if (err) {
		printk("GPIO_0 add callback error: %d", err);
		return;
	}
}


// static void button_changed(uint32_t button_state, uint32_t has_changed)
// {
// 	uint32_t buttons = button_state & has_changed;
	
// 	if (buttons & DK_BTN4_MSK) {
// 		LOG_INF("button4 isr and going to send a nus packet");
// 		 k_sem_give(&sem_nus_op);	
// 	}	
// }


void main(void)
{
	
	int err = 0;
	
	LOG_INF("### netcore firmware compiled at %s %s\n", __TIME__, __DATE__);

	setup_ext_int();
	// err = dk_buttons_init(button_changed);
	// if (err) {
	// 	LOG_ERR("Cannot init buttons (err: %d)", err);
	// }


#ifdef CONFIG_RPC_SIMULATE_UART
	err = nrf_rpc_tr_init(rpc_receive_handler);
	if (err < 0) {
		printk("rpc tr init error %d \n", err);
		return;
	}	
#endif

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("bt initialized error");
		return;
	}

#ifdef CONFIG_EXAMPLE_DFU_OTA
	/* Initialize the Bluetooth mcumgr transport. */
	smp_bt_register_rpc();
#endif

	LOG_INF("Bluetooth initialized");	

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_nus_init(&nus_cb);
	if (err) {
		LOG_ERR("Failed to initialize UART service (err: %d)", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL,
			      0);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
	}

	LOG_INF("Started NUS example on netcore");

#ifdef CONFIG_RPC_REMOTE_API
	err = net2app_send_bt_addr();
	if (err) {
		LOG_ERR("BT dev addr sent err %d", err);
	}
#endif		

	for (;;) {		
		k_sleep(K_SECONDS(1));	
	}
}

void ble_write_thread(void)
{
	static char data[20];
	static uint8_t cnt;
	int err;

	/* Don't go any further until BLE is initialized */
	k_sem_take(&ble_conn_ok, K_FOREVER);
	cnt = 0;

	for (;;) {
		k_sem_take(&sem_nus_op, K_FOREVER);		
		snprintf(data, 12, "HelloNet%d", cnt++);		
		data[11] = 0;		
		err = bt_nus_send(NULL, data, 12);
		if (err) {
			LOG_WRN("bt_nus_send err %d", err);
		}
		k_msleep(100);
		//debouce button press
		gpio_pin_interrupt_configure(gpio_dev, EXT_INT_IO, GPIO_INT_EDGE_TO_INACTIVE);				
	}
}

K_THREAD_DEFINE(ble_write_thread_id, STACKSIZE, ble_write_thread, NULL, NULL,
		NULL, PRIORITY, 0, 0);
