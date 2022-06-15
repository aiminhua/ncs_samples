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
#include <zephyr.h>
#include <stdint.h>
#include <stdbool.h>
#include "sdk_config.h"
#include "nrf_dfu.h"
#include "nrf_dfu_types.h"
#include "nrf_dfu_req_handler.h"
#include "nrf_dfu_handling_error.h"
#include "app_util.h"
#include "crc32.h"
#include "nrf_dfu_validation.h"
#include "nrf_dfu_settings.h"
#include <logging/log.h>
#include "nrf_dfu_flash.h"
#include <logging/log_ctrl.h>
#ifdef CONFIG_BOOTLOADER_MCUBOOT 
#include "pm_config.h"
#endif

#define LOG_MODULE_NAME nrf_dfu_req
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_NRF_DFU_LOG_LEVEL);

#define NRF_DFU_PROTOCOL_VERSION    (0x01)

static uint32_t m_firmware_start_addr;          /**< Start address of the current firmware image. */
static uint32_t m_firmware_size_req;            /**< The size of the entire firmware image. Defined by the init command. */

static nrf_dfu_observer_t m_observer;

typedef struct {
	void *fifo_reserved;
	nrf_dfu_request_t req;	
}dfu_data_t;

static K_FIFO_DEFINE(fifo_dfu_data);

static nrf_dfu_result_t ext_err_code_handle(nrf_dfu_result_t ret_val)
{
    if (ret_val < NRF_DFU_RES_CODE_EXT_ERROR)
    {
        return ret_val;
    }
    else
    {
        nrf_dfu_ext_error_code_t ext_err =
                (nrf_dfu_ext_error_code_t)((uint8_t)ret_val - (uint8_t)NRF_DFU_RES_CODE_EXT_ERROR);
        return ext_error_set(ext_err);
    }
}


static void on_prn_set_request(nrf_dfu_request_t * p_req, nrf_dfu_response_t * p_res)
{
    UNUSED_PARAMETER(p_req);
    UNUSED_PARAMETER(p_res);
    LOG_DBG("Handle NRF_DFU_OP_RECEIPT_NOTIF_SET");
}


static void on_abort_request(nrf_dfu_request_t * p_req, nrf_dfu_response_t * p_res)
{
    UNUSED_PARAMETER(p_req);
    UNUSED_PARAMETER(p_res);
    LOG_DBG("Handle NRF_DFU_OP_ABORT");

    m_observer(NRF_DFU_EVT_DFU_ABORTED);
}


/* Set offset and CRC fields in the response for a 'command' message. */
static void cmd_response_offset_and_crc_set(nrf_dfu_response_t * const p_res)
{
    __ASSERT_NO_MSG(p_res);

    /* Copy the CRC and offset of the init packet. */
    p_res->crc.offset = s_dfu_settings.progress.command_offset;
    p_res->crc.crc    = s_dfu_settings.progress.command_crc;
}


static void on_cmd_obj_select_request(nrf_dfu_request_t const * p_req, nrf_dfu_response_t * p_res)
{
    UNUSED_PARAMETER(p_req);
    LOG_DBG("Handle NRF_DFU_OP_OBJECT_SELECT (command)");

    p_res->select.max_size = INIT_COMMAND_MAX_SIZE;
    cmd_response_offset_and_crc_set(p_res);
}


static void on_cmd_obj_create_request(nrf_dfu_request_t * p_req, nrf_dfu_response_t * p_res)
{
    __ASSERT_NO_MSG(p_req);
    __ASSERT_NO_MSG(p_res);

    LOG_DBG("Handle NRF_DFU_OP_OBJECT_CREATE (command)");

    m_observer(NRF_DFU_EVT_DFU_STARTED);

    nrf_dfu_result_t ret_val = nrf_dfu_validation_init_cmd_create(p_req->create.object_size);
    p_res->result = ext_err_code_handle(ret_val);
}


