/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <stdio.h>
#include <dk_buttons_and_leds.h>
#include <nrf.h>
#include <nrfx.h>
#include <drivers/uart.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/services/nus.h>
#include <settings/settings.h>

#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
#include "img_mgmt/img_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
#include "os_mgmt/os_mgmt.h"
#endif
#ifdef CONFIG_NRF_DFU
#include "nrf_dfu_settings.h"
#include "nrf_dfu.h"
#include "power/reboot.h"
#include "nrf_dfu.h"
#include "nrf_dfu_validation.h"
#endif
#include <logging/log.h>
#include <logging/log_ctrl.h>

#define LOG_MODULE_NAME main
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)
#define KEY_PASSKEY_ACCEPT DK_BTN1_MSK
#define KEY_PASSKEY_REJECT DK_BTN2_MSK

#define ERASE_DELAY_AFTER_BOOT 30   //unit: s
#ifndef CONFIG_NCS_V1_5_x
static struct k_work_delayable blinky_work;
#else
static struct k_delayed_work blinky_work;
#endif

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

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

/* Stack definition for application workqueue */
K_THREAD_STACK_DEFINE(application_stack_area,
		      1024);
static struct k_work_q application_work_q;

#define LED0_NODE DT_ALIAS(led0)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define LED0_PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define LED0_FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)

#define LED1_NODE DT_ALIAS(led1)
#define LED1	DT_GPIO_LABEL(LED1_NODE, gpios)
#define LED1_PIN	DT_GPIO_PIN(LED1_NODE, gpios)
#define LED1_FLAGS	DT_GPIO_FLAGS(LED1_NODE, gpios)

