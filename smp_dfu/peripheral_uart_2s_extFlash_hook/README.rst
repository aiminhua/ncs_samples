.. _peripheral_uart_2s_extFlash:

Peripheral UART + BLE SMP
##########################

.. contents::
   :local:
   :depth: 2

This sample integrates peripheral_uart and ble smp. It has 2 secondary slots for app core image and net core image.
The secondary slots are on the external Flash. The mcuboot_primary_1 is only intended for label use.
You can update application core and network core images in one go, or update them separately.

This example **may** modify the original NCS source code. Refer to ``sdk_change`` for the detailed changes. 
For example, to work with NCS v2.0.0, copy folder ``sdk_change/ncs_v2.0.x`` and overwrite the same files 
in the correspondent NCS ``v2.2.0`` folders.

The following development kits are tested for this sample. 

+------------------------------------------------------------------+
|Build target                                                      +
+==================================================================+
|nrf5340dk_nrf5340_cpuapp                                          |
+------------------------------------------------------------------+

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. Connect the kit to the computer using a USB cable. The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. Open both serial terminal and RTT viewer.
#. Open nRF Connect or Device Manager on your phone. 
#. Connect the board. 
#. Enter folder: ``build*/zephyr``. Copy ``dfu_application.zip``, ``app_update.bin`` or ``net_core_app_update.bin`` to your phone for your needs. (Change some code lines before a second build)
#. Tap **DFU** button on the right top corner of nRF Connect.
#. Select ``dfu_application.zip``, ``app_update.bin`` or ``net_core_app_update.bin`` on your phone.
#. Wait until the DFU process is done.
