.. nrf_dfu_uart_intFlash:

uart nrf_dfu example(internal secondary slot)
#############################################

.. contents::
   :local:
   :depth: 2

Use DFU module from nRF5 SDK v17.0.2 to do DFU in nRF Connect SDK. This DFU module is called nrf_dfu in this document. The DFU procedure is exactly the
same as that of nRF5 SDK. See https://infocenter.nordicsemi.com/index.jsp?topic=%2Fsdk_nrf5_v17.0.2%2Fble_sdk_app_dfu_bootloader.html
for a detailed description of nRF5 SDK DFU steps if you don't have too much knowledge of it.

Overview
********

In this sample, the secondary slot is on the internal Flash of nRF5 SoC. That is, the new image will be transferred by UART and stored in the secondary slot. 
After that, MCUBoot would perform the swap operation to finish the whole DFU process. 

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
|nrf5340dk_nrf5340_cpuapp/nrf52840dk_nrf52840                      |
+------------------------------------------------------------------+

For example, enter the following command to build ``nrf5340dk_nrf5340_cpuapp``.

.. code-block:: console

   west build -b nrf5340dk_nrf5340_cpuapp -d build_nrf5340dk_nrf5340_cpuapp -p

To flash the images to the board, just double click ``program.bat``, or use the following command:

.. code-block:: console

   west flash -d build_nrf5340dk_nrf5340_cpuapp     

Testing
*******

After programming the sample to your development kit, test it by performing the following steps:

1. Connect the kit to the computer using a USB cable. The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. Reset the kit. It shall advertise ``Nordic_DFU``
#. Copy app_signed.hex in folder ``build*/zephyr`` (and net_core_app_signed.hex if you want to upgrade network core of nRF5340) to folder ``update``.
#. Double click ``upload_app.bat`` (change the COM port if needed) in folder ``update``. (If you want to update nRF5340 network core, edit ``upload_app.bat`` to uncomment the last 3 lines)
#. Waiting until the transfer is done. 
#. DFU is completed. 