static void on_cmd_obj_write_request(nrf_dfu_request_t * p_req, nrf_dfu_response_t * p_res)
{
    __ASSERT_NO_MSG(p_req != NULL);
    __ASSERT_NO_MSG(p_req->write.p_data != NULL);
    __ASSERT_NO_MSG(p_req->write.len != 0);
    __ASSERT_NO_MSG(p_res != NULL);

    LOG_DBG("Handle NRF_DFU_OP_OBJECT_WRITE (command)");

    nrf_dfu_result_t ret_val;

    ret_val = nrf_dfu_validation_init_cmd_append(p_req->write.p_data, p_req->write.len);
    p_res->result = ext_err_code_handle(ret_val);

    /* Update response. This is only used when the PRN is triggered and the 'write' message
     * is answered with a CRC message and these field are copied into the response. */
    cmd_response_offset_and_crc_set(p_res);

    /* If a callback to free the request payload buffer was provided, invoke it now. */
    //if (p_req->callback.write)
    //{
    //    p_req->callback.write((void*)p_req->write.p_data);
    //}
}


static void on_cmd_obj_execute_request(nrf_dfu_request_t const * p_req, nrf_dfu_response_t * p_res)
{
    __ASSERT_NO_MSG(p_req);
    __ASSERT_NO_MSG(p_res);

    LOG_DBG("Handle NRF_DFU_OP_OBJECT_EXECUTE (command)");

    nrf_dfu_result_t ret_val;
    ret_val = nrf_dfu_validation_init_cmd_execute(&m_firmware_start_addr, &m_firmware_size_req);
    p_res->result = ext_err_code_handle(ret_val);
    LOG_INF("new fw start=%x, fw size=%x ", m_firmware_start_addr, m_firmware_size_req);

    if (p_res->result == NRF_DFU_RES_CODE_SUCCESS)
    {
        if (nrf_dfu_settings_write_and_backup(NULL) == NRF_SUCCESS)
        {
            /* Setting DFU to initialized */
            LOG_DBG("Writing valid init command to flash.");
        }
        else
        {
            p_res->result = NRF_DFU_RES_CODE_OPERATION_FAILED;
        }
    }
    int err = dfu_flash_start(m_firmware_start_addr, m_firmware_size_req);
    if (err)
    {
        p_res->result = NRF_DFU_RES_CODE_OPERATION_FAILED;
    }
}


static void on_cmd_obj_crc_request(nrf_dfu_request_t const * p_req, nrf_dfu_response_t * p_res)
{
    UNUSED_PARAMETER(p_req);
    LOG_DBG("Handle NRF_DFU_OP_CRC_GET (command)");

    cmd_response_offset_and_crc_set(p_res);
}


/** @brief Function handling command requests from the transport layer.
 *
 * @param   p_req[in]       Pointer to the structure holding the DFU request.
 * @param   p_res[out]      Pointer to the structure holding the DFU response.
 *
 * @retval NRF_SUCCESS      If the command request was executed successfully.
 *                          Any other error code indicates that the data request
 *                          could not be handled.
 */
static void nrf_dfu_command_req(nrf_dfu_request_t * p_req, nrf_dfu_response_t * p_res)
{
    __ASSERT_NO_MSG(p_req);
    __ASSERT_NO_MSG(p_res);

    switch (p_req->request)
    {
        case NRF_DFU_OP_OBJECT_CREATE:
        {
            on_cmd_obj_create_request(p_req, p_res);
        } break;

        case NRF_DFU_OP_CRC_GET:
        {
            on_cmd_obj_crc_request(p_req, p_res);
        } break;

        case NRF_DFU_OP_OBJECT_WRITE:
        {
            on_cmd_obj_write_request(p_req, p_res);
        } break;

        case NRF_DFU_OP_OBJECT_EXECUTE:
        {
            on_cmd_obj_execute_request(p_req, p_res);
        } break;

        case NRF_DFU_OP_OBJECT_SELECT:
        {
            on_cmd_obj_select_request(p_req, p_res);
        } break;

        default:
        {
            __ASSERT_NO_MSG(false);
        } break;
    }
}


static void on_data_obj_select_request(nrf_dfu_request_t * p_req, nrf_dfu_response_t * p_res)
{
    LOG_DBG("Handle NRF_DFU_OP_OBJECT_SELECT (data)");

    p_res->select.crc    = s_dfu_settings.progress.firmware_image_crc;
    p_res->select.offset = s_dfu_settings.progress.firmware_image_offset;

    p_res->select.max_size = DATA_OBJECT_MAX_SIZE;

    LOG_DBG("crc = 0x%x, offset = 0x%x, max_size = 0x%x",
                  p_res->select.crc,
                  p_res->select.offset,
                  p_res->select.max_size);
}


