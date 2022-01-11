.. netcore_ble:

NUS and OTA BLE services on nRF5340 netcore
###########################################

.. contents::
   :local:
   :depth: 2

This sample works with ``ncs_samples/comprehensive_rpc``, ``ncs_samples/nrf_dfu/ble_extFlash_rpc`` and ``ncs_samples/nrf_dfu/ble_intFlash_rpc``.
It serves as a child image of those samples. That is, it would be built automatically by the system when you build those samples.

Overview
********

This sample has NUS and OTA services. There are two kinds of OTA services: SMP and nrf_dfu. SMP is the out-of-box module of NCS. 
nrf_dfu is ported from nRF5 SDK v17.0.2.  

Build
*****

To work with the parent projects automatically, make sure this project is put in the following folder.

::

    NCS root folder
    ├── nrf
    ├── zephyr
    ├── ncs_samples          
    │   ├── netcore_ble


The following NCS tags are tested for this sample. By default, NCS ``v1.8.0`` is used.

+------------------------------------------------------------------+
|NCS tags                                                          +
+==================================================================+
|v1.5.x/v1.6.x/v1.7.x/v1.8.x                                       |
+------------------------------------------------------------------+

To work with a specified NCS tag, read **prj.conf** carefully. Open the configurations relating to the specified version
and close the configurations of other versions. Search **NCS** in **prj.conf** to locate the configurations quickly.

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

Refer to ``ncs_samples/comprehensive_rpc``, ``ncs_samples/nrf_dfu/ble_extFlash_rpc`` and ``ncs_samples/nrf_dfu/ble_intFlash_rpc`` for the testing steps.