/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash/flash_simulator.h>
#include "bootutil/image.h"
#include "bootutil/bootutil.h"
#include "bootutil/fault_injection_hardening.h"
#include "flash_map_backend/flash_map_backend.h"

#define NET_CORE_SECONDARY_SLOT 1
#define NET_CORE_VIRTUAL_PRIMARY_SLOT 3

#include <dfu/pcd.h>

int boot_read_image_header_hook(int img_index, int slot,
		struct image_header *img_head)
{
	if (img_index == 1 && slot == 0) {
		img_head->ih_magic = IMAGE_MAGIC;
		img_head->ih_hdr_size = PM_MCUBOOT_PAD_SIZE;
		img_head->ih_load_addr = PM_MCUBOOT_PRIMARY_1_ADDRESS;
		img_head->ih_img_size = PM_CPUNET_APP_SIZE;
		img_head->ih_flags = 0;
		img_head->ih_ver.iv_major = 0;
		img_head->ih_ver.iv_minor = 0;
		img_head->ih_ver.iv_revision = 0;
		img_head->ih_ver.iv_build_num = 0;
		img_head->_pad1 = 0;
		return 0;
	}

	return BOOT_HOOK_REGULAR;
}

#define BOOT_TMPBUF_SZ  256

fih_int boot_image_check_hook(int img_index, int slot)
{
	const struct flash_area *fap;
	struct image_header hdr;
	int rc;
	static uint8_t tmpbuf[BOOT_TMPBUF_SZ];

	// printk("boot_image_check_hook %d %d \r", img_index, slot);

	if (img_index == 1 && slot == 0) {
		FIH_RET(FIH_SUCCESS);
	}
	else if (img_index == 0 && slot == 0)
	{
		//validate primary XIP area	 
		rc = flash_area_open(PM_EXTFLASH_ID, &fap);
		flash_area_read(fap, 0, &hdr, sizeof(struct image_header));
		printk("primary XIP hdr size:0x%x img size:%d\r",hdr.ih_hdr_size, hdr.ih_img_size); 
		rc = bootutil_img_validate(NULL, 0, &hdr, fap, tmpbuf, BOOT_TMPBUF_SZ, NULL, 0, NULL);		
		if (rc)
		{
			printk("primary XIP validated error: %d\r", rc);
			FIH_RET(FIH_FAILURE);
		}
	}
	else if (img_index == 0 && slot == 1)
	{
		//validate secondary XIP area
		rc = flash_area_open(PM_MCUBOOT_SECONDARY_2_ID, &fap);
		flash_area_read(fap, 0, &hdr, sizeof(struct image_header));
        printk("secondary XIP hdr size:%x img size:%x\r",hdr.ih_hdr_size, hdr.ih_img_size); 
		rc = bootutil_img_validate(NULL, 0, &hdr, fap, tmpbuf, BOOT_TMPBUF_SZ, NULL, 0, NULL);
		if (rc)
		{
			printk("secondary XIP validated error: %d\r", rc);
			FIH_RET(FIH_FAILURE);
		}		
	}	

	FIH_RET(fih_int_encode(BOOT_HOOK_REGULAR));
}

int boot_perform_update_hook(int img_index, struct image_header *img_head,
		const struct flash_area *area)
{
	return BOOT_HOOK_REGULAR;
}

int boot_read_swap_state_primary_slot_hook(int image_index,
		struct boot_swap_state *state)
{
	if (image_index == 1) {
		/* Populate with fake data */
		state->magic = BOOT_MAGIC_UNSET;
		state->swap_type = BOOT_SWAP_TYPE_NONE;
		state->image_num = image_index;
		state->copy_done = BOOT_FLAG_UNSET;
		state->image_ok = BOOT_FLAG_UNSET;

		/*
		 * Skip more handling of the primary slot for Image 1 as the slot
		 * exsists in RAM and is empty.
		 */
		return 0;
	}

	return BOOT_HOOK_REGULAR;
}

