.. ble_netcore_ipc:

NUS, SMP and IPC on nRF5340 netcore
###################################

.. contents::
   :local:
   :depth: 2

Overview
********

This sample works with ``comprehensive_ipc`` and ``uart_nrf_dfu``. It serves as a child image of the samples. 
That is, it would be built automatically by the system when you build the 2 samples.

Build
*****

To work with the parent projects automatically, make sure this project is put in the following folder.

::

    NCS root folder
    ├── nrf
    ├── zephyr
    ├── ncs_samples          
    │   ├── ble_ipc
	│      ├── netcore_ipc 


The following NCS tags are tested for this sample. By default, NCS ``v2.2.0`` is used.

+------------------------------------------------------------------+
|NCS tags                                                          +
+==================================================================+
|v1.9.x/v2.0.x/v2.2.x                                              |
+------------------------------------------------------------------+

The following development kits are tested for this sample.

+------------------------------------------------------------------+
|Build target                                                      +
+==================================================================+
|nrf5340dk_nrf5340_cpunet                                          |
+------------------------------------------------------------------+

For example, enter the following command to build ``nrf5340dk_nrf5340_cpunet``.

.. code-block:: console

   west build -b nrf5340dk_nrf5340_cpunet -d build_nrf5340dk_nrf5340_cpunet -p
   

Testing
*******

Refer to ``comprehensive_ipc`` and ``uart_nrf_dfu`` for the testing steps.