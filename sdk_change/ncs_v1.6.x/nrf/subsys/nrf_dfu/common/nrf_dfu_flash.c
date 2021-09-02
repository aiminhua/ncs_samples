/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <inttypes.h>
#include <zephyr/types.h>
#include <sys/byteorder.h>
#include <storage/flash_map.h>
#include <logging/log.h>
#include "nrf_dfu_flash.h"
#include <drivers/flash.h>
#include <storage/stream_flash.h>
#include <dfu/flash_img.h>
#include "nrf_dfu_settings.h"

LOG_MODULE_REGISTER(dfu_flash, CONFIG_NRF_DFU_LOG_LEVEL);

const void * const dfu_flash_module;

#ifdef CONFIG_BOARD_HAS_NRF5_BOOTLOADER
#define DFU_STREAM 1
#endif


#define FLASH_PAGE_SIZE_LOG2	12
#define FLASH_PAGE_SIZE		BIT(FLASH_PAGE_SIZE_LOG2)
#define FLASH_PAGE_ID(off)	((off) >> FLASH_PAGE_SIZE_LOG2)
#define FLASH_CLEAN_VAL		UINT32_MAX
#define FLASH_READ_CHUNK_SIZE	(FLASH_PAGE_SIZE / 8)

/* Keep small to avoid blocking the workqueue for long periods of time. */
#define STORE_CHUNK_SIZE		16 /* bytes */

#define SYNC_BUFFER_SIZE (128 * sizeof(uint32_t)) /* bytes */

#if CONFIG_SECURE_BOOT
 #include <fw_info.h>
 #define IMAGE0_ID		PM_S0_IMAGE_ID
 #define IMAGE0_ADDRESS		PM_S0_IMAGE_ADDRESS
 #define IMAGE1_ID		PM_S1_IMAGE_ID
 #define IMAGE1_ADDRESS		PM_S1_IMAGE_ADDRESS
#elif CONFIG_BOOTLOADER_MCUBOOT
 #include <dfu/mcuboot.h>
 #define IMAGE0_ID		PM_MCUBOOT_PRIMARY_ID
 #define IMAGE0_ADDRESS		PM_MCUBOOT_PRIMARY_ADDRESS
 #define IMAGE1_ID		PM_MCUBOOT_SECONDARY_ID
 #define IMAGE1_ADDRESS		PM_MCUBOOT_SECONDARY_ADDRESS
#else
 //#error Bootloader not supported.
#endif

#ifndef DFU_STREAM
static const struct flash_area *flash_area;
#endif

#define DFU_UNLOCKED	0

static atomic_t dfu_locked = ATOMIC_INIT(DFU_UNLOCKED);


bool dfu_lock(const void *module_id)
{
	return atomic_cas(&dfu_locked, DFU_UNLOCKED, (atomic_val_t)module_id);
}

void dfu_unlock(const void *module_id)
{
	bool success = atomic_cas(&dfu_locked, (atomic_val_t)module_id,
				  DFU_UNLOCKED);

	/* Module that have not locked dfu, should not try to unlock it. */
	__ASSERT_NO_MSG(success);
	ARG_UNUSED(success);
}

#ifndef DFU_STREAM
static uint8_t dfu_slot_id(void)
{
#if CONFIG_BOOTLOADER_MCUBOOT
	/* MCUBoot always puts new image in the secondary slot. */
	return IMAGE1_ID;
#else
	BUILD_ASSERT(IMAGE0_ADDRESS < IMAGE1_ADDRESS);
	if ((uint32_t)(uintptr_t)dfu_slot_id < IMAGE1_ADDRESS) {
		return IMAGE1_ID;
	}

	return IMAGE0_ID;
#endif
}

static bool is_page_clean(const struct flash_area *fa, int off, size_t len)
{
	static const size_t chunk_size = FLASH_READ_CHUNK_SIZE;
	static const size_t chunk_cnt = FLASH_PAGE_SIZE / chunk_size;

	BUILD_ASSERT(chunk_size * chunk_cnt == FLASH_PAGE_SIZE);
	BUILD_ASSERT(chunk_size % sizeof(uint32_t) == 0);

	uint32_t buf[chunk_size / sizeof(uint32_t)];

	int err;

	for (size_t i = 0; i < chunk_cnt; i++) {
		err = flash_area_read(fa, off + i * chunk_size, buf, chunk_size);

		if (err) {
			LOG_ERR("Cannot read flash");
			return false;
		}

		for (size_t j = 0; j < ARRAY_SIZE(buf); j++) {
			if (buf[j] != FLASH_CLEAN_VAL) {
				return false;
			}
		}
	}

	return true;
}
#endif