#ifdef CONFIG_PM_DEVICE
const struct device *devUart0;
const struct device *devUart1;
static void get_device_handles(void)
{
	devUart0 = device_get_binding("UART_0");
    devUart1 = device_get_binding("UART_1");
}
#if defined(CONFIG_NCS_V1_5_x)
void set_device_pm_state(void)
{
	int err;
	uint32_t pm_state;

	device_get_power_state(devUart1, &pm_state);
	if (pm_state == DEVICE_PM_SUSPEND_STATE)
	{
		LOG_INF("UART1 is in suspend state. We activate it");
		err = device_set_power_state(devUart1,
						DEVICE_PM_ACTIVE_STATE,
						NULL, NULL);
		if (err) {
			LOG_ERR("UART1 enable failed");			
		}
		else
		{
			LOG_INF("## UART1 is actvie now ##");
		}		
	}
	else if (pm_state == DEVICE_PM_ACTIVE_STATE)
	{
		LOG_INF("UART1 is in active state. We suspend it");
		err = device_set_power_state(devUart1,
						DEVICE_PM_SUSPEND_STATE,
						NULL, NULL);
		if (err) {
			LOG_ERR("UART1 disable failed");
		}
		else
		{
			LOG_INF("## UART1 is suspended now ##");
		}		
	}



	device_get_power_state(devUart0, &pm_state);
	if (pm_state == DEVICE_PM_SUSPEND_STATE)
	{
		LOG_INF("UART0 is in suspend state. We activate it");
		err = device_set_power_state(devUart0,
						DEVICE_PM_ACTIVE_STATE,
						NULL, NULL);
		if (err) {
			LOG_ERR("UART0 enable failed");			
		}
		else
		{
			LOG_INF("## UART0 is active now ##");
		}		
	}
	else if (pm_state == DEVICE_PM_ACTIVE_STATE)
	{
		LOG_INF("UART0 is in active state. We suspend it");
		//print out all the pending logging messages
		while(log_process(false));
		err = device_set_power_state(devUart0,
						DEVICE_PM_SUSPEND_STATE,
						NULL, NULL);
		if (err) {
			LOG_ERR("UART0 disable failed");
		}
		else
		{
			LOG_INF("## UART0 is suspended now ##");
		}		
	}

}
#elif defined(CONFIG_NCS_V1_6_x)
void set_device_pm_state(void)
{
	int err;
	uint32_t pm_state;

	if (devUart1)
	{
		pm_device_state_get(devUart1, &pm_state);
		if (pm_state == PM_DEVICE_STATE_SUSPEND)
		{
			LOG_INF("UART1 is in suspend state. We activate it");
			err = pm_device_state_set(devUart1,
							PM_DEVICE_STATE_ACTIVE,
							NULL, NULL);
			if (err) {
				LOG_ERR("UART1 enable failed");			
			}
			else
			{
				LOG_INF("## UART1 is active now ##");
			}		
		}
		else if (pm_state == PM_DEVICE_STATE_ACTIVE)
		{
			LOG_INF("UART1 is in active state. We suspend it");

#if CONFIG_UART_ASYNC_API && CONFIG_UART_1_NRF_HW_ASYNC
		((const struct uart_driver_api *)devUart1->api)->rx_disable(devUart1);
#endif			
			err = pm_device_state_set(devUart1,
							PM_DEVICE_STATE_SUSPEND,
							NULL, NULL);
			if (err) {
				LOG_ERR("UART1 disable failed");
			}
			else
			{
				LOG_INF("## UART1 is suspended now ##");
			}		
		}
	}

	pm_device_state_get(devUart0, &pm_state);
	if (pm_state == PM_DEVICE_STATE_SUSPEND)
	{
		LOG_INF("UART0 is in suspend state. We activate it");
		err = pm_device_state_set(devUart0,
						PM_DEVICE_STATE_ACTIVE,
						NULL, NULL);
		if (err) {
			LOG_ERR("UART0 enable failed");			
		}
		else
		{
			LOG_INF("## UART0 is active now ##");
		}		
	}
	else if (pm_state == PM_DEVICE_STATE_ACTIVE)
	{
		LOG_INF("UART0 is in active state. We suspend it");
		//print out all the pending logging messages
		while(log_process(false));

#if CONFIG_UART_ASYNC_API && CONFIG_UART_0_NRF_HW_ASYNC
		((const struct uart_driver_api *)devUart0->api)->rx_disable(devUart0);
#endif
		err = pm_device_state_set(devUart0,
						PM_DEVICE_STATE_SUSPEND,
						NULL, NULL);
		if (err) {
			LOG_ERR("UART0 disable failed");
		}
		else
		{
			LOG_INF("## UART0 is suspended now ##");
		}		
	}

}
#else
void set_device_pm_state(void)
{
	int err;
	enum pm_device_state pm_state;

	if (devUart1)
	{
		pm_device_state_get(devUart1, &pm_state);
		if (pm_state == PM_DEVICE_STATE_LOW_POWER)
		{
			LOG_INF("UART1 is in low power state. We activate it");
			err = pm_device_state_set(devUart1,PM_DEVICE_STATE_ACTIVE);
			if (err) {
				LOG_ERR("UART1 enable failed");			
			}
			else
			{
				LOG_INF("## UART1 is active now ##");
			}		
		}
		else if (pm_state == PM_DEVICE_STATE_ACTIVE)
		{
			LOG_INF("UART1 is in active state. We suspend it");

#if CONFIG_UART_ASYNC_API && CONFIG_UART_1_NRF_HW_ASYNC
		((const struct uart_driver_api *)devUart1->api)->rx_disable(devUart1);
#endif			
			err = pm_device_state_set(devUart1,	PM_DEVICE_STATE_LOW_POWER);
			if (err) {
				LOG_ERR("UART1 disable failed");
			}
			else
			{
				LOG_INF("## UART1 is suspended now ##");
			}		
		}
	}

	pm_device_state_get(devUart0, &pm_state);
	if (pm_state == PM_DEVICE_STATE_LOW_POWER)
	{
		LOG_INF("UART0 is in low power state. We activate it");
		err = pm_device_state_set(devUart0,	PM_DEVICE_STATE_ACTIVE);
		if (err) {
			LOG_ERR("UART0 enable failed");			
		}
		else
		{
			LOG_INF("## UART0 is active now ##");
		}		
	}
	else if (pm_state == PM_DEVICE_STATE_ACTIVE)
	{
		LOG_INF("UART0 is in active state. We suspend it");
		//print out all the pending logging messages
		while(log_process(false));

#if CONFIG_UART_ASYNC_API && CONFIG_UART_0_NRF_HW_ASYNC
		((const struct uart_driver_api *)devUart0->api)->rx_disable(devUart0);
#endif
		err = pm_device_state_set(devUart0,	PM_DEVICE_STATE_LOW_POWER);
		if (err) {
			LOG_ERR("UART0 disable failed");
		}
		else
		{
			LOG_INF("## UART0 is suspended now ##");
		}		
	}

}
#endif //CONFIG_NCS_V1_6_x & CONFIG_NCS_V1_5_x

