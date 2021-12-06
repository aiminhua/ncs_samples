/**
 * Copyright (c) 2016 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**@file
 *
 * @defgroup nrf_dfu_settings DFU settings
 * @{
 * @ingroup  nrf_dfu
 */

#ifndef NRF_DFU_SETTINGS_H__
#define NRF_DFU_SETTINGS_H__

#include <stdint.h>
#include "nrf_dfu_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*nrf_dfu_flash_callback_t)(void * p_buf);

/**@brief   Global settings.
 *
 * @note Using this variable is not thread-safe.
 *
 */
extern nrf_dfu_settings_t s_dfu_settings;


/**@brief   Function for writing DFU settings to flash.
 *
 * @param[in]   callback    Pointer to a function that is called after completing the write operation.
 *
 * @retval  NRF_SUCCESS         If the write process was successfully initiated.
 * @retval  NRF_ERROR_INTERNAL  If a flash error occurred.
 */
ret_code_t nrf_dfu_settings_write(nrf_dfu_flash_callback_t callback);


/**@brief   Function for backing up the settings.
 *
 * This function copies the contents of the settings page (in flash) to a separate page (in flash).
 * During @ref nrf_dfu_settings_init, the backup is restored if the original is invalid.
 *
 * @param[in]   callback    Pointer to a function that is called after completing the write operation.
 */
void nrf_dfu_settings_backup(nrf_dfu_flash_callback_t callback);


/**@brief   Function for writing DFU settings to flash and to backup.
 *
 * This function first calls @ref nrf_dfu_settings_write and then @ref nrf_dfu_settings_backup.
 *
 * @param[in]   callback    Pointer to a function that is called after completing the write and backup operation.
 *
 * @retval  NRF_SUCCESS         If the write process was successfully initiated.
 * @retval  NRF_ERROR_INTERNAL  If a flash error occurred during the first write.
 */
ret_code_t nrf_dfu_settings_write_and_backup(nrf_dfu_flash_callback_t callback);


/**@brief   Function for initializing the DFU settings structure.
 *
 * Initializes the RAM structure from the flash contents.
 * This function is called as part of @ref nrf_dfu_settings_init.
 *
 * @retval  NRF_SUCCESS         If the initialization was successful.
 * @retval  NRF_ERROR_INTERNAL  If a flash error occurred.
 */
void nrf_dfu_settings_reinit(void);


/**@brief   Function for initializing the DFU settings module.
 *
 * @retval  NRF_SUCCESS         If the initialization was successful.
 * @retval  NRF_ERROR_INTERNAL  If a flash error occurred.
 */
ret_code_t nrf_dfu_settings_init(bool sd_irq_initialized);

/** @brief Function for erasing additional data in DFU settings.
 *
 * @note    Erasing additional data in DFU settings is only possible
 *          if nrf_dfu_flash is initialized to not use SoftDevice calls.
 *
 * @retval  NRF_SUCCESS     Additional data was successfully erased.
 * @retval  Any other error code reported by nrf_dfu_flash
 */
ret_code_t nrf_dfu_settings_additional_erase(void);

/** @brief Function for resetting both init command and DFU transfer progress inside settings structure.
 *
 * @note    This function does not perform flash operation.
 *          In order to save the reset state, please use @ref nrf_dfu_settings_write function.
 */
void nrf_dfu_settings_progress_reset(void);

uint32_t nrf_dfu_bank1_start_addr(void);

void update_settings_dfu_mode(uint32_t data_addr, uint32_t data_len);

#ifdef __cplusplus
}
#endif

#endif // NRF_DFU_SETTINGS_H__

/**@} */
