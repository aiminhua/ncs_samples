.. _ncs_samples:

nRF Connect SDK Samples
#######################

Overview
********

This repo contains many nRF Connect SDK(NCS) samples. All of them are designed for test purposes.

The folder ``sdk_change`` in each sample would list all the changes of the related NCS tag to make the samples run on the specified NCS tag. 
To make the samples build and run successfully, you can just overwrite the same files in the correspondent NCS folders. 

A overview of the samples in this repo is shown below.

+---------------------------------------+-------------------------------------------------------------------------------------------+
|Sample                                 |Brief                                                                                      +
+=======================================+===========================================================================================+
|comprehensive                          |a comprehensive example containing nus, dfu, spi, uart, etc.                               |
+---------------------------------------+-------------------------------------------------------------------------------------------+
|comprehensive_rpc                      |a comprehensive example containing rpc, nus, dfu, spi, uart, etc. Work with ``netcore_ble``|
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
|nrfx_api                               |demo the direct invocation of nrfx api                                                                        |
+---------------------------------------+-------------------------------------------------------------------------------------------+

**Note: see the README.rst in the individual sample folder for a detailed description of the samples**.
