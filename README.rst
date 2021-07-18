.. _ncs_sample:

nRF Connect SDK Samples
#######################

Overview
********

This repo contains many nRF Connect SDK(NCS) samples. All of them are desgined for test purposes.

This repo will follow nRF Connect SDK latest tag. That is, all samples should work out of box with the latest tag. In addition, it should work with other versions of NCS. 

Refer to folder ``sdk_change`` for all tested NCS tags. ``sdk_change`` list all the changes of the related NCS tag to make the samples run on the specified NCS tag. 
You can check the diff file for a quick overview of the changes. To make the samples build and run successfully, you can just overwrite the same files in the correspondent NCS folders. 

Most samples can be put anywhere under NCS root folder. However, to build **ble_netcore** automatically, you must rename the repo's local folder name as ``sample``. 
And put it at the same folder level as ``nrf`` or ``zephyr`` folder just like below.

::

    NCS root folder
    ├── nrf
    ├── zephyr
    ├── sample          
    │   ├── nrf53_ble
    │       └── ble_netcore
    |       └── appcore 


Thus, all the description documents would assume that you put this repo under folder **sample**.

A overview of the samples in this repo is shown below.

+---------------------------------------+-------------------------------------------------------------------------------------------+
|Sample                                 |Brief                                                                                      +
+=======================================+===========================================================================================+
|sample/nrf53_ble/appcore               |a comprehensive example containing rpc, dfu, spi, uart, etc. Work with ble_netcore         |
+---------------------------------------+-------------------------------------------------------------------------------------------+
|sample/nrf53_ble/ble_netcore           |the whole ble stack and services running on the netcore                                    |
+---------------------------------------+-------------------------------------------------------------------------------------------+
|sample/nrf_dfu/inFlash                 |nrf dfu example with secondary slot on internal Flash                                      |
+---------------------------------------+-------------------------------------------------------------------------------------------+
|sample/nrf_dfu/exFlash                 |nrf dfu example with secondary slot on external Flash                                      |
+---------------------------------------+-------------------------------------------------------------------------------------------+
|sample/nrf_dfu/ap_inFlash_rpc          |5340 appcore nrf dfu example with secondary slot on internal Flash. Work with ble_netcore  |
+---------------------------------------+-------------------------------------------------------------------------------------------+
|sample/nrf_dfu/ap_exFlash_rpc          |5340 appcore nrf dfu example with secondary slot on external Flash. Work with ble_netcore  |
+---------------------------------------+-------------------------------------------------------------------------------------------+

**note: see the README.rst in the individual sample folder for a detailed description of the sample**

Requirements
************

The following development kits are tested. In principle, other nRF52 SoC should work too if nRF52840 work.

+---------------------------------------+------------------------------------------------------------------+
|Sample                                 |Build target                                                      +
+=======================================+==================================================================+
|sample/nrf53_ble/appcore               |nrf5340dk_nrf5340_cpuapp                                          |
+---------------------------------------+------------------------------------------------------------------+
|sample/nrf53_ble/ble_netcore           |nrf5340dk_nrf5340_cpunet                                          |
+---------------------------------------+------------------------------------------------------------------+
|sample/nrf_dfu/inFlash                 |nrf5340dk_nrf5340_cpuapp/nrf52840dk_nrf52840                      |
+---------------------------------------+------------------------------------------------------------------+
|sample/nrf_dfu/exFlash                 |nrf5340dk_nrf5340_cpuapp/nrf52840dk_nrf52840                      |
+---------------------------------------+------------------------------------------------------------------+
|sample/nrf_dfu/ap_inFlash_rpc          |nrf5340dk_nrf5340_cpuapp                                          |
+---------------------------------------+------------------------------------------------------------------+
|sample/nrf_dfu/ap_exFlash_rpc          |nrf5340dk_nrf5340_cpuapp                                          |
+---------------------------------------+------------------------------------------------------------------+