#ifndef DFU_STREAM
int dfu_flash_start(uint32_t image_start, uint32_t image_len)
{
	if (flash_area) {
		LOG_WRN("DFU already in progress");
		return 0;
	}

	if (!dfu_lock(dfu_flash_module)) {
		LOG_WRN("DFU already started by another module");
		return 0;
	}
	
	__ASSERT_NO_MSG(flash_area == NULL);
	int err = flash_area_open(dfu_slot_id(), &flash_area);

	if (err) {
		LOG_ERR("Cannot open flash area (%d)", err);
		flash_area = NULL;
		dfu_unlock(dfu_flash_module);

		return err;
	}

	// LOG_DBG("flash area size=0x%x image size=0x%x id %d", flash_area->fa_size, image_len, flash_area->fa_id);
	
	// if (!is_page_clean(flash_area, 0, FLASH_PAGE_SIZE)) {
	// 	uint32_t round_size = image_len/FLASH_PAGE_SIZE * 4096;
	// 	if (image_len % FLASH_PAGE_SIZE) 
	// 	{
	// 		round_size += 4096;
	// 	}
	// 	err = flash_area_erase(flash_area, 0, round_size);
	// 	if (err) {
	// 		LOG_ERR("Cannot erase the whole image area %d", err);
	// 		flash_area_close(flash_area);
	// 		flash_area = NULL;			
	// 	}
	// 	else
	// 	{
	// 		LOG_INF("**the size=0x%x of Flash erased", image_len);
	// 	}
	// }

	return err;	
}
#else
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	static struct flash_img_context *ctx = NULL;
#else
	static struct flash_img_context ctx_data;
#define ctx (&ctx_data)
#endif
int dfu_flash_start(uint32_t image_start, uint32_t image_len)
{
	int rc = 0;

	if (!dfu_lock(dfu_flash_module)) {
		LOG_WRN("DFU already started by another module");
		return 0;
	}		
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	if (ctx == NULL) {
		ctx = k_malloc(sizeof(*ctx));
		if (ctx == NULL) {
			return -EFAULT;
		}
	}
#endif
	rc = flash_img_init(ctx);
	if (rc)
	{
		LOG_ERR("flash_img_init err %d", rc);		
	}

	ctx->stream.offset = image_start;

	return rc;

}
#endif

#ifndef DFU_STREAM
void dfu_flash_finish(void)
{
	__ASSERT_NO_MSG(flash_area != NULL);
	
#ifdef CONFIG_BOOTLOADER_MCUBOOT
	int err = boot_request_upgrade(false);
	if (err) {
		LOG_ERR("Cannot request the image upgrade (err:%d)", err);
	}
#endif
	LOG_INF("image trailer written");	
	flash_area_close(flash_area);
	dfu_unlock(dfu_flash_module);
	flash_area = NULL;	
}
#else
extern nrf_dfu_settings_t s_dfu_settings;
void dfu_flash_finish(void)
{
	int rc;
	
	ctx->stream.offset = BOOTLOADER_SETTINGS_ADDRESS;
	ctx->stream.buf_bytes = 0;
	ctx->stream.bytes_written = 0;
	ctx->stream.available = FLASH_PAGE_SIZE;	
	rc = flash_img_buffered_write(ctx, (void *)&s_dfu_settings, sizeof(nrf_dfu_settings_t), true);
	if(rc)
	{
		LOG_ERR("update settings err %d", rc);
	}
	else
	{
		LOG_INF("settings page update done");
	}

	dfu_unlock(dfu_flash_module);

#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)	
	k_free(ctx);
	ctx = NULL;	
#endif

}
#endif

#ifndef DFU_STREAM
int dfu_data_store(int off, const void *src,
		     size_t len, bool flush)
{
	int err;
	LOG_DBG("flash store off=%x, src=%p, len=%d", off, src, len);
	err = flash_area_write(flash_area, off, src, len);
	if (err) {
		LOG_ERR("Cannot write flash (%d)", err);
		flash_area_close(flash_area);
		flash_area = NULL;		
	}
	return err;
}
#else
int dfu_data_store(int off, const void *src,
		     size_t len, bool flush)
{
	int rc;
		/* Cast away const. */
	rc = flash_img_buffered_write(ctx, (void *)src, len, flush);

	return rc;
}
#endif

#ifndef DFU_STREAM
int dfu_page_erase(int off, size_t len)
{
	int err = 0;

    // __ASSERT_NO_MSG(flash_area != NULL);
	if (flash_area == NULL)
	{
		err = flash_area_open(dfu_slot_id(), &flash_area);
		if (err) {
			LOG_ERR("Cannot open flash area (%d)", err);
			return err;
		}
	}

	__ASSERT_NO_MSG(off + FLASH_PAGE_SIZE <= flash_area->fa_size);    

	if (!is_page_clean(flash_area, off, FLASH_PAGE_SIZE)) {
		err = flash_area_erase(flash_area, off, FLASH_PAGE_SIZE);
		if (err) {
			LOG_ERR("Cannot erase page (%d)", err);
			flash_area_close(flash_area);
			flash_area = NULL;			
		}
		else
		{
			LOG_INF("===erase off=0x%x len=%d ==", off, len);
		}
	}
        
	return err;
}
#else
int dfu_page_erase(int off, size_t len)
{
	return 0;
}
#endif