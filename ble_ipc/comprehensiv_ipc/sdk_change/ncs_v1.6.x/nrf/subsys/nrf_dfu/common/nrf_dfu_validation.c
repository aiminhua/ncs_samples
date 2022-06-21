/**
 * Copyright (c) 2017 - 2020, Nordic Semiconductor ASA
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
#include <stdbool.h>
#include "nrf_dfu_types.h"
#include "nrf_dfu_settings.h"
#include "pb.h"
#include "pb_common.h"
#include "pb_decode.h"
#include "dfu-cc.pb.h"
#include "crc32.h"
#include "nrf_dfu_validation.h"
#include <logging/log.h>

#define LOG_MODULE_NAME dfu_validate
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_NRF_DFU_LOG_LEVEL);


#define EXT_ERR(err) (nrf_dfu_result_t)((uint32_t)NRF_DFU_RES_CODE_EXT_ERROR + (uint32_t)err)

/* Whether a complete init command has been received and prevalidated, but the firmware
 * is not yet fully transferred. This value will also be correct after reset.
 */
static bool               m_valid_init_cmd_present = false;
static dfu_packet_t       m_packet                 = DFU_PACKET_INIT_DEFAULT;
static uint8_t*           m_init_packet_data_ptr   = 0;
static uint32_t           m_init_packet_data_len   = 0;
static pb_istream_t       m_pb_stream;

static dfu_init_command_t const * mp_init = NULL;

/** @brief Flag used by parser code to indicate that the init command has been found to be invalid.
 */
static bool                                         m_init_packet_valid = false;

 static void pb_decoding_callback(pb_istream_t *str,
                                  uint32_t tag,
                                  pb_wire_type_t wire_type,
                                  void *iter)
 {
     pb_field_iter_t* p_iter = (pb_field_iter_t *) iter;

     // Match the beginning of the init command.
     if (p_iter->pos->ptr == &dfu_init_command_fields[0])
     {
         uint8_t  * ptr  = (uint8_t *)str->state;
         uint32_t   size = str->bytes_left;

         if (m_init_packet_data_ptr != NULL || m_init_packet_data_len != 0)
         {
             m_init_packet_valid = false;
             return;
         }

         // Remove tag.
         while (*ptr & 0x80)
         {
             ptr++;
             size--;
         }
         ptr++;
         size--;

         // Store the info in init_packet_data.
         m_init_packet_data_ptr = ptr;
         m_init_packet_data_len = size;
         m_init_packet_valid    = true;

         LOG_DBG("PB: Init packet data len: %d", size);
     }
 }

 /** @brief Function for decoding byte stream into variable.
  *
  *  @retval true   If the stored init command was successfully decoded.
  *  @retval false  If there was no stored init command, or the decoding failed.
  */
 static bool stored_init_cmd_decode(void)
 {
     m_pb_stream = pb_istream_from_buffer(s_dfu_settings.init_command,
                                          s_dfu_settings.progress.command_size);

     dfu_init_command_t * p_init;

     // Attach our callback to follow the field decoding.
     m_pb_stream.decoding_callback = pb_decoding_callback;

     m_init_packet_valid    = false;
     m_init_packet_data_ptr = NULL;
     m_init_packet_data_len = 0;
     memset(&m_packet, 0, sizeof(m_packet));

     if (!pb_decode(&m_pb_stream, dfu_packet_fields, &m_packet))
     {
         LOG_ERR("Handler: Invalid protocol buffer m_pb_stream");
         return false;
     }

     if (!m_init_packet_valid || (m_packet.has_signed_command && m_packet.has_command))
     {
         LOG_ERR("Handler: Invalid init command.");
         return false;
     }
     else if (m_packet.has_signed_command && m_packet.signed_command.command.has_init)
     {
         p_init = &m_packet.signed_command.command.init;

         m_pb_stream = pb_istream_from_buffer(m_init_packet_data_ptr, m_init_packet_data_len);
         memset(p_init, 0, sizeof(dfu_init_command_t));

         if (!pb_decode(&m_pb_stream, dfu_init_command_fields, p_init))
         {
             LOG_ERR("Handler: Invalid protocol buffer m_pb_stream (init command)");
             return false;
         }
     }
     else if (m_packet.has_command && m_packet.command.has_init)
     {
         p_init = &m_packet.command.init;
     }
     else
     {
         return false;
     }

     mp_init = p_init;

     return true;
 }

void nrf_dfu_validation_init(void)
{
     //If the command is stored to flash, init command was valid.
     if ((s_dfu_settings.progress.command_size != 0) &&
          stored_init_cmd_decode())
     {
         m_valid_init_cmd_present = true;
     }
     else
     {
         m_valid_init_cmd_present = false;
     }    
}


nrf_dfu_result_t nrf_dfu_validation_init_cmd_create(uint32_t size)
{
    nrf_dfu_result_t ret_val = NRF_DFU_RES_CODE_SUCCESS;
    if (size == 0)
    {
        ret_val = NRF_DFU_RES_CODE_INVALID_PARAMETER;
    }
    else if (size > INIT_COMMAND_MAX_SIZE)
    {
        ret_val = NRF_DFU_RES_CODE_INSUFFICIENT_RESOURCES;
    }
    else
    {
        // Set DFU to uninitialized.
        m_valid_init_cmd_present = false;

        // Reset all progress.
        nrf_dfu_settings_progress_reset();

        // Set the init command size.
        s_dfu_settings.progress.command_size = size;
		
    }
    return ret_val;
}