static void on_data_obj_create_request(nrf_dfu_request_t * p_req, nrf_dfu_response_t * p_res)
{
    LOG_DBG("Handle NRF_DFU_OP_OBJECT_CREATE (data)");

    if (!nrf_dfu_validation_init_cmd_present())
    {
        /* Can't accept data because DFU isn't initialized by init command. */
        LOG_ERR("Cannot create data object without valid init command");
        p_res->result = NRF_DFU_RES_CODE_OPERATION_NOT_PERMITTED;
        return;
    }

    if (p_req->create.object_size == 0)
    {
        LOG_ERR("Object size cannot be 0.");
        p_res->result = NRF_DFU_RES_CODE_INVALID_PARAMETER;
        return;
    }

    if (  ((p_req->create.object_size & (CODE_PAGE_SIZE - 1)) != 0)
        && (s_dfu_settings.progress.firmware_image_offset_last + p_req->create.object_size != m_firmware_size_req))
    {
        LOG_ERR("Object size must be page aligned");
        p_res->result = NRF_DFU_RES_CODE_INVALID_PARAMETER;
        return;
    }

    if (p_req->create.object_size > DATA_OBJECT_MAX_SIZE)
    {
        /* It is impossible to handle the command because the size is too large */
        LOG_ERR("Invalid size for object (too large)");
        p_res->result = NRF_DFU_RES_CODE_INSUFFICIENT_RESOURCES;
        return;
    }

    if ((s_dfu_settings.progress.firmware_image_offset_last + p_req->create.object_size) >
        m_firmware_size_req)
    {
        LOG_ERR("Creating the object with size 0x%08x would overflow firmware size. "
                      "Offset is 0x%08x and firmware size is 0x%08x.",
                      p_req->create.object_size,
                      s_dfu_settings.progress.firmware_image_offset_last,
                      m_firmware_size_req);

        p_res->result = NRF_DFU_RES_CODE_OPERATION_NOT_PERMITTED;
        return;
    }

    s_dfu_settings.progress.data_object_size      = p_req->create.object_size;
    s_dfu_settings.progress.firmware_image_crc    = s_dfu_settings.progress.firmware_image_crc_last;
    s_dfu_settings.progress.firmware_image_offset = s_dfu_settings.progress.firmware_image_offset_last;
    s_dfu_settings.write_offset                   = s_dfu_settings.progress.firmware_image_offset_last;

    /* Erase the page we're at. */
    if (dfu_page_erase(m_firmware_start_addr + s_dfu_settings.progress.firmware_image_offset, p_req->create.object_size))                      
    {
        LOG_ERR("Erase operation failed");
        p_res->result = NRF_DFU_RES_CODE_INVALID_OBJECT;
        return;
    }

    LOG_DBG("Creating object with size: %d. Offset: 0x%08x, CRC: 0x%08x",
                 s_dfu_settings.progress.data_object_size,
                 s_dfu_settings.progress.firmware_image_offset,
                 s_dfu_settings.progress.firmware_image_crc);
}


static void on_data_obj_write_request(nrf_dfu_request_t * p_req, nrf_dfu_response_t * p_res)
{
    LOG_DBG("Handle NRF_DFU_OP_OBJECT_WRITE (data)");

    if (!nrf_dfu_validation_init_cmd_present())
    {
        /* Can't accept data because DFU isn't initialized by init command. */
        p_res->result = NRF_DFU_RES_CODE_OPERATION_NOT_PERMITTED;
        return;
    }

    uint32_t const data_object_offset = s_dfu_settings.progress.firmware_image_offset -
                                        s_dfu_settings.progress.firmware_image_offset_last;

    if ((p_req->write.len + data_object_offset) > s_dfu_settings.progress.data_object_size)
    {
        /* Can't accept data because too much data has been received. */
        LOG_ERR("Write request too long");
        p_res->result = NRF_DFU_RES_CODE_INVALID_PARAMETER;
        return;
    }

    uint32_t const write_addr = m_firmware_start_addr + s_dfu_settings.write_offset;
    /* CRC must be calculated before handing off the data to fstorage because the data is
     * freed on write completion.
     */
    uint32_t const next_crc =
        crc32_compute(p_req->write.p_data, p_req->write.len, &s_dfu_settings.progress.firmware_image_crc);

    // __ASSERT_NO_MSG(p_req->callback.write);
    int ret;
    if ((s_dfu_settings.progress.firmware_image_offset + p_req->write.len) == m_firmware_size_req)
    {
        LOG_DBG("last image packet");
        ret = dfu_data_store(write_addr, p_req->write.p_data, p_req->write.len, true);        
    }
    else
    {
        ret = dfu_data_store(write_addr, p_req->write.p_data, p_req->write.len, false);
    }
    // int ret = dfu_data_store(write_addr, p_req->write.p_data, p_req->write.len);
				   
    // uint32_t ret =
    //     nrf_dfu_flash_store(write_addr, p_req->write.p_data, p_req->write.len, p_req->callback.write);
    // p_req->callback.write((void*)p_req->write.p_data);
    // if (ret != NRF_SUCCESS)
    if (ret)
    {
        /* When nrf_dfu_flash_store() fails because there is no space in the queue,
         * stop processing the request so that the peer can detect a CRC error
         * and retransmit this object. Remember to manually free the buffer !
         */
        // p_req->callback.write((void*)p_req->write.p_data);
        LOG_ERR("DFU write error %d", ret);
        return;
    }

    /* Update the CRC of the firmware image. */
    s_dfu_settings.write_offset                   += p_req->write.len;
    s_dfu_settings.progress.firmware_image_offset += p_req->write.len;
    s_dfu_settings.progress.firmware_image_crc     = next_crc;

    /* This is only used when the PRN is triggered and the 'write' message
     * is answered with a CRC message and these field are copied into the response.
     */
    p_res->write.crc    = s_dfu_settings.progress.firmware_image_crc;
    p_res->write.offset = s_dfu_settings.progress.firmware_image_offset;
}


