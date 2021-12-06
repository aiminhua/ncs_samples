.. ble_netcore:

NUS and OTA BLE services on nRF5340 netcore
###########################################

.. contents::
   :local:
   :depth: 2

This sample works with ``nrf53_ble/appcore``, ``nrf_dfu/ap_inFlash_rpc`` and ``nrf_dfu/ap_exFlash_rpc``. It serves as a child image of other samples. That is, it would
be built automatically by the system without your awareness. In addition, you can compile this sample separately too once you disable some default configurations.

Overview
********

This sample has NUS and OTA services. There are two kinds of OTA services: SMP and nrf_dfu. SMP is the out-of-box module of NCS, but its OTA speed is as low as 2kB/s. 
nrf_dfu is ported from nRF5 SDK v17.0.2. Its OTA speed can hit 17kB/s or more.   

Build
*****

To work with other parent projects automatically, make sure nrf53_ble/ble_netcore is put in the following folder.

::

    NCS root folder
    ├── nrf
    ├── zephyr
    ├── sample          
    │   ├── nrf53_ble
    │       └── ble_netcore

By default, this sample works with the latest NCS tag. To work with other versions of NCS, read **prj.conf** carefully. Open the configurations relating to the specified version
and close the configurations of other versions. Search **NCS** in **prj.conf** to locate the configurations quickly.
	
This example need to modify the original NCS source code. Refer to ``sdk_change`` for the detailed changes. For example, to work with NCS v1.7.1, enter folder ``sdk_change/ncs_v1.7.x`` 
and overwrite the same files in the correspondent NCS ``v1.7.1`` folders.

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

Refer to ``nrf53_ble/appcore``, ``nrf_dfu/ap_inFlash_rpc`` and ``nrf_dfu/ap_exFlash_rpc`` for the testing steps.