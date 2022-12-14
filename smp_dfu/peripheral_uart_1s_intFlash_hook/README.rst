.. _peripheral_uart_1s_intFlash_hook:

Peripheral UART + BLE SMP
##########################

.. contents::
   :local:
   :depth: 2

This sample integrates peripheral_uart and ble smp. It has only 1 secondary slot to hold both app core image and net core image.
The secondary slot is on the internal Flash. You can update application core image and network core image separately.

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
#. Enter folder: ``build*/zephyr``. Copy ``app_update.bin`` or ``net_core_app_update.bin`` to your phone for your needs. (Change some code lines before a second build)
#. Tap **DFU** button on the right top corner of nRF Connect.
#. Select ``app_update.bin`` or ``net_core_app_update.bin`` on your phone.
#. Wait until the DFU process is done.
