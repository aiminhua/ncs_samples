.. ble_intFlash_nrf5_bl_mcuboot:

nrf_dfu and nrf5 bootloader/MCUBoot dfu example
###############################################

.. contents::
   :local:
   :depth: 2

Use Bootloader and DFU module from nRF5 SDK v17.0.2 to do OTA in nRF Connect SDK. The bootloader is called nRF5 Bootloader. 
The DFU module is called nrf_dfu in this document. The OTA procedure is exactly the same as that of nRF5 SDK. 
See https://infocenter.nordicsemi.com/index.jsp?topic=%2Fsdk_nrf5_v17.0.2%2Fble_sdk_app_dfu_bootloader.html
for a detailed description of nRF5 SDK DFU steps if you don't have too much knowledge of it.

Overview
********

To run this sample, you need to compile nRF5 bootloader in ''nRF5_SDK_17.0.2_d674dde/examples/dfu''. To make nRF5 bootloader work with
this sample, you need to do the following changes.

* #define NRF_BL_DFU_ALLOW_UPDATE_FROM_APP 1 in sdk_config.h
* comment all nrf_bootloader_flash_protect invocation to avoid flash access blocking in app
* in this sample, we take ''nRF5_SDK_17.0.2_d674dde/examples/dfu/secure_bootloader/pca10056_uart_debug'' as the example bootloader.

**Note: you must replace priv.pem with your own keys before production.**

In this sample, the secondary slot of MCUBoot is on the internal Flash of nRF5 SoC. That is, the new image will be stored in the secondary slot first. After that, MCUBoot would perform
the swap operation to finish the whole DFU process.

**note: In this sample, MCUBoot uses the default signing key, which must be replaced with your own key before production.** Do it like below:

.. code-block:: console

	CONFIG_BOOT_SIGNATURE_KEY_FILE="my_mcuboot_private.pem"	

Build & Programming
*******************

The following NCS tags are tested for this sample. By default, NCS ``v1.8.0`` is used.

+------------------------------------------------------------------+
|NCS tags                                                          +
+==================================================================+
|v1.5.x/v1.6.x/v1.7.x/v1.8.x                                       |
+------------------------------------------------------------------+

To work with a specified NCS tag, read **prj.conf** carefully. Open the configurations relating to the specified version
and close the configurations of other versions. Search **NCS** in **prj.conf** to locate the configurations quickly.
	
This example may modify the original NCS source code. Refer to ``sdk_change`` for the detailed changes. For example, to work with NCS v1.7.1, 
enter folder ``sdk_change/ncs_v1.7.x`` and overwrite the same files in the correspondent NCS ``v1.7.1`` folders. 

The following development kits are tested for this sample. However, other nRF52 SoC should work too.

+------------------------------------------------------------------+
|Build target                                                      +
+==================================================================+
|nrf52840dk_nrf52840                                               |
+------------------------------------------------------------------+

For example, enter the following command to build ``nrf52840dk_nrf52840``.

.. code-block:: console

   west build -b nrf52840dk_nrf52840 -d build_nrf52840dk_nrf52840 -p

To flash the images to the board, just double click ``program.bat``, or use the following command:

.. code-block:: console

   west flash -d build_nrf52840dk_nrf52840     


Testing
*******

After programming the sample to your development kit, test it by performing the following steps:

1. Connect the kit to the computer using a USB cable. The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. Download MBR and nRF5 SDK bootloader from nRF5 SDK v17.x.x to the device. You can do it directly by double clicking ``program.bat`` in folder ``update``(Change the bootloader hex if needed).
#. Copy ``build_*/**mcuboot**/zephyr/zephyr.hex`` to folder: ``update``. Double click ``upload_mcuboot.bat``(Change COM number if needed) to upload MCUBoot image to the device. 
#. Reset the kit while pressing ``Button2``, which would enter serial recovery mode of MCUBoot.
#. Copy ``build*/**zephyr**/app_update.bin`` to folder: ``update``.
#. Open ``CMD`` and change the active directory to folder:  ``update``. Enter the following commands to finish serial DFU of MCUBoot.

.. code-block:: console

   mcumgr conn add myCOM type="serial" connstring="dev=COM4,baud=115200,mtu=256"     (Note: change the COM if needed)
   mcumgr -c myCOM image upload app_update.bin
   mcumgr -c myCOM reset
   
#. Reset the kit. It shall advertise ``Nordic_DFU``
#. Copy ``build*/**zephyr**/app_signed.hex`` to folder ``update``. Double click ``app_zip_generate.bat`` to generate app_new.zip.
#. Copy app_new.zip to the phone
#. Perform the DFU steps as nRF5 SDK do.