#endif //CONFIG_PM_DEVICE

static void blinky_work_fn(struct k_work *work)
{
    //LOG_INF("blinky fn in system workqueue\n");
	gpio_pin_set(runLED, LED0_PIN, (int)led_is_on);
	led_is_on = !led_is_on;

#ifndef CONFIG_NCS_V1_5_x
	k_work_reschedule_for_queue(&application_work_q, &blinky_work,
					   	K_SECONDS(2));
#else
    //k_delayed_work_submit(&blinky_work, K_SECONDS(2));
	k_delayed_work_submit_to_queue(&application_work_q, &blinky_work,
				       K_SECONDS(2));
#endif

}

// #ifdef CONFIG_NCS_V1_5_x
// static struct k_delayed_work erase_slot_work;
// #else
// static struct k_work_delayable erase_slot_work;
// #endif //CONFIG_NCS_V1_5_x
// static void erase_work_fn(struct k_work *work)
// {   	
//    	img_mgmt_impl_erase_slot();
// 	LOG_WRN("Time is out and erase the secondary slot to speed up DFU");
// }

#ifdef CONFIG_NRF_DFU

/**@brief Function for handling DFU events.
 */
static void dfu_observer(nrf_dfu_evt_type_t evt_type)
{
    switch (evt_type)
    {
        case NRF_DFU_EVT_DFU_STARTED:
        case NRF_DFU_EVT_OBJECT_RECEIVED:

            break;
        case NRF_DFU_EVT_DFU_COMPLETED:
        case NRF_DFU_EVT_DFU_ABORTED:
			LOG_INF("resetting...");
			while(log_process(false));
            sys_reboot(SYS_REBOOT_WARM);
            break;
        case NRF_DFU_EVT_TRANSPORT_DEACTIVATED:
            // Reset the internal state of the DFU settings to the last stored state.
			LOG_INF("NRF_DFU_EVT_TRANSPORT_DEACTIVATED");
            nrf_dfu_settings_reinit();
            break;
        default:
            break;
    }

}

int dfu_init(void)
{
    int ret_val;

    ret_val = nrf_dfu_settings_init(true);
    if (ret_val != NRF_SUCCESS)
	{
		LOG_WRN("dfu settings init err %d", ret_val);
	}

    ret_val = nrf_dfu_init(dfu_observer);
    if (ret_val != NRF_SUCCESS)
	{
		LOG_WRN("dfu init err %d", ret_val);
	}

    return ret_val;
}
#endif //CONFIG_NRF_DFU

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
	LOG_INF("Connected %s", log_strdup(addr));

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

	LOG_INF("Disconnected: %s (reason %u)", log_strdup(addr), reason);

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
		LOG_INF("Security changed: %s level %u", log_strdup(addr),
			level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d", log_strdup(addr),
			level, err);
	}
}
#endif

static void param_updated(struct bt_conn *conn, uint16_t interval,
				 uint16_t latency, uint16_t timeout)
{
	LOG_INF("conn interval=%d, latency=%d, timeout=%d", interval, latency, timeout);
}
static struct bt_conn_cb conn_callbacks = {
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

