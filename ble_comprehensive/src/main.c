/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <dk_buttons_and_leds.h>
#include <nrf.h>
#include <nrfx.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/services/nus.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/pm/device.h>

#define LOG_MODULE_NAME main
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)
#define KEY_PASSKEY_ACCEPT DK_BTN1_MSK
#define KEY_PASSKEY_REJECT DK_BTN2_MSK

#define ERASE_DELAY_AFTER_BOOT 30   //unit: s
static struct k_work_delayable blinky_work;
K_SEM_DEFINE(sem_spi_txrx, 0, 1);

static struct bt_conn *current_conn;
static struct bt_conn *auth_conn;
const struct device *runLED;
const struct device *conLED;
bool led_is_on = true;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

// static const struct bt_data sd[] = {
// 	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
// };

/* Stack definition for application workqueue */
K_THREAD_STACK_DEFINE(application_stack_area,
		      1024);
static struct k_work_q application_work_q;

#define LED0_NODE DT_ALIAS(led0)
#define LED0	DT_GPIO_CTLR(LED0_NODE, gpios)
#define LED0_PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define LED0_FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)

#define LED1_NODE DT_ALIAS(led1)
#define LED1	DT_GPIO_CTLR(LED1_NODE, gpios)
#define LED1_PIN	DT_GPIO_PIN(LED1_NODE, gpios)
#define LED1_FLAGS	DT_GPIO_FLAGS(LED1_NODE, gpios)

#ifdef CONFIG_PM_DEVICE
const struct device *devUart0;
const struct device *devUart1;
const struct device *devI2C;
const struct device *devSPI;
static void get_device_handles(void)
{
	devUart0 = DEVICE_DT_GET(DT_NODELABEL(uart0));
    devUart1 = DEVICE_DT_GET(DT_NODELABEL(uart1));
	devI2C = DEVICE_DT_GET(DT_ALIAS(myi2c));
    devSPI = DEVICE_DT_GET(DT_ALIAS(myspi));	
}

extern int my_uart_enable();
void set_device_pm_state(void)
{
	static bool is_off;
	int err = 0;
	
	if (is_off)
	{
		LOG_INF("Turning on UART/SPI/I2C");
		is_off = false;
		err = pm_device_action_run(devUart0, PM_DEVICE_ACTION_RESUME);
		err |= pm_device_action_run(devI2C,	PM_DEVICE_ACTION_RESUME);
		err |= pm_device_action_run(devSPI,	PM_DEVICE_ACTION_RESUME);		
		err |= pm_device_action_run(devUart1, PM_DEVICE_ACTION_RESUME);

		if (err) {
			LOG_ERR("Activating err %d", err);			
		}
		else
		{
			LOG_INF("Entered active state");
		}

#if CONFIG_UART_ASYNC_API && CONFIG_UART_1_NRF_HW_ASYNC
		my_uart_enable();
#endif

	}
	else
	{
		LOG_INF("Turning off UART/SPI/I2C to save power");
		is_off = true;
		err = pm_device_action_run(devI2C,	PM_DEVICE_ACTION_SUSPEND);
		err |= pm_device_action_run(devSPI,	PM_DEVICE_ACTION_SUSPEND);		

#if CONFIG_UART_ASYNC_API && CONFIG_UART_1_NRF_HW_ASYNC
		((const struct uart_driver_api *)devUart1->api)->rx_disable(devUart1);
#endif			
		err |= pm_device_action_run(devUart1, PM_DEVICE_ACTION_SUSPEND);
		if (err) {
			LOG_ERR("Entering low power err %d", err);			
		}
		else
		{
			LOG_INF("Entered lowe power");
		}			
		while(log_process());
		err = pm_device_action_run(devUart0, PM_DEVICE_ACTION_SUSPEND);
	}

}
#endif //CONFIG_PM_DEVICE

static void blinky_work_fn(struct k_work *work)
{
    //LOG_INF("blinky fn in system workqueue\n");
	gpio_pin_set(runLED, LED0_PIN, (int)led_is_on);
	led_is_on = !led_is_on;

	k_work_reschedule_for_queue(&application_work_q, &blinky_work,
					   	K_SECONDS(2));

}

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

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected %s", addr);

	current_conn = bt_conn_ref(conn);

#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
	if (bt_conn_set_security(conn, BT_SECURITY_L3)) {
		LOG_INF("Failed to set security\n");
	}	
#endif
#ifdef CONFIG_BT_GATT_CLIENT		
	exchange_params.func = exchange_func;
	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err) {
		LOG_ERR("MTU exchange failed");
	} 		
#endif

	gpio_pin_set(conLED, LED1_PIN, 1);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason %u)", addr, reason);

	if (auth_conn) {
		bt_conn_unref(auth_conn);
		auth_conn = NULL;
	}

	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
		gpio_pin_set(conLED, LED1_PIN, 0);
	}
}

#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", addr, level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d", addr,
			level, err);
	}
}
#endif

static void param_updated(struct bt_conn *conn, uint16_t interval,
				 uint16_t latency, uint16_t timeout)
{
	LOG_INF("conn interval=%d, latency=%d, timeout=%d", interval, latency, timeout);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected    = connected,
	.disconnected = disconnected,
	.le_param_updated = param_updated,
#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
	.security_changed = security_changed,
#endif
};