static void on_data_obj_crc_request(nrf_dfu_request_t * p_req, nrf_dfu_response_t * p_res)
{
    LOG_DBG("Handle NRF_DFU_OP_CRC_GET (data)");
    LOG_DBG("Offset:0x%x, CRC:0x%08x",
                 s_dfu_settings.progress.firmware_image_offset,
                 s_dfu_settings.progress.firmware_image_crc);

    p_res->crc.crc    = s_dfu_settings.progress.firmware_image_crc;
    p_res->crc.offset = s_dfu_settings.progress.firmware_image_offset;
}


static void on_data_obj_execute_request_sched(void * p_evt, uint16_t event_length)
{
    UNUSED_PARAMETER(event_length);

    uint32_t          ret;
    nrf_dfu_request_t * p_req = (nrf_dfu_request_t *)(p_evt);

    nrf_dfu_response_t res =
    {
        .request = NRF_DFU_OP_OBJECT_EXECUTE,
    };

    if (s_dfu_settings.progress.firmware_image_offset == m_firmware_size_req)
    {
        LOG_INF("Whole firmware image received.");

        res.result = NRF_DFU_RES_CODE_SUCCESS;

        res.result = ext_err_code_handle(res.result);

        /* Provide response to transport */
        p_req->callback.response(&res, p_req->p_context);        

#ifdef CONFIG_BOARD_HAS_NRF5_BOOTLOADER
        update_settings_dfu_mode(m_firmware_start_addr, m_firmware_size_req);
#endif
        dfu_flash_finish();

        m_observer(NRF_DFU_EVT_DFU_COMPLETED);       
        
    }
    else
    {
        res.result = NRF_DFU_RES_CODE_SUCCESS;

        /* Provide response to transport */
        p_req->callback.response(&res, p_req->p_context);

        if (NRF_DFU_SAVE_PROGRESS_IN_FLASH)
        {
            /* Allowing skipping settings backup to save time and flash wear. */
            ret = nrf_dfu_settings_write_and_backup(NULL);
            UNUSED_RETURN_VALUE(ret);
        }
    }

    LOG_DBG("Request handling complete. Result: 0x%x fw offset=0x%x", res.result, s_dfu_settings.progress.firmware_image_offset);
}


static bool on_data_obj_execute_request(nrf_dfu_request_t * p_req, nrf_dfu_response_t * p_res)
{
    LOG_DBG("Handle NRF_DFU_OP_OBJECT_EXECUTE (data)");

    uint32_t const data_object_size = s_dfu_settings.progress.firmware_image_offset -
                                      s_dfu_settings.progress.firmware_image_offset_last;

    if (s_dfu_settings.progress.data_object_size != data_object_size)
    {
        // /* The size of the written object was not as expected. */
        LOG_ERR("Invalid data. expected: %d, got: %d",
                      s_dfu_settings.progress.data_object_size,
                      data_object_size);

        // p_res->result = NRF_DFU_RES_CODE_OPERATION_NOT_PERMITTED;
        // return true;
        nrf_dfu_settings_progress_reset();
    }

    /* Update the offset and crc values for the last object written. */
    s_dfu_settings.progress.data_object_size           = 0;
    s_dfu_settings.progress.firmware_image_crc_last    = s_dfu_settings.progress.firmware_image_crc;
    s_dfu_settings.progress.firmware_image_offset_last = s_dfu_settings.progress.firmware_image_offset;

    on_data_obj_execute_request_sched(p_req, 0);

    m_observer(NRF_DFU_EVT_OBJECT_RECEIVED);

    return false;
}


