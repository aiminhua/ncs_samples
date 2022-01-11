.. smp_dfu_exFlash:

smp_dfu OTA example(external secondary slot)
############################################

.. contents::
   :local:
   :depth: 2

Use NCS native DFU module: smp to update firmware via Bluetooth LE connection.

Overview
********

In this sample, the secondary slot is on the external Flash. That is, the new image will be stored in the secondary slot first. After that, MCUBoot would perform
the swap operation to finish the whole DFU process. Regarding ``nRF5340``, the BLE host is running on the application core in this sample(``zephyr/samples/bluetooth/hci_rpmsg`` runs on the netcore). 

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
#. |connect_terminal|
#. Copy ``build*/zephyr/app_update.bin`` to your mobile phone. (If you want to update the net core image, use **net_core_app_update.bin** instead)
#. Open nRF connect for Mobile on your phone. 
#. Connect the board. 
#. Tap **DFU** button on the right top corner of the mobile app.
#. Select **app_update.bin** in your phone. (If you want to update the net core image, use **net_core_app_update.bin** instead)
#. Complete the DFU process.