	LOG_INF("Passkey for %s: %06u", log_strdup(addr), passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	auth_conn = bt_conn_ref(conn);

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Passkey for %s: %06u", log_strdup(addr), passkey);
	LOG_INF("Press Button 1 to confirm, Button 2 to reject.");
}


static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", log_strdup(addr));
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing completed: %s, bonded: %d", log_strdup(addr),
		bonded);
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing failed conn: %s, reason %d", log_strdup(addr),
		reason);
}


static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_display = auth_passkey_display,
	.passkey_confirm = auth_passkey_confirm,
	.cancel = auth_cancel,	
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};
#else
static struct bt_conn_auth_cb conn_auth_callbacks;
#endif

extern int my_uart_send(const uint8_t *buf, size_t len);
static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data,
			  uint16_t len)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN] = {0};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));

	LOG_INF("Received data from: %s", log_strdup(addr));

	err = my_uart_send(data, len);
	
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

	if (buttons & DK_BTN3_MSK) {
		LOG_INF("button3 isr");
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

void main(void)
{
	int err;	

	LOG_INF("### comprehensive example v0.3 compiled at %s %s\n", __TIME__, __DATE__);

	runLED = device_get_binding(LED0);
	if (runLED == NULL) {
		LOG_ERR("LED0 bind null\n");
	}
	err = gpio_pin_configure(runLED, LED0_PIN, GPIO_OUTPUT_ACTIVE | LED0_FLAGS);
	if (err < 0) {
		LOG_ERR("led0 configure error %d \n", err);		
	}

	conLED = device_get_binding(LED1);
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

#ifndef CONFIG_NCS_V1_5_x
	k_work_queue_start(&application_work_q, application_stack_area,
			   K_THREAD_STACK_SIZEOF(application_stack_area), 10,
			   NULL);
	k_work_init_delayable(&blinky_work, blinky_work_fn);
	k_work_reschedule_for_queue(&application_work_q, &blinky_work,
					   	K_MSEC(20));	
#else
	k_work_q_start(&application_work_q, application_stack_area,
		       K_THREAD_STACK_SIZEOF(application_stack_area),
		       10);
	k_delayed_work_init(&blinky_work, blinky_work_fn);
	// k_delayed_work_submit(&blinky_work, K_MSEC(20));
	k_delayed_work_submit_to_queue(&application_work_q, &blinky_work,
				       K_MSEC(20));				   	
#endif

	bt_conn_cb_register(&conn_callbacks);

	if (IS_ENABLED(CONFIG_BT_NUS_SECURITY_ENABLED)) {
		bt_conn_auth_cb_register(&conn_auth_callbacks);
	}

	err = bt_enable(NULL);
	if (err) {
		error();
	}

	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_nus_init(&nus_cb);
	if (err) {
		LOG_ERR("Failed to initialize UART service (err: %d)", err);
		return;
	}

#ifdef CONFIG_MCUMGR
	smp_bt_register();
	os_mgmt_register_group();
	img_mgmt_register_group();
#endif

#ifdef CONFIG_NRF_DFU
	err = dfu_init();
	if (err) {
		LOG_ERR("dfu service init err %d", err);		
	}
#endif //CONFIG_NRF_DFU

// #ifdef CONFIG_NCS_V1_5_x
// 	k_delayed_work_init(&erase_slot_work, erase_work_fn);
// 	k_delayed_work_submit(&erase_slot_work, K_SECONDS(ERASE_DELAY_AFTER_BOOT));
// #else
// 	k_work_init_delayable(&erase_slot_work, erase_work_fn);
// 	k_work_reschedule(&erase_slot_work, K_SECONDS(ERASE_DELAY_AFTER_BOOT));
// #endif

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd,
			      ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
	}

	while(1)
	{
		//add your code here
		k_sleep(K_SECONDS(20));
		LOG_INF("main thread\n");
	}
	// //since we don't put any work in main thread, exit directly
	// LOG_WRN("exit main thread\n");

}
