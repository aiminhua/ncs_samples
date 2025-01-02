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

#define NVS_LABEL nvs_fs
static struct nvs_fs fs;

#define NVS_ID_TEST 3
#define NVS_ID_KEY 4
#define NVS_ID_CNT 5
static uint8_t key[8];
static uint32_t reboot_counter = 0U;
static uint8_t test_data[256];	

static int nvs_usage_init(void)
{
	int rc;
	struct flash_pages_info info;
	const struct flash_area *fa;

	//erase the whole area first
	rc = flash_area_open(PM_NVS_FS_ID, &fa);
	if (rc) {
		printk("flash_area_open err:%d\n", rc);
		return -EINVAL;
	}

	flash_area_read(fa, 0x1000, test_data, 256);
	LOG_INF("external Flash read: 0x%x ~ 0x%x",
				test_data[0], test_data[255]);

	// LOG_INF("erasing the whole external SIMULATION RAM Flash area");
	// LOG_INF("Please wait......");
	// rc = flash_area_erase(fa, 0, PM_RAM_SIM_SIZE);   //PM_RAM_SIM_SIZE
	// if (rc) {
	// 	printk("flash_area_open err:%d\n", rc);
	// 	return -EINVAL;
	// }	

	fs.flash_device = FLASH_AREA_DEVICE(NVS_LABEL);
	if (!device_is_ready(fs.flash_device)) {
		printk("Flash device %s is not ready\n", fs.flash_device->name);
		return -EINVAL;
	}
	fs.offset = FLASH_AREA_OFFSET(NVS_LABEL);
	rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
	if (rc) {
		printk("Unable to get page info\n");
		return -EINVAL;
	}
	fs.sector_size = info.size;
	fs.sector_count = PM_NVS_FS_SIZE / info.size;

	LOG_INF("NVS sector_size=%d sector_count=%d\n", fs.sector_size, fs.sector_count);
	
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

	/* NVS_ID_KEY is used to store a key, lets see if we can read it from flash
	 */
	rc = nvs_read(&fs, NVS_ID_KEY, &key, sizeof(key));
	if (rc > 0)
	{ /* item was found, show it */
		LOG_HEXDUMP_INF(key, sizeof(key), "Key value in NVS:");
	}
	else
	{ /* item was not found, add it */
		LOG_INF("No key found, adding it at id %d by NVS API", NVS_ID_KEY);
		key[0] = 0xFF;
		key[1] = 0xFE;
		key[2] = 0xFD;
		key[3] = 0xFC;
		key[4] = 0xFB;
		key[5] = 0xFA;
		key[6] = 0xF9;
		key[7] = 0xF8;
		rc = nvs_write(&fs, NVS_ID_KEY, &key, sizeof(key));
		if (rc != sizeof(key))
		{
			LOG_ERR("nvs_write key err:%d", rc);
		}
	}
	/* NVS_ID_CNT is used to store the reboot counter, lets see
	 * if we can read it from flash
	 */	
	rc = nvs_read(&fs, NVS_ID_CNT, &reboot_counter, sizeof(reboot_counter));
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
	rc = nvs_write(&fs, NVS_ID_CNT, &reboot_counter,
					sizeof(reboot_counter));
	if (rc != sizeof(reboot_counter))
	{
		LOG_ERR("nvs_write reboot_counter err:%d", rc);
	}

	LOG_INF("save test_data by NVS API");	
	memset(test_data, reboot_counter, sizeof(test_data));
	rc = nvs_write(&fs, NVS_ID_TEST, &test_data,
					sizeof(test_data));
	if (rc != sizeof(test_data))
	{
		LOG_ERR("nvs_write test_data err:%d", rc);
	}	

	rc = nvs_read(&fs, NVS_ID_TEST, &test_data, sizeof(test_data));
	if (rc > 0)
	{ /* item was found, show it */
		LOG_INF("test_data: 0x%x ~ 0x%x in NVS",
				test_data[0], test_data[255]);
	}
	else
	{
		LOG_ERR("nvs_read test data err:%d", rc);
	}


	return 0;

}

void nvs_thread(void)
{
	int rc;

	LOG_INF("File system sample on external Flash");

	//let other works finish first
	k_sleep(K_SECONDS(1));

	rc = nvs_usage_init();
	if (rc)
	{
		LOG_ERR("nvs init failed %d", rc);
		return;
	}	

	while (1)   //exit
	{
		k_msleep(5000);
		flash_access();		
	}
}

K_THREAD_DEFINE(nvs_thread_id, 1024, nvs_thread, NULL, NULL,
				NULL, 9, 0, 0);