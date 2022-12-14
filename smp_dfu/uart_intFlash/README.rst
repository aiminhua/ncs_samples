.. uart_smp_dfu_inFlash:

UART SMP DFU example(internal secondary slot)
############################################

.. contents::
   :local:
   :depth: 2

Use NCS native DFU module: smp to update firmware via Bluetooth LE connection.

Overview
********

In this sample, the secondary slot is on the internal Flash of nRF5 SoC. That is, the new image will be stored in the secondary slot first. After that, MCUBoot would perform
the swap operation to finish the whole DFU process. Regarding ``nRF5340``, the BLE host is running on the application core in this sample(``zephyr/samples/bluetooth/hci_rpmsg`` runs on the netcore). 

**note: In this sample, MCUBoot uses the default signing key, which must be replaced with your own key before production.** Do it like below:

.. code-block:: console

	CONFIG_BOOT_SIGNATURE_KEY_FILE="my_mcuboot_private.pem"	
	
Build & Programming
*******************

The following NCS tags are tested for this sample. By default, NCS ``v2.2.0`` is used.

+------------------------------------------------------------------+
|NCS tags                                                          +
+==================================================================+
|v1.5.x/v1.6.x/v1.7.x/v1.8.x/v1.9.x/v2.0.x/v2.2.x                  |
+------------------------------------------------------------------+

Use ``git tag`` to see supported tags. For ncs versions later than v1.9.0, for example ncs v2.0.0, 
use ``git checkout v2.0`` to switch to the specified NCS tag. Use ``git checkout v1.5_v1.9`` to switch to 
ncs versions earlier than v1.9.0. After the checkout operation, open this README.rst again and follow 
the instructions. 
	
This example **may** modify the original NCS source code. Refer to ``sdk_change`` for the detailed changes. 
For example, to work with NCS v1.9.1, copy folder ``sdk_change/ncs_v1.9.x`` and overwrite the same files 
in the correspondent NCS ``v1.9.1`` folders.

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
#. Copy ``build*/zephyr/app_update.bin`` to folder: ``update``. (If you want to update the net core image, use **net_core_app_update.bin** instead)
#. Open ``CMD`` and change the active directory to folder:  ``update``. 
#. Enter the following commands to finish DFU process

.. code-block:: console

   mcumgr conn add myCOM type="serial" connstring="dev=COM13,baud=115200,mtu=256"     (Note: change the COM if needed)
   mcumgr -c myCOM image upload app_update.bin
   mcumgr -c myCOM image list
   mcumgr -c myCOM image test <hash of slot-1 image>
   mcumgr -c myCOM reset
   mcumgr -c myCOM image confirm
   mcumgr -c myCOM reset                  (Note: you will see new image running logs now)
   
Note: see https://docs.zephyrproject.org/latest/guides/device_mgmt/mcumgr.html for a detailed description of mcumgr commands.