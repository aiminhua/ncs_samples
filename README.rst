.. _ncs_samples:

nRF Connect SDK Samples
#######################

Overview
********

This repo contains many nRF Connect SDK(NCS) samples. All of them are desgined for test purposes.

This repo will follow nRF Connect SDK latest tag. That is, all samples should work out of box with the latest tag. In addition, it should work with other versions of NCS. 

Refer to folder ``sdk_change`` for all tested NCS tags. ``sdk_change`` list all the changes of the related NCS tag to make the samples run on the specified NCS tag. 
To make the samples build and run successfully, you can just overwrite the same files in the correspondent NCS folders. 

A overview of the samples in this repo is shown below.

+---------------------------------------+-------------------------------------------------------------------------------------------+
|Sample                                 |Brief                                                                                      +
+=======================================+===========================================================================================+
|comprehensive_rpc                      |a comprehensive example containing rpc, dfu, spi, uart, etc. Work with ``netcore_ble``     |
+---------------------------------------+-------------------------------------------------------------------------------------------+
|netcore_ble                            |the whole ble stack and services run on the netcore                                        |
+---------------------------------------+-------------------------------------------------------------------------------------------+
|nrf_dfu                                |nrf dfu examples with different bootloaders, transport and secondary slot locations        |
+---------------------------------------+-------------------------------------------------------------------------------------------+
|smp_dfu                                |smp dfu examples with different bootloaders, transport and secondary slot locations        |
+---------------------------------------+-------------------------------------------------------------------------------------------+
|spi_master                             |SPI master example                                                                         |
+---------------------------------------+-------------------------------------------------------------------------------------------+
|i2c_master                             |I2C master example                                                                         |
+---------------------------------------+-------------------------------------------------------------------------------------------+
|uart_highspeed                         |high speed UART example                                                                    |
+---------------------------------------+-------------------------------------------------------------------------------------------+

**note: see the README.rst in the individual sample folder for a detailed description of the samples**.