int network_core_update(bool wait)
{
	struct image_header *hdr;
	static const struct device *mock_flash_dev;
	void *mock_flash;
	size_t mock_size;

	mock_flash_dev = DEVICE_DT_GET(DT_NODELABEL(PM_MCUBOOT_PRIMARY_1_DEV));
	if (!device_is_ready(mock_flash_dev)) {
		return -ENODEV;
	}

	mock_flash = flash_simulator_get_memory(NULL, &mock_size);
	hdr = (struct image_header *) mock_flash;
	if (hdr->ih_magic == IMAGE_MAGIC) {
		uint32_t fw_size = hdr->ih_img_size;
		uint32_t vtable_addr = (uint32_t)hdr + hdr->ih_hdr_size;
		uint32_t *vtable = (uint32_t *)(vtable_addr);
		uint32_t reset_addr = vtable[1];

		if (reset_addr > PM_CPUNET_B0N_ADDRESS) {
			if (wait) {
				return pcd_network_core_update(vtable, fw_size);
			} else {
				return pcd_network_core_update_initiate(vtable, fw_size);
			}
		}
	}

	/* No IMAGE_MAGIC no valid image */
	return -ENODATA;
}

int boot_copy_region_post_hook(int img_index, const struct flash_area *area,
		size_t size)
{
	if (img_index == NET_CORE_SECONDARY_SLOT) {
		return network_core_update(true);
	}
	else if (img_index == 0)
	{
    	const struct flash_area *fap_primary_slot;
    	const struct flash_area *fap_secondary_slot;
		int rc;
		struct image_header phdr;
		struct image_header shdr;
		struct image_tlv_info info;
		static uint8_t buf[CONFIG_NORDIC_QSPI_NOR_FLASH_LAYOUT_PAGE_SIZE] __attribute__((aligned(4)));
		uint32_t bytes_copied = 0;
		uint32_t sz;

		printk("Copying secondary XIP to primary XIP\r");
		//no need to do protection for power loss

		rc = flash_area_open(PM_EXTFLASH_ID, &fap_primary_slot);
		assert (rc == 0);

		rc = flash_area_open(PM_MCUBOOT_SECONDARY_2_ID, &fap_secondary_slot);
		assert (rc == 0);

		rc = flash_area_read(fap_primary_slot, 0, &phdr, sizeof(struct image_header));
		assert (rc == 0);

		sz = phdr.ih_hdr_size + phdr.ih_img_size;
		rc = flash_area_read(fap_primary_slot, sz, &info, sizeof(struct image_tlv_info));
		assert (rc == 0);
		sz += (info.it_tlv_tot + 4);
		sz = (sz / CONFIG_NORDIC_QSPI_NOR_FLASH_LAYOUT_PAGE_SIZE + 1) * CONFIG_NORDIC_QSPI_NOR_FLASH_LAYOUT_PAGE_SIZE;
		// printk("erase size %x\r", sz);
		rc = flash_area_erase(fap_primary_slot, 0, sz);
		if (rc)
		{
			printk("qspi erase error:%d\r", rc);
			return -1;
		}

		flash_area_read(fap_secondary_slot, 0, &shdr, sizeof(struct image_header));

		sz = shdr.ih_hdr_size + shdr.ih_img_size;
		rc = flash_area_read(fap_secondary_slot, sz, &info, sizeof(struct image_tlv_info));
		assert (rc == 0);
		// printk("it_tlv_tot size 0x%x\r", info.it_tlv_tot);
		sz += (info.it_tlv_tot + 4);
		sz = (sz / CONFIG_NORDIC_QSPI_NOR_FLASH_LAYOUT_PAGE_SIZE + 1) * CONFIG_NORDIC_QSPI_NOR_FLASH_LAYOUT_PAGE_SIZE;
		// printk("copy size %x\r", sz);

		//the trailer size is 0x98 for ecc256. 0x1000 is used here just for simplicity
		// sz = shdr.ih_hdr_size + shdr.ih_img_size + 0x1000; 
		// printk("copy size %x\r", sz);

		bytes_copied = 0;
		while (bytes_copied < sz) {

			rc = flash_area_read(fap_secondary_slot, bytes_copied, buf, sizeof(buf));
			assert (rc == 0);
			// printk("%02X %02X %02X %02X\r", buf[0], buf[1], buf[2], buf[3]);

			rc = flash_area_write(fap_primary_slot, bytes_copied, buf, sizeof(buf));
			if (rc)
			{
				printk("qspi write error:%d\r", rc);
				return -1;
			}

			bytes_copied += sizeof(buf);

			//feed watchdog here if needed
		}

	}

	return 0;
}

int boot_serial_uploaded_hook(int img_index, const struct flash_area *area,
		size_t size)
{
	if (img_index == NET_CORE_VIRTUAL_PRIMARY_SLOT) {
		return network_core_update(false);
	}

	return 0;
}