static bool nrf_dfu_data_req(nrf_dfu_request_t * p_req, nrf_dfu_response_t * p_res)
{
    __ASSERT_NO_MSG(p_req);
    __ASSERT_NO_MSG(p_res);

    bool response_ready = true;

    switch (p_req->request)
    {
        case NRF_DFU_OP_OBJECT_CREATE:
        {
            on_data_obj_create_request(p_req, p_res);
        } break;

        case NRF_DFU_OP_OBJECT_WRITE:
        {
            on_data_obj_write_request(p_req, p_res);
        } break;

        case NRF_DFU_OP_CRC_GET:
        {
            on_data_obj_crc_request(p_req, p_res);
        } break;

        case NRF_DFU_OP_OBJECT_EXECUTE:
        {
            response_ready = on_data_obj_execute_request(p_req, p_res);
        } break;

        case NRF_DFU_OP_OBJECT_SELECT:
        {
            on_data_obj_select_request(p_req, p_res);
        } break;

        default:
        {
            __ASSERT_NO_MSG(false);
        } break;
    }

    return response_ready;
}


/**@brief Function for handling requests to manipulate data or command objects.
 *
 * @param[in]  p_req    Request.
 * @param[out] p_res    Response.
 *
 * @return  Whether response is ready to be sent.
 */
static bool nrf_dfu_obj_op(nrf_dfu_request_t * p_req, nrf_dfu_response_t * p_res)
{
    /* Keep track of the current object type since write and execute requests don't contain it. */
    static nrf_dfu_obj_type_t current_object = NRF_DFU_OBJ_TYPE_COMMAND;

    if (    (p_req->request == NRF_DFU_OP_OBJECT_SELECT)
        ||  (p_req->request == NRF_DFU_OP_OBJECT_CREATE))
    {
        __ASSERT(offsetof(nrf_dfu_request_select_t, object_type) ==
                      offsetof(nrf_dfu_request_create_t, object_type),
                      "Wrong object_type offset!");

        current_object = (nrf_dfu_obj_type_t)(p_req->select.object_type);
    }

    bool response_ready = true;

    switch (current_object)
    {
        case NRF_DFU_OBJ_TYPE_COMMAND:
            nrf_dfu_command_req(p_req, p_res);
            break;

        case NRF_DFU_OBJ_TYPE_DATA:
            response_ready = nrf_dfu_data_req(p_req, p_res);
            break;

        default:
            /* The select request had an invalid object type. */
            LOG_ERR("Invalid object type in request.");
            current_object = NRF_DFU_OBJ_TYPE_INVALID;
            p_res->result  = NRF_DFU_RES_CODE_INVALID_OBJECT;
            break;
    }

    return response_ready;
}

#ifndef CONFIG_NRF_DFU_PROTOCOL_REDUCED
static void on_protocol_version_request(nrf_dfu_request_t const * p_req, nrf_dfu_response_t * p_res)
{
    UNUSED_PARAMETER(p_req);
    LOG_DBG("Handle NRF_DFU_OP_PROTOCOL_VERSION");

    if (NRF_DFU_PROTOCOL_VERSION_MSG)
    {
        p_res->protocol.version = NRF_DFU_PROTOCOL_VERSION;
    }
    else
    {
        LOG_DBG("NRF_DFU_OP_PROTOCOL_VERSION disabled.");
        p_res->result = NRF_DFU_RES_CODE_OP_CODE_NOT_SUPPORTED;
    }
}