#if defined(CONFIG_BT_NUS_SECURITY_ENABLED)
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Passkey for %s: %06u", addr, passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	auth_conn = bt_conn_ref(conn);

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Passkey for %s: %06u", addr, passkey);
	LOG_INF("Press Button 1 to confirm, Button 2 to reject.");
}


static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", addr);
}


static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing completed: %s, bonded: %d", addr, bonded);
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing failed conn: %s, reason %d", addr, reason);
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

extern int my_uart_send(const uint8_t *buf, size_t len);

static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data,
			  uint16_t len)
{	
	char addr[BT_ADDR_LE_STR_LEN] = {0};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));

	LOG_INF("Received data from: %s", addr);

	my_uart_send(data, len);
	
}

static struct bt_nus_cb nus_cb = {
	.received = bt_receive_cb,
};

void error(void)
{
	dk_set_leds_state(DK_ALL_LEDS_MSK, DK_NO_LEDS_MSK);

	while (true) {
		/* Spin for ever */
		k_sleep(K_MSEC(1000));
	}
}

#if defined(CONFIG_BT_NUS_SECURITY_ENABLED)
static void num_comp_reply(bool accept)
{
	if (accept) {
		bt_conn_auth_passkey_confirm(auth_conn);
		LOG_INF("Numeric Match, conn %p", (void *)auth_conn);
	} else {
		bt_conn_auth_cancel(auth_conn);
		LOG_INF("Numeric Reject, conn %p", (void *)auth_conn);
	}

	bt_conn_unref(auth_conn);
	auth_conn = NULL;
}

#endif

void button_changed(uint32_t button_state, uint32_t has_changed)
{
	uint32_t buttons = button_state & has_changed;

	if (buttons & DK_BTN2_MSK) {
		LOG_INF("button2 isr");
		k_sem_give(&sem_spi_txrx);
	}

	if (buttons & DK_BTN1_MSK) {
		LOG_INF("button1 isr");
#ifdef CONFIG_PM_DEVICE		
		set_device_pm_state();
#endif		
	}

#if defined(CONFIG_BT_NUS_SECURITY_ENABLED)
	if (auth_conn) {
		if (buttons & KEY_PASSKEY_ACCEPT) {
			num_comp_reply(true);
		}

		if (buttons & KEY_PASSKEY_REJECT) {
			num_comp_reply(false);
		}
	}
#endif

}

void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	LOG_INF("MTU Updated: tx %d, rx %d\n", tx, rx);
}

static struct bt_gatt_cb mtu_cb = {
	.att_mtu_updated = att_mtu_updated,
};

int main(void)
{
	int err;	

	LOG_INF("### Comprehensive example v1.1 built at %s %s\n", __TIME__, __DATE__);

	runLED = DEVICE_DT_GET(LED0);
	if (runLED == NULL) {
		LOG_ERR("LED0 bind null\n");
	}
	err = gpio_pin_configure(runLED, LED0_PIN, GPIO_OUTPUT_ACTIVE | LED0_FLAGS);
	if (err < 0) {
		LOG_ERR("led0 configure error %d \n", err);		
	}

	conLED = DEVICE_DT_GET(LED1);
	if (conLED == NULL) {
		LOG_ERR("LED1 bind null\n");
	}
	err = gpio_pin_configure(conLED, LED1_PIN, GPIO_OUTPUT_ACTIVE | LED1_FLAGS);
	if (err < 0) {
		LOG_ERR("led1 configure error %d \n", err);		
	}
	gpio_pin_set(conLED, LED1_PIN, 0);	

	//only initialize buttons module. LED moudle is not used!
	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);
	}

#ifdef CONFIG_PM_DEVICE
	get_device_handles();
#endif

	k_work_queue_start(&application_work_q, application_stack_area,
			   K_THREAD_STACK_SIZEOF(application_stack_area), 10,
			   NULL);
	k_work_init_delayable(&blinky_work, blinky_work_fn);
	k_work_reschedule_for_queue(&application_work_q, &blinky_work,
					   	K_MSEC(20));	

	if (IS_ENABLED(CONFIG_BT_NUS_SECURITY_ENABLED)) {
		err = bt_conn_auth_cb_register(&conn_auth_callbacks);
		if (err) {
			printk("Failed to register authorization callbacks.\n");
			return 0;
		}

		err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
		if (err) {
			printk("Failed to register authorization info callbacks.\n");
			return 0;
		}
	}

	err = bt_enable(NULL);
	if (err) {
		error();
	}

	LOG_INF("Bluetooth initialized");

	bt_gatt_cb_register(&mtu_cb);

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_nus_init(&nus_cb);
	if (err) {
		LOG_ERR("Failed to initialize UART service (err: %d)", err);
		return err;
	}

	struct bt_le_adv_param adv_para;
	memset(&adv_para, 0, sizeof(struct bt_le_adv_param));
	adv_para.options = BT_LE_ADV_OPT_CONNECTABLE;
	adv_para.interval_min = 200;
	adv_para.interval_max = 300;

	err = bt_le_adv_start(&adv_para, ad, ARRAY_SIZE(ad), NULL,
			      0);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return err;
	}

	while(1)
	{
		//add your code here
		LOG_INF("main thread");
		k_sleep(K_SECONDS(20));		
	}

	return 0;
}
