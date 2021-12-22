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

LOG_MODULE_REGISTER(nrf_dfu_flash, CONFIG_NRF_DFU_LOG_LEVEL);

const void * const dfu_flash_module;


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
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
		k_free(ctx);
		ctx = NULL;
#endif		
		LOG_ERR("flash_img_init err %d", rc);		
	}
	else
	{
#ifdef PM_MCUBOOT_SECONDARY_ADDRESS
		ctx->stream.offset = PM_MCUBOOT_SECONDARY_ADDRESS + image_start;
#else
		ctx->stream.offset = image_start;
#endif
	}
	return rc;
}

#ifdef CONFIG_BOARD_HAS_NRF5_BOOTLOADER
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
#else
void dfu_flash_finish(void)
{	
#ifdef CONFIG_BOOTLOADER_MCUBOOT
	int err = boot_request_upgrade(false);
	if (err) {
		LOG_ERR("Cannot request the image upgrade (err:%d)", err);
	}
#endif

	dfu_unlock(dfu_flash_module);

#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)	
	k_free(ctx);
	ctx = NULL;	
#endif
	LOG_INF("image trailer written");

}
#endif //CONFIG_BOARD_HAS_NRF5_BOOTLOADER

int dfu_data_store(int off, const void *src,
		     size_t len, bool flush)
{
	int rc;
		/* Cast away const. */
	rc = flash_img_buffered_write(ctx, (void *)src, len, flush);

	return rc;
}

int dfu_page_erase(int off, size_t len)
{
	return 0;
}