static void on_hw_version_request(nrf_dfu_request_t const * p_req, nrf_dfu_response_t * p_res)
{
    LOG_DBG("Handle NRF_DFU_OP_HARDWARE_VERSION");

    p_res->hardware.part    = NRF_FICR->INFO.PART;
    p_res->hardware.variant = NRF_FICR->INFO.VARIANT;

    /* FICR values are in Kilobytes, we report them in bytes. */
    p_res->hardware.memory.ram_size      = NRF_FICR->INFO.RAM   * 1024;
    p_res->hardware.memory.rom_size      = NRF_FICR->INFO.FLASH * 1024;
#if defined(CONFIG_SOC_SERIES_NRF52X) || defined(CONFIG_SOC_SERIES_NRF51X)   
    p_res->hardware.memory.rom_page_size = NRF_FICR->CODEPAGESIZE;
#else
    p_res->hardware.memory.rom_page_size = NRF_FICR->INFO.CODEPAGESIZE;
#endif
}


static void on_fw_version_request(nrf_dfu_request_t const * p_req, nrf_dfu_response_t * p_res)
{
    LOG_DBG("Handle NRF_DFU_OP_FIRMWARE_VERSION");
    LOG_DBG("Firmware image requested: %d", p_req->firmware.image_number);

    if (NRF_DFU_PROTOCOL_FW_VERSION_MSG)
    {
        uint8_t fw_count = 1;

        if (s_dfu_settings.bank_0.bank_code == NRF_DFU_BANK_VALID_APP)
        {
            fw_count++;
        }

        p_res->result = NRF_DFU_RES_CODE_SUCCESS;

        if (p_req->firmware.image_number == 0)
        {
            /* Bootloader is always present and it is always image zero. */
            p_res->firmware.type    = NRF_DFU_FIRMWARE_TYPE_BOOTLOADER;
            p_res->firmware.version = s_dfu_settings.bootloader_version;
        #ifdef CONFIG_BOOTLOADER_MCUBOOT    
            p_res->firmware.addr    = PM_MCUBOOT_ADDRESS;
            p_res->firmware.len     = PM_MCUBOOT_SIZE;
        #else
            p_res->firmware.addr    = BOOTLOADER_SETTINGS_ADDRESS;
            p_res->firmware.len     = 0x8000;            
        #endif    
        }
        else if ((p_req->firmware.image_number < fw_count))
        {
            /* Either there is no SoftDevice and the firmware image requested is one,
             * or there is a SoftDevice and the firmware image requested is two.
             */
            p_res->firmware.type    = NRF_DFU_FIRMWARE_TYPE_APPLICATION;
            p_res->firmware.version = s_dfu_settings.app_version;
        #ifdef CONFIG_BOOTLOADER_MCUBOOT    
            p_res->firmware.addr    = PM_MCUBOOT_END_ADDRESS;
        #else
            p_res->firmware.addr    = 0x1000;
        #endif    
            p_res->firmware.len     = s_dfu_settings.bank_0.image_size;
        }
        else
        {
            LOG_DBG("No such firmware image");
            p_res->firmware.type    = NRF_DFU_FIRMWARE_TYPE_UNKNOWN;
            p_res->firmware.version = 0x00;
            p_res->firmware.addr    = 0x00;
            p_res->firmware.len     = 0x00;
        }
    }
    else
    {
        LOG_DBG("NRF_DFU_OP_FIRMWARE_VERSION disabled.");
        p_res->result        = NRF_DFU_RES_CODE_OP_CODE_NOT_SUPPORTED;
        p_res->firmware.type = NRF_DFU_FIRMWARE_TYPE_UNKNOWN;
    }
}


static void on_ping_request(nrf_dfu_request_t * p_req, nrf_dfu_response_t * p_res)
{
    LOG_DBG("Handle NRF_DFU_OP_PING");
    p_res->ping.id = p_req->ping.id;
}


static void on_mtu_get_request(nrf_dfu_request_t * p_req, nrf_dfu_response_t * p_res)
{
    LOG_DBG("Handle NRF_DFU_OP_MTU_GET");
    p_res->mtu.size = p_req->mtu.size;
}
#endif // CONFIG_NRF_DFU_PROTOCOL_REDUCED


static void nrf_dfu_req_handler_req_process(nrf_dfu_request_t * p_req)
{
    __ASSERT_NO_MSG(p_req->callback.response);

    bool response_ready = true;

    /* The request handlers assume these values to be set. */
    nrf_dfu_response_t response =
    {
        .request = p_req->request,
        .result  = NRF_DFU_RES_CODE_SUCCESS,
    };


    switch (p_req->request)
    {
#ifndef CONFIG_NRF_DFU_PROTOCOL_REDUCED
        case NRF_DFU_OP_PROTOCOL_VERSION:
        {
            on_protocol_version_request(p_req, &response);
        } break;

        case NRF_DFU_OP_HARDWARE_VERSION:
        {
            on_hw_version_request(p_req, &response);
        } break;

        case NRF_DFU_OP_FIRMWARE_VERSION:
        {
            on_fw_version_request(p_req, &response);
        } break;

        case NRF_DFU_OP_PING:
        {
            on_ping_request(p_req, &response);
        } break;

        case NRF_DFU_OP_MTU_GET:
        {
            on_mtu_get_request(p_req, &response);
        } break;
#endif        
        case NRF_DFU_OP_RECEIPT_NOTIF_SET:
        {
            on_prn_set_request(p_req, &response);
        } break;

        case NRF_DFU_OP_ABORT:
        {
            on_abort_request(p_req, &response);
        } break;

        case NRF_DFU_OP_OBJECT_CREATE:
            /* Restart the inactivity timer on CREATE messages. */
            /* Fallthrough. */
        case NRF_DFU_OP_OBJECT_SELECT:
        case NRF_DFU_OP_OBJECT_WRITE:
        case NRF_DFU_OP_OBJECT_EXECUTE:
        case NRF_DFU_OP_CRC_GET:
        {
            response_ready = nrf_dfu_obj_op(p_req, &response);
        } break;

        default:
            LOG_INF("Invalid opcode received: 0x%x.", p_req->request);
            response.result = NRF_DFU_RES_CODE_OP_CODE_NOT_SUPPORTED;
            break;
    }

    if (response_ready)
    {
        LOG_DBG("Request handling complete. Result: 0x%x", response.result);

        p_req->callback.response(&response, p_req->p_context);

        if (response.result != NRF_DFU_RES_CODE_SUCCESS)
        {
            m_observer(NRF_DFU_EVT_DFU_FAILED);
        }
    }
}

void req_handler_thread(void)
{	
	LOG_INF("DFU thread created");

	while (1) {
        dfu_data_t * dfu_req = k_fifo_get(&fifo_dfu_data,
						     K_FOREVER);
        nrf_dfu_req_handler_req_process(&dfu_req->req);
        if(dfu_req->req.request == NRF_DFU_OP_OBJECT_WRITE) 
        {
            k_free(dfu_req->req.write.p_data);	
        }  	        
        k_free(dfu_req);	
	}
}

uint32_t nrf_dfu_req_handler_on_req(nrf_dfu_request_t * p_req)
{
    uint32_t ret = NRF_SUCCESS;

    if (p_req->callback.response == NULL)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    dfu_data_t *dfu_req = k_malloc(sizeof(dfu_data_t));
    memcpy(&dfu_req->req, p_req, sizeof(nrf_dfu_request_t));
    if(p_req->request == NRF_DFU_OP_OBJECT_WRITE) 
    {
        uint8_t * data = k_malloc(p_req->write.len);
        memcpy(data, p_req->write.p_data, p_req->write.len);
        dfu_req->req.write.p_data = data;
    }  		
    k_fifo_put(&fifo_dfu_data, dfu_req);

    // nrf_dfu_req_handler_req_process(p_req);
    
    return ret;
}

uint32_t nrf_dfu_req_handler_init(nrf_dfu_observer_t observer)
{
    // uint32_t       ret_val;
    nrf_dfu_result_t result;

    if (observer == NULL)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    nrf_dfu_validation_init();
    if (nrf_dfu_validation_init_cmd_present())
    {
        /* Execute a previously received init packed. Subsequent executes will have no effect. */
        result = nrf_dfu_validation_init_cmd_execute(&m_firmware_start_addr, &m_firmware_size_req);
        if (result != NRF_DFU_RES_CODE_SUCCESS)
        {
            /* Init packet in flash is not valid! */
            return NRF_ERROR_INTERNAL;
        }
    }


    m_observer = observer;

    /* Initialize extended error handling with "No error" as the most recent error. */
    result = ext_error_set(NRF_DFU_EXT_ERROR_NO_ERROR);
    UNUSED_RETURN_VALUE(result);

    return NRF_SUCCESS;
}

K_THREAD_DEFINE(req_handler_thread_id, CONFIG_NRF_DFU_THREAD_STACK_SIZE, req_handler_thread, NULL, NULL,
		NULL, CONFIG_NRF_DFU_THREAD_PRIO, 0, 0);