nrf_dfu_result_t nrf_dfu_validation_init_cmd_append(uint8_t const * p_data, uint32_t length)
{
    nrf_dfu_result_t ret_val = NRF_DFU_RES_CODE_SUCCESS;

    if ((length + s_dfu_settings.progress.command_offset) > s_dfu_settings.progress.command_size)
    {
        LOG_ERR("Init command larger than expected.");
        ret_val = NRF_DFU_RES_CODE_INVALID_PARAMETER;
    }
    else
    {
        // Copy the received data to RAM, update offset and calculate CRC.
        memcpy(&s_dfu_settings.init_command[s_dfu_settings.progress.command_offset],
                p_data,
                length);

        s_dfu_settings.progress.command_offset += length;
        s_dfu_settings.progress.command_crc = crc32_compute(p_data,
                                                            length,
                                                            &s_dfu_settings.progress.command_crc);
    }
    return ret_val;
}


void nrf_dfu_validation_init_cmd_status_get(uint32_t * p_offset,
                                            uint32_t * p_crc,
                                            uint32_t * p_max_size)
{
    *p_offset   = s_dfu_settings.progress.command_offset;
    *p_crc      = s_dfu_settings.progress.command_crc;
    *p_max_size = INIT_COMMAND_MAX_SIZE;
}


bool nrf_dfu_validation_init_cmd_present(void)
{
    return m_valid_init_cmd_present;    
}

// Function to calculate the total size of the firmware(s) in the update.
static nrf_dfu_result_t update_data_size_get(dfu_init_command_t const * p_init, uint32_t * p_size)
{
    nrf_dfu_result_t ret_val = EXT_ERR(NRF_DFU_EXT_ERROR_INIT_COMMAND_INVALID);
    uint32_t         fw_sz   = 0;

    fw_sz = p_init->app_size;

    if (fw_sz)
    {
        *p_size = fw_sz;
        ret_val = NRF_DFU_RES_CODE_SUCCESS;
    }
    else
    {
        LOG_ERR("Init packet does not contain valid firmware size");
    }

    return ret_val;
}

/**@brief Function to determine where to temporarily store the incoming firmware.
 *        This also checks whether the update will fit, and deletes existing
 *        firmware to make room for the new firmware.
 *
 * @param[in]  p_init   Init command.
 * @param[in]  fw_size  The size of the incoming firmware.
 * @param[out] p_addr   The address at which to initially store the firmware.
 *
 * @retval NRF_DFU_RES_CODE_SUCCESS                 If the size check passed and
 *                                                  an address was found.
 * @retval NRF_DFU_RES_CODE_INSUFFICIENT_RESOURCES  If the size check failed.
 */
static nrf_dfu_result_t update_data_addr_get(dfu_init_command_t const * p_init,
                                             uint32_t                   fw_size,
                                             uint32_t                 * p_addr)
{    
    *p_addr = 0;
#ifdef CONFIG_BOARD_HAS_NRF5_BOOTLOADER
    *p_addr = nrf_dfu_bank1_start_addr();
#endif    
    return NRF_DFU_RES_CODE_SUCCESS;
}


nrf_dfu_result_t nrf_dfu_validation_init_cmd_execute(uint32_t * p_dst_data_addr,
                                                     uint32_t * p_data_len)
{
    nrf_dfu_result_t ret_val = NRF_DFU_RES_CODE_SUCCESS;

     if (s_dfu_settings.progress.command_offset != s_dfu_settings.progress.command_size)
     {
         // The object wasn't the right (requested) size.
         LOG_ERR("Execute with faulty offset");
         ret_val = NRF_DFU_RES_CODE_OPERATION_NOT_PERMITTED;
     }
     else if (m_valid_init_cmd_present)
     {         
         *p_dst_data_addr = 0;
#ifdef CONFIG_BOARD_HAS_NRF5_BOOTLOADER
        *p_dst_data_addr = nrf_dfu_bank1_start_addr();
#endif
         ret_val          = update_data_size_get(mp_init, p_data_len);
     }
     else if (stored_init_cmd_decode())
     {
         *p_dst_data_addr = 0;
         *p_data_len      = 0;

         ret_val = update_data_size_get(mp_init, p_data_len);

         // Get address where to flash the binary.
         if (ret_val == NRF_DFU_RES_CODE_SUCCESS)
         {
             ret_val = update_data_addr_get(mp_init, *p_data_len, p_dst_data_addr);
         }

         // Set flag validating the init command.
         if (ret_val == NRF_DFU_RES_CODE_SUCCESS)
         {
             m_valid_init_cmd_present = true;
         }
         else
         {
             nrf_dfu_settings_progress_reset();
         }
     }
     else
     {
         LOG_ERR("Failed to decode init packet");
         ret_val = NRF_DFU_RES_CODE_INVALID_OBJECT;
     }

    return ret_val;
}

