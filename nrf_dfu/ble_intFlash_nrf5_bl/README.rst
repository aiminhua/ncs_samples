.. ble_intFlash_nrf5_bl:

nrf_dfu and nrf5 bootloader dfu example
#######################################

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

If you intend to build this sample on other nRF52 SoC, the following configs should be modified. 

*  overlay file in ``boards`` folder. Make sure that the end of storage_partition is the starting address of the nRF5 bootloader.
*  ``CONFIG_FLASH_LOAD_SIZE`` in ``prj.conf``

Testing
*******

1. Connect the kit to the computer using a USB cable. 
#. Compile nRF5 bootloader from nRF5 SDK v17.x.x
#. Build this sample
#. Copy ``zephyr.hex`` and ``nRF5 Bootloader`` to ``update``. Double click ``program.bat`` to program the images to the kit
#. Reset the kit. It shall advertise ``Nordic_DFU``
#. Build this sample again with some modification. Rename ``zephyr.hex`` to ``app_new.hex``. Copy it to ``update``. 
   Double click ``zip_generate.bat``. Upload ``ncs_nrf5_bl.zip'' to your testing mobile phone. 
#. Perform the DFU steps as nRF5 SDK do
