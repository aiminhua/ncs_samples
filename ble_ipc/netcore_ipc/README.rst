.. ble_ipc_netcore:

NUS and IPC on nRF5340 netcore
###############################

.. contents::
   :local:
   :depth: 2

Overview
********

This sample works with ``ncs_samples\ble_ipc\appcore``. It serves as a child image of the sample. 
That is, it would be built automatically by the system when you build ``ncs_samples\ble_ipc\appcore``.

Build
*****

To work with the parent projects automatically, make sure this project is put in the following folder.

::

    NCS root folder
    ├── nrf
    ├── zephyr
    ├── ncs_samples          
        ├── ble_ipc
             ├──netcore


The following development kits are tested for this sample.

+------------------------------------------------------------------+
|Build target                                                      +
+==================================================================+
|nrf5340dk_nrf5340_cpunet                                          |
+------------------------------------------------------------------+

The following NCS tags are tested for this sample. By default, NCS ``v1.9.1`` is used.

+------------------------------------------------------------------+
|NCS tags                                                          +
+==================================================================+
|v1.9.x                                                            |
+------------------------------------------------------------------+

For example, enter the following command to build ``nrf5340dk_nrf5340_cpunet``.

.. code-block:: console

   west build -b nrf5340dk_nrf5340_cpunet -d build_nrf5340dk_nrf5340_cpunet -p
   

Testing
*******

Refer to ``ncs_samples\ble_ipc\appcore`` for the testing steps.