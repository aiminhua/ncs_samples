/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME nvs_thread
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define STORAGE_NODE_LABEL storage
static struct nvs_fs fs;

#define KEY_ID 0x1000
#define RBT_CNT_ID 0x1001
static uint8_t key[8];
static uint32_t reboot_counter = 0U;	

static int nvs_usage_init(void)
{
	int rc;
	struct flash_pages_info info;

	fs.flash_device = FLASH_AREA_DEVICE(STORAGE_NODE_LABEL);
	if (!device_is_ready(fs.flash_device)) {
		printk("Flash device %s is not ready\n", fs.flash_device->name);
		return -EINVAL;
	}
	fs.offset = FLASH_AREA_OFFSET(STORAGE_NODE_LABEL);
	rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
	if (rc) {
		printk("Unable to get page info\n");
		return -EINVAL;
	}
	fs.sector_size = info.size;
	fs.sector_count = CONFIG_PM_PARTITION_SIZE_SETTINGS_STORAGE / info.size;

	LOG_INF("NVS sector size=%d sector count=%d\n", fs.sector_size, fs.sector_count);
	
	rc = nvs_mount(&fs);
	if (rc)
	{
		LOG_ERR("NVS Init failed %d", rc);
		return -EINVAL;
	}

	return 0;
}

static int flash_access(void)
{
	int rc;

	/* KEY_ID is used to store a key, lets see if we can read it from flash
	 */
	rc = nvs_read(&fs, KEY_ID, &key, sizeof(key));
	if (rc > 0)
	{ /* item was found, show it */
		LOG_HEXDUMP_INF(key, sizeof(key), "Key value in NVS:");
	}
	else
	{ /* item was not found, add it */
		LOG_INF("No key found, adding it at id %d by NVS API", KEY_ID);
		key[0] = 0xFF;
		key[1] = 0xFE;
		key[2] = 0xFD;
		key[3] = 0xFC;
		key[4] = 0xFB;
		key[5] = 0xFA;
		key[6] = 0xF9;
		key[7] = 0xF8;
		rc = nvs_write(&fs, KEY_ID, &key, sizeof(key));
		if (rc != sizeof(key))
		{
			LOG_ERR("nvs_write key err:%d", rc);
		}
	}
	/* RBT_CNT_ID is used to store the reboot counter, lets see
	 * if we can read it from flash
	 */	
	rc = nvs_read(&fs, RBT_CNT_ID, &reboot_counter, sizeof(reboot_counter));
	if (rc > 0)
	{ /* item was found, show it */
		LOG_INF("*** Reboot counter in NVS: %d ***",
				reboot_counter);
	}
	else
	{
		LOG_ERR("nvs_read reboot_counter err:%d", rc);
	}

	LOG_INF("save new reboot counter by NVS API");
	reboot_counter++;
	rc = nvs_write(&fs, RBT_CNT_ID, &reboot_counter,
					sizeof(reboot_counter));
	if (rc != sizeof(reboot_counter))
	{
		LOG_ERR("nvs_write reboot_counter err:%d", rc);
	}					

	return 0;

}

void nvs_thread(void)
{
	static bool operation = true;
	int rc;

	LOG_INF("Flash access using NVS APIs");

	rc = nvs_usage_init();
	if (rc)
	{
		LOG_ERR("nvs init failed %d", rc);
		return;
	}	

	while (1)
	{
		LOG_INF("NVS thread");
		
		if (operation)
		{
			operation = false;
			flash_access();
		}

		k_sleep(K_SECONDS(20));
	}
}

K_THREAD_DEFINE(nvs_thread_id, 1024, nvs_thread, NULL, NULL,
				NULL, 3, 0, 0);