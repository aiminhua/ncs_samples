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



#ifndef SDK_CONFIG_H
#define SDK_CONFIG_H

// <h> nRF_DFU 

//==========================================================
// <h> DFU security - nrf_dfu_validation - DFU validation

//==========================================================
// <q> NRF_DFU_APP_ACCEPT_SAME_VERSION  - Whether to accept application upgrades with the same version as the current application.
 

// <i> This applies to application updates, and possibly to SoftDevice updates.
// <i> Bootloader upgrades always require higher versions. SoftDevice upgrades
// <i> look at the sd_req field independently of this config.
// <i> Disabling this protects against replay attacks wearing out the flash of the device.
// <i> This config only has an effect when NRF_DFU_APP_DOWNGRADE_PREVENTION is enabled.

#ifndef NRF_DFU_APP_ACCEPT_SAME_VERSION
#define NRF_DFU_APP_ACCEPT_SAME_VERSION 1
#endif

// <q> NRF_DFU_APP_DOWNGRADE_PREVENTION  - Check the firmware version and SoftDevice requirements of application (and SoftDevice) updates.
 

// <i> Whether to check the incoming version against the version of the existing app and/or
// <i> the incoming SoftDevice requirements against the existing SoftDevice.
// <i> This applies to application updates, and possibly to SoftDevice updates.
// <i> Disabling this causes the checks to always ignore the incoming firmware version and
// <i> to ignore the SoftDevice requirements if the first requirement is 0.
// <i> This does not apply the bootloader updates. If the bootloader depends on the SoftDevice
// <i> e.g. for BLE transport, this does not apply to SoftDevice updates.
// <i> See @ref lib_bootloader_dfu_validation for more information.
// <i> When signed updates are required, version checking should always be enabled.

#ifndef NRF_DFU_APP_DOWNGRADE_PREVENTION
#define NRF_DFU_APP_DOWNGRADE_PREVENTION 1
#endif

// <q> NRF_DFU_EXTERNAL_APP_VERSIONING  - Require versioning for external applications.
 

// <i> This configuration is only used if NRF_DFU_SUPPORTS_EXTERNAL_APP is set to 1.
// <i> Setting this will require that any FW images using the FW upgrade type 
// <i> DFU_FW_TYPE_EXTERNAL_APPLICATION must follow a monotonic versioning scheme
// <i> where the FW version of an upgrade must always be larger than the previously stored 
// <i> FW version.

#ifndef NRF_DFU_EXTERNAL_APP_VERSIONING
#define NRF_DFU_EXTERNAL_APP_VERSIONING 1
#endif

// <q> NRF_DFU_FORCE_DUAL_BANK_APP_UPDATES  - Accept only dual-bank application updates.
 

// <i> If not enabled then if there is not enough space to perform dual-bank update
// <i> application is deleted and single-bank update is performed. In case it is considered
// <i> security concern user can prefer to discard update request rather than overwrite
// <i> current application.

#ifndef NRF_DFU_FORCE_DUAL_BANK_APP_UPDATES
#define NRF_DFU_FORCE_DUAL_BANK_APP_UPDATES 1
#endif

// <o> NRF_DFU_HW_VERSION - Device hardware version. 
// <i> This is used to determine if given update is targeting the device.
// <i> It is checked against the hw_version value in the init packet

#ifndef NRF_DFU_HW_VERSION
#define NRF_DFU_HW_VERSION 52
#endif

// <q> NRF_DFU_REQUIRE_SIGNED_APP_UPDATE  - Require a valid signature to update the application or SoftDevice.
 

#ifndef NRF_DFU_REQUIRE_SIGNED_APP_UPDATE
#define NRF_DFU_REQUIRE_SIGNED_APP_UPDATE 1
#endif

// <q> NRF_DFU_SINGLE_BANK_APP_UPDATES  - Place the application and the SoftDevice directly where they are supposed to be.
 

// <i> Note that this creates security concerns when signing and  version checks
// <i> are enabled. An attacker will be able to delete (but not replace)
// <i> the current app or SoftDevice without knowing the signature key.

#ifndef NRF_DFU_SINGLE_BANK_APP_UPDATES
#define NRF_DFU_SINGLE_BANK_APP_UPDATES 0
#endif

// </h> 
//==========================================================

// <q> NRF_DFU_SETTINGS_COMPATIBILITY_MODE  - nrf_dfu_settings - DFU Settings
 

#ifndef NRF_DFU_SETTINGS_COMPATIBILITY_MODE
#define NRF_DFU_SETTINGS_COMPATIBILITY_MODE 1
#endif

// <h> nrf_dfu - Device Firmware Upgrade

//==========================================================
// <h> DFU transport 

//==========================================================
// <e> NRF_DFU_TRANSPORT_ANT - ANT transport settings
//==========================================================
#ifndef NRF_DFU_TRANSPORT_ANT
#define NRF_DFU_TRANSPORT_ANT 0
#endif
// <o> NRF_DFU_ANT_MTU - MTU size used for firmware bursts. 
// <i> Sets the maximum burst size used for DFU write commands.

#ifndef NRF_DFU_ANT_MTU
#define NRF_DFU_ANT_MTU 1024
#endif

// <h> ANT DFU buffers 

//==========================================================
// <e> NRF_DFU_ANT_BUFFERS_OVERRIDE 

// <i> Check this option to override the default number of buffers.
//==========================================================
#ifndef NRF_DFU_ANT_BUFFERS_OVERRIDE
#define NRF_DFU_ANT_BUFFERS_OVERRIDE 0
#endif
// <o> NRF_DFU_ANT_BUFFERS - Number of buffers in the ANT transport. 
// <i> Number of buffers to store incoming data while it is being written to flash.
// <i> Reduce this value to save RAM. If this value is too low, the DFU process will fail.

#ifndef NRF_DFU_ANT_BUFFERS
#define NRF_DFU_ANT_BUFFERS 8
#endif

// </e>

// </h> 
//==========================================================

// <h> ANT DFU Channel Configuration 

//==========================================================
// <o> NRF_DFU_ANT_RF_FREQ - DFU RF channel. 
#ifndef NRF_DFU_ANT_RF_FREQ
#define NRF_DFU_ANT_RF_FREQ 66
#endif

// <o> NRF_DFU_ANT_DEV_TYPE - Device type field to use for DFU channel id. 
#ifndef NRF_DFU_ANT_DEV_TYPE
#define NRF_DFU_ANT_DEV_TYPE 10
#endif

// <o> NRF_DFU_ANT_CHANNEL_PERIOD - Channel period of DFU ANT channel. 
#ifndef NRF_DFU_ANT_CHANNEL_PERIOD
#define NRF_DFU_ANT_CHANNEL_PERIOD 2048
#endif

// </h> 
//==========================================================

// </e>

// <e> NRF_DFU_TRANSPORT_BLE - BLE transport settings
//==========================================================
#ifndef NRF_DFU_TRANSPORT_BLE
#define NRF_DFU_TRANSPORT_BLE 1
#endif
// <q> NRF_DFU_BLE_SKIP_SD_INIT  - Skip the SoftDevice and interrupt vector table initialization.
 

#ifndef NRF_DFU_BLE_SKIP_SD_INIT
#define NRF_DFU_BLE_SKIP_SD_INIT 0
#endif

// <s> NRF_DFU_BLE_ADV_NAME - Default advertising name.
#ifndef NRF_DFU_BLE_ADV_NAME
#define NRF_DFU_BLE_ADV_NAME "DfuTarg"
#endif

// <o> NRF_DFU_BLE_ADV_INTERVAL - Advertising interval (in units of 0.625 ms) 
#ifndef NRF_DFU_BLE_ADV_INTERVAL
#define NRF_DFU_BLE_ADV_INTERVAL 40
#endif

// <h> BLE DFU security 

//==========================================================
// <q> NRF_DFU_BLE_REQUIRES_BONDS  - Require bond with peer.
 

#ifndef NRF_DFU_BLE_REQUIRES_BONDS
#define NRF_DFU_BLE_REQUIRES_BONDS 0

#endif

// </h> 
//==========================================================

// <h> BLE DFU connection 

//==========================================================
// <o> NRF_DFU_BLE_MIN_CONN_INTERVAL - Minimum connection interval (units). 
// <i> Minimum GAP connection interval, in 1.25 ms units.

#ifndef NRF_DFU_BLE_MIN_CONN_INTERVAL
#define NRF_DFU_BLE_MIN_CONN_INTERVAL 12
#endif

// <o> NRF_DFU_BLE_MAX_CONN_INTERVAL - Maximum connection interval (units). 
// <i> Maximum GAP connection interval, in 1.25 ms units.

#ifndef NRF_DFU_BLE_MAX_CONN_INTERVAL
#define NRF_DFU_BLE_MAX_CONN_INTERVAL 12
#endif

// <o> NRF_DFU_BLE_CONN_SUP_TIMEOUT_MS - Supervision timeout (ms). 
// <i> GAP connection supervision timeout, in milliseconds.

#ifndef NRF_DFU_BLE_CONN_SUP_TIMEOUT_MS
#define NRF_DFU_BLE_CONN_SUP_TIMEOUT_MS 6000
#endif

// </h> 
//==========================================================

// <h> BLE DFU buffers 

//==========================================================
// <e> NRF_DFU_BLE_BUFFERS_OVERRIDE 

// <i> Check this option to override the default number of buffers.
//==========================================================
#ifndef NRF_DFU_BLE_BUFFERS_OVERRIDE
#define NRF_DFU_BLE_BUFFERS_OVERRIDE 0
#endif
// <o> NRF_DFU_BLE_BUFFERS - Number of buffers in the BLE transport. 
// <i> Number of buffers to store incoming data while it is being written to flash.
// <i> Reduce this value to save RAM. If this value is too low, the DFU process will fail.

#ifndef NRF_DFU_BLE_BUFFERS
#define NRF_DFU_BLE_BUFFERS 8
#endif

// </e>

// </h> 
//==========================================================

// </e>

// </h> 
//==========================================================

// <h> DFU protocol 

//==========================================================
// <q> NRF_DFU_PROTOCOL_FW_VERSION_MSG  - Firmware version message support.
 

// <i> Firmware version message support.
// <i> If disabled, firmware version requests will return NRF_DFU_RES_CODE_OP_CODE_NOT_SUPPORTED.

#ifndef NRF_DFU_PROTOCOL_FW_VERSION_MSG
#define NRF_DFU_PROTOCOL_FW_VERSION_MSG 1
#endif

// <q> NRF_DFU_PROTOCOL_REDUCED  - Reduced protocol opcode selection.
 

// <i> Only support a minimal set of opcodes; return NRF_DFU_RES_CODE_OP_CODE_NOT_SUPPORTED 
// <i> for unsupported opcodes. The supported opcodes are:NRF_DFU_OP_OBJECT_CREATE, 
// <i> NRF_DFU_OP_OBJECT_EXECUTE, NRF_DFU_OP_OBJECT_SELECT, NRF_DFU_OP_OBJECT_WRITE, 
// <i> NRF_DFU_OP_CRC_GET, NRF_DFU_OP_RECEIPT_NOTIF_SET, and NRF_DFU_OP_ABORT. 
// <i> This reduced feature set is used by the BLE transport to reduce flash usage.

#ifndef NRF_DFU_PROTOCOL_REDUCED
#define NRF_DFU_PROTOCOL_REDUCED 1
#endif

// <q> NRF_DFU_PROTOCOL_VERSION_MSG  - Protocol version message support.
 

// <i> Protocol version message support.
// <i> If disabled, protocol version requests will return NRF_DFU_RES_CODE_OP_CODE_NOT_SUPPORTED.

#ifndef NRF_DFU_PROTOCOL_VERSION_MSG
#define NRF_DFU_PROTOCOL_VERSION_MSG 1
#endif

// </h> 
//==========================================================

// <h> Misc DFU settings 

//==========================================================
// <o> NRF_DFU_APP_DATA_AREA_SIZE - The size (in bytes) of the flash area reserved for application data. 
// <i> This area is found at the end of the application area, next to the start of
// <i> the bootloader. This area will not be erased by the bootloader during a
// <i> firmware upgrade. The size must be a multiple of the flash page size.

#ifndef NRF_DFU_APP_DATA_AREA_SIZE
#define NRF_DFU_APP_DATA_AREA_SIZE 12288
#endif

// <q> NRF_DFU_IN_APP  - Specifies that this code is in the app, not the bootloader, so some settings are off-limits.
 

// <i> Enable this to disable writing to areas of the settings that are protected
// <i> by the bootlader. If this is not enabled in the app, certain settings write
// <i> operations will cause HardFaults or will be ignored. Enabling this option
// <i> also causes postvalidation to be disabled since this is meant to be done
// <i> in the bootloader. NRF_BL_DFU_ALLOW_UPDATE_FROM_APP must be enabled in the bootloader.

#ifndef NRF_DFU_IN_APP
#define NRF_DFU_IN_APP 1
#endif

// <q> NRF_DFU_SAVE_PROGRESS_IN_FLASH  - Save DFU progress in flash.
 

// <i> Save DFU progress to flash so that it can be resumed if interrupted, instead of being restarted.
// <i> Keep this setting disabled to maximize transfer speed and minimize flash wear.
// <i> The init packet is always saved in flash, regardless of this setting.

#ifndef NRF_DFU_SAVE_PROGRESS_IN_FLASH
#define NRF_DFU_SAVE_PROGRESS_IN_FLASH 0
#endif

// <q> NRF_DFU_SUPPORTS_EXTERNAL_APP  - [Experimental] Support for external app.
 

// <i> External apps are apps that will not be activated. They can 
// <i> e.g. be apps to be sent to a third party. External app updates 
// <i> are verified upon reception, but will remain in bank 1, and 
// <i> will never be booted. An external app will be overwritten if 
// <i> a new DFU procedure is performed. Note: This functionality is 
// <i> experimental and not yet used in any examples.

#ifndef NRF_DFU_SUPPORTS_EXTERNAL_APP
#define NRF_DFU_SUPPORTS_EXTERNAL_APP 0
#endif

// </h> 
//==========================================================

// </h> 
//==========================================================

// </h> 
//==========================================================

// <<< end of configuration section >>>
#endif //SDK_CONFIG_H

