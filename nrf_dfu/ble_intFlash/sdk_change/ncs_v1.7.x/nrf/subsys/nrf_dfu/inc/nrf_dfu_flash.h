/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _NRF_DFU_FLASH_H_
#define _NRF_DFU_FLASH_H_

#include <zephyr/types.h>

bool dfu_lock(const void *module_id);
void dfu_unlock(const void *module_id);
void dfu_flash_cmd_handler(const uint8_t opt_id, const uint8_t *data,
			  const size_t size);
int dfu_data_store(int off, const void *src, size_t len, bool flush);
int dfu_page_erase(int off, size_t len);
int dfu_flash_start(uint32_t image_start, uint32_t image_len);
void dfu_flash_finish(void);

#endif /* _NRF_DFU_FLASH_H_ */
