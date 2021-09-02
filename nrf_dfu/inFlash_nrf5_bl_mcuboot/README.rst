.. inFlash_nrf5_bl_mcuboot:

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
#. Download MBR and nRF5 SDK bootloader from nRF5 SDK v17.x.x
#. Copy ``build_*/mcuboot/zephyr/zephyr.hex`` to ``update_zip``. Double click ``dfu.bat`` to upload MCUBoot image to the device 
#. Reset the kit while pressing ``Button2``, which would enter serial recovery mode of MCUBoot
#. Open CMD and enter the following commands to finish serial DFU of MCUBoot

.. code-block:: console

   mcumgr conn add myCOM type="serial" connstring="dev=COM28,baud=115200,mtu=256"
   mcumgr -c myCOM image upload app_update.bin
   mcumgr -c myCOM -t 20 reset
   
#. Reset the kit. It shall advertise ``Nordic_DFU``
#. Enter ``zephyr folder`` of the ``build`` folder. Copy app_signed.hex to folder ``update_zip``. Double click ``zip_generate.bat``.
#. Copy app_nrf5_bl.zip to the phone
#. Perform the DFU steps as nRF5 SDK do
