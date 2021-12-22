/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <power/reboot.h>
#include <device.h>
#include <string.h>
#include <drivers/flash.h>
#include <storage/flash_map.h>
#include <fs/nvs.h>
#include "settings/settings.h"
#include <logging/log.h>

#define LOG_MODULE_NAME flash_thread
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static struct nvs_fs fs;

#define KEY_ID 1
#define RBT_CNT_ID 2

int nvs_usage_init(void)
{
	int rc;
	uint8_t key[8];
	uint32_t reboot_counter = 0U;
	const struct device *flash_dev;
	struct flash_pages_info info;

	flash_dev = device_get_binding(DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	if (!device_is_ready(flash_dev)) {
		LOG_ERR("Flash device %s is not ready", flash_dev->name);
		return -EINVAL;
	}
	fs.offset = FLASH_AREA_OFFSET(storage);
	rc = flash_get_page_info_by_offs(flash_dev, fs.offset, &info);
	if (rc) {
		LOG_ERR("Unable to get page info");
		return -EINVAL;
	}
	fs.sector_size = info.size;
	fs.sector_count = CONFIG_PM_PARTITION_SIZE_SETTINGS_STORAGE / info.size;

	LOG_INF("NVS sector size=%d sector count=%d\n", fs.sector_size, fs.sector_count);

	rc = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
	if (rc)
	{
		LOG_ERR("Flash Init failed %d", rc);
		return -EINVAL;
	}

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
		(void)nvs_write(&fs, KEY_ID, &key, sizeof(key));
	}
	/* RBT_CNT_ID is used to store the reboot counter, lets see
	 * if we can read it from flash
	 */
	reboot_counter = 0;
	rc = nvs_read(&fs, RBT_CNT_ID, &reboot_counter, sizeof(reboot_counter));
	if (rc > 0)
	{ /* item was found, show it */
		LOG_INF("*** Reboot counter in NVS: %d ***",
				reboot_counter);
	}
	LOG_INF("save new reboot counter by NVS API");
	reboot_counter++;
	(void)nvs_write(&fs, RBT_CNT_ID, &reboot_counter,
					sizeof(reboot_counter));

	return 0;
}

static uint32_t reboot_cnt;
static uint8_t key_s[8];

int alpha_handle_set(const char *name, size_t len, settings_read_cb read_cb,
					 void *cb_arg)
{
	const char *next;
	int rc;

	LOG_INF("set handler name=%s, len=%d ", log_strdup(name), len);
	if (settings_name_steq(name, "boot_cnt", &next) && !next)
	{
		if (len != sizeof(reboot_cnt))
		{
			return -EINVAL;
		}
		rc = read_cb(cb_arg, &reboot_cnt, sizeof(reboot_cnt));
		LOG_INF("*** Reboot counter in Settings: %d ****", reboot_cnt);
		return 0;
	}

	if (settings_name_steq(name, "key", &next) && !next)
	{
		if (len != sizeof(key_s))
		{
			return -EINVAL;
		}
		rc = read_cb(cb_arg, key_s, sizeof(key_s));
		LOG_HEXDUMP_INF(key_s, sizeof(key_s), "Key value in Settings:");
		return 0;
	}

	return -ENOENT;
}

/* dynamic main tree handler */
struct settings_handler alph_handler = {
	.name = "alpha",
	.h_get = NULL,
	.h_set = alpha_handle_set,
	.h_commit = NULL,
	.h_export = NULL};

void settings_usage_init(void)
{
	int rc;

	rc = settings_subsys_init();
	if (rc)
	{
		LOG_ERR("settings subsys initialization: fail (err %d) ", rc);
		return;
	}

	LOG_INF("settings subsys initialization: OK.");

	rc = settings_register(&alph_handler);
	if (rc)
	{
		LOG_ERR("subtree <%s> handler registered: fail (err %d)",
				alph_handler.name, rc);
	}

	/* load all key-values at once
	 * In case a key-value doesn't exist in the storage
	 * default valuse should be assigned to settings variable
	 * before any settings load call
	 */
	LOG_INF("Load all key-value pairs using registered handlers");
	settings_load();

	if (reboot_cnt == 0)
	{
		LOG_INF("save key_s By Settings API");
		key_s[0] = 0x30;
		key_s[1] = 0x31;
		key_s[2] = 0x32;
		key_s[3] = 0x33;
		key_s[4] = 0x34;
		key_s[5] = 0x35;
		key_s[6] = 0x36;
		key_s[7] = 0x37;
		rc = settings_save_one("alpha/key", (const void *)key_s,
							   sizeof(key_s));
		if (rc)
		{
			LOG_ERR("key_s save err %d ", rc);
		}
	}

	LOG_INF("save new reboot counter by Settings API");
	reboot_cnt++;
	rc = settings_save_one("alpha/boot_cnt", (const void *)&reboot_cnt,
						   sizeof(reboot_cnt));
	if (rc)
	{
		LOG_ERR("boot_cnt save err %d ", rc);
	}
}
void flash_thread(void)
{

	LOG_INF("**Flash access example using both NVS and settings API");

	nvs_usage_init();

	settings_usage_init();

	while (1)
	{
		LOG_INF("Flash thread");
		k_sleep(K_SECONDS(20));
	}
}

K_THREAD_DEFINE(flash_thread_id, 1024, flash_thread, NULL, NULL,
				NULL, 9, 0, 0);