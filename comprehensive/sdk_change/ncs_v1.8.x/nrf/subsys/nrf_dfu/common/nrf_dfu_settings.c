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

#include <stdint.h>
#include "nrf_dfu_settings.h"
#include <stddef.h>
#include <string.h>
#include "crc32.h"
#include "sdk_config.h"
#include "nrf_dfu_types.h"
#include <logging/log.h>
#include <nrfx_nvmc.h>

#define LOG_MODULE_NAME dfu_settings
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_NRF_DFU_LOG_LEVEL);

#define DFU_SETTINGS_VERSION_OFFSET             (offsetof(nrf_dfu_settings_t, settings_version))                         //<! Offset in the settings struct where the settings version is located.
#define DFU_SETTINGS_INIT_COMMAND_OFFSET        (offsetof(nrf_dfu_settings_t, init_command))                             //<! Offset in the settings struct where the InitCommand is located.
#define DFU_SETTINGS_BOOT_VALIDATION_OFFSET     (offsetof(nrf_dfu_settings_t, boot_validation_crc))                      //<! Offset in the settings struct where the boot validation info is located.
#define DFU_SETTINGS_BOOT_VALIDATION_SIZE       ((3 * sizeof(boot_validation_t)) + 4)
#define DFU_SETTINGS_BOND_DATA_OFFSET_V1        (offsetof(nrf_dfu_settings_t, init_command) + INIT_COMMAND_MAX_SIZE_v1)  //<! Offset in the settings struct where the bond data was located in settings version 1.
#define DFU_SETTINGS_ADV_NAME_OFFSET_V1         (offsetof(nrf_dfu_settings_t, init_command) + INIT_COMMAND_MAX_SIZE_v1 + NRF_DFU_PEER_DATA_LEN)  //<! Offset in the settings struct where the bond data was located in settings version 1.


#define NRF_DFU_IN_APP 1


nrf_dfu_settings_t s_dfu_settings;
nrf_dfu_settings_t m_dfu_settings_buffer;

static uint32_t settings_crc_get(nrf_dfu_settings_t const * p_settings)
{
    BUILD_ASSERT(offsetof(nrf_dfu_settings_t, crc) == 0);

    // The crc is calculated from the s_dfu_settings struct, except the crc itself, the init command, bond data, and boot validation.
    return crc32_compute((uint8_t*)(p_settings) + 4, DFU_SETTINGS_INIT_COMMAND_OFFSET - 4, NULL);
}

static uint32_t boot_validation_crc(nrf_dfu_settings_t const * p_settings)
{
    return crc32_compute((const uint8_t *)&p_settings->boot_validation_softdevice,
                          DFU_SETTINGS_BOOT_VALIDATION_SIZE - 4,
                          NULL);
}


void nrf_dfu_settings_reinit(void)
{
    
    LOG_DBG("Resetting bootloader settings");
    memset(&s_dfu_settings, 0x00, sizeof(nrf_dfu_settings_t));
    s_dfu_settings.settings_version = NRF_DFU_SETTINGS_VERSION;

#ifdef CONFIG_BOARD_HAS_NRF5_BOOTLOADER
    memcpy(&s_dfu_settings, (void *)BOOTLOADER_SETTINGS_ADDRESS, sizeof(nrf_dfu_settings_t));
    nrf_dfu_settings_progress_reset();
#endif
    return;
}

uint32_t nrf_dfu_settings_init(bool sd_irq_initialized)
{
    uint32_t err_code;

    LOG_DBG("Calling nrf_dfu_settings_init()...");

    nrf_dfu_settings_reinit();

    err_code = nrf_dfu_settings_write_and_backup(NULL);

    if (err_code != NRF_SUCCESS)
    {
        LOG_ERR("nrf_dfu_settings_write_and_backup() failed with error: %x", err_code);
        return NRF_ERROR_INTERNAL;
    }

    return NRF_SUCCESS;
}


static uint32_t settings_write(void                   * p_dst,
                                 void const             * p_src,
                                 nrf_dfu_flash_callback_t callback,
                                 nrf_dfu_settings_t     * p_dfu_settings_buffer)
{

    if (callback != NULL)
    {
        callback(NULL);
    }

    return NRF_SUCCESS;
}

#ifdef CONFIG_BOARD_HAS_NRF5_BOOTLOADER
uint32_t nrf_dfu_bank1_start_addr(void)
{
    uint32_t bank0_addr = MBR_SIZE;
    return ALIGN_TO_PAGE(bank0_addr + s_dfu_settings.bank_0.image_size);
}

void update_settings_dfu_mode(uint32_t data_addr, uint32_t data_len)
{
    s_dfu_settings.bank_current = NRF_DFU_CURRENT_BANK_1;
    s_dfu_settings.bank_1.image_crc  = crc32_compute((uint8_t *)data_addr, data_len, NULL);
    s_dfu_settings.bank_1.image_size = data_len;
    s_dfu_settings.bank_1.bank_code = NRF_DFU_BANK_VALID_APP;
    s_dfu_settings.progress.update_start_address = data_addr;

    s_dfu_settings.crc = settings_crc_get(&s_dfu_settings);
    s_dfu_settings.boot_validation_crc = boot_validation_crc(&s_dfu_settings);  
}
#endif

uint32_t nrf_dfu_settings_write(nrf_dfu_flash_callback_t callback)
{
    static nrf_dfu_settings_t dfu_settings_buffer;
    s_dfu_settings.crc = settings_crc_get(&s_dfu_settings);
    s_dfu_settings.boot_validation_crc = boot_validation_crc(&s_dfu_settings);
    return settings_write(&s_dfu_settings,
                          &s_dfu_settings,
                          callback,
                          &dfu_settings_buffer);
}


uint32_t nrf_dfu_settings_write_and_backup(nrf_dfu_flash_callback_t callback)
{
    uint32_t err_code = nrf_dfu_settings_write(callback);
    return err_code;
}

void nrf_dfu_settings_progress_reset(void)
{
    memset(s_dfu_settings.init_command, 0xFF, INIT_COMMAND_MAX_SIZE); // Remove the last init command
    memset(&s_dfu_settings.progress, 0, sizeof(dfu_progress_t));
    s_dfu_settings.write_offset = 0;
}
