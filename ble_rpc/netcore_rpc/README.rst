.. ble_netcore_rpc:

NUS, SMP and RPC on nRF5340 netcore
###################################

.. contents::
   :local:
   :depth: 2

Overview
********

This sample works with ``comprehensive_rpc``, ``nrf_dfu_extFlash_rpc`` and ``nrf_dfu_intFlash_rpc``. It serves as a 
child image of those samples. That is, it would be built automatically by the system when you build those samples.

Build
*****

To work with the parent projects automatically, make sure this project is put in the following folder.

::

    NCS root folder
    ├── nrf
    ├── zephyr
    ├── ncs_samples          
    │   ├── ble_rpc
	│      ├── netcore_rpc 


The following NCS tags are tested for this sample. By default, NCS ``v2.0.0`` is used.

+------------------------------------------------------------------+
|NCS tags                                                          +
+==================================================================+
|v1.5.x/v1.6.x/v1.7.x/v1.8.x/v1.9.x/v2.0.x                         |
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

Refer to ``comprehensive_rpc``, ``nrf_dfu_extFlash_rpc`` and ``nrf_dfu_intFlash_rpc`` for the testing steps.