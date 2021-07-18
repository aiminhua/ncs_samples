.. appcore:

Comprehensive example
#####################

.. contents::
   :local:
   :depth: 2

This comprehensive example integrates most samples under the folder ``sample``. It contains BLE NUS service, BLE OTA service, dual core interactions, high speed UART(1Mbps) example, 
SPI master example, I2C master example, ADC example, external interrupt example, Flash access example, raw nrfx driver access example and device pm example. 

Overview
********

This sample will follow nRF Connect SDK latest tag. That is, it should work out of box with the latest tag. In addition, it should work with other versions of NCS. 

Refer to folder ``sdk_change`` for all tested NCS tags. ``sdk_change`` list all the changes of the related NCS tag to make the sample run on the specified NCS tag. 
You can check the diff file for a quick overview of the changes. To make the sample build and run successfully, you can just overwrite the same files in the correspondent NCS folders. 

By default, this sample works with NCS ``v1.6.0``. To work with other versions of NCS, read **prj.conf** carefully. Open the configurations relating to the specified version
and close the configurations of other versions. Search **NCS** in **prj.conf** to locate the configurations quickly.
	
Before building this sample, enter folder ``sdk_change/ncs_v1.6.0`` and overwrite the same files in the correspondent NCS ``v1.6.0`` folders. If you want to build this sample
in NCS ``v1.5.1`` or ealier. Use folder ``sdk_change/ncs_v1.5.1`` instead. 


Build & Programming
*******************

Make sure nrf53_ble/ble_netcore is put in the following folder.

::

    NCS root folder
    ├── nrf
    ├── zephyr
    ├── sample          
    │   ├── nrf53_ble
    │       └── ble_netcore


The following development kits are tested for this sample. 

+------------------------------------------------------------------+
|Build target                                                      +
+==================================================================+
|nrf5340dk_nrf5340_cpuapp                                          |
+------------------------------------------------------------------+

For example, enter the following command to build ``nrf5340dk_nrf5340_cpuapp``.

.. code-block:: console

   west build -b nrf5340dk_nrf5340_cpuapp -d build_nrf5340dk_nrf5340_cpuapp -p
   
To flash the images to the board, just double click ``program.bat``, or use the following command:

.. code-block:: console

   west flash -d build_nrf5340dk_nrf5340_cpuapp

Testing
*******

After programming the sample to your development kit, you can test following samples directly.

BLE NUS Service
===============

Refer to https://github.com/nrfconnect/sdk-nrf/tree/master/samples/bluetooth/peripheral_uart for the testing steps(Please ignore LED operation).

BLE OTA Service
===============

Refer to https://github.com/aiminhua/ncs_sample/tree/master/nrf_dfu/ap_exFlash_rpc for the testing steps.

Dual core interactions
======================

In fact, when you test NUS service or OTA service, application core and networek core already communicate with each other. 
You can also press **Button1** to let appcore send a message to netcore by ``nrf_rpc``. Then netcore would forward the mesage to mobile app if connected.

SPI master example
==================

This example shows how to call Zephyr SPI APIs to communicate with a SPI slave. The SPI slave image can be directly obtained from nRF5_SDK\examples\peripheral\spis. 
To facilitate the test, we put the spis images at nrf53_ble/resources/hex. The spis pin definitions are shown below.

.. code-block:: console

   APP_SPIS_SCK_PIN 26
   APP_SPIS_MISO_PIN 30
   APP_SPIS_MOSI_PIN 29
   APP_SPIS_CS_PIN 31
   
See your DTS file for SPI master pin definitions.
 
Program the spis hex file to a nRF52832DK or nRF52840DK. Connect the spis related pins to their counterparts in your board.
After pressing **Button2**, this example can start to communicate with spis. The logging looks similar to the following output.

.. code-block:: console

	<inf> spi_thread: Received SPI data:
			4e 6f 72 64 69 63 00

I2C master example
==================

This example shows how to use Zephyr I2C APIs to communicate with a I2C slave. The I2C slave image can be directly obtained from nRF5_SDK\examples\peripheral\twi_master_with_twis_slave.
To facilitate the test, we put the twis images at nrf53_ble/resources/hex. The twis pin definitions are shown below.

.. code-block:: console

	EEPROM_SIM_SCL_S         31   
	EEPROM_SIM_SDA_S         30  
   
See your DTS file for I2C master pin definitions.
 
Program the twis hex file to a nRF52832DK or nRF52840DK. Connect the twis related pins to their counterparts in your board.
After P0.06 is pulled down, this example can start to communicate with twis. The logging looks like below.

.. code-block:: console

	<inf> i2c_thread: EEPROM:
			f8 6f 32 5f e4 21 80 65 e3 a3 4b 3c 8d 91 03 7f
	
ADC example
===========

ADC has 2 working modes: sync and async mode. And it can sample many channels simultaneously. This example samples 2 channels (VDD and P0.05) together, 
and work in both sync and async mode. If you change the voltage on P0.05, you would see a changing ADC value from the log.

.. code-block:: console

	<inf> adc_thread: ADC thread
	<inf> adc_thread: Voltage0: 2988 mV / 3400
	<inf> adc_thread: Voltage1: 259 mV / 295
	<inf> adc_thread: Voltage0: 2988 mV / 3400 async
	<inf> adc_thread: Voltage1: 259 mV / 295 async

External interrupt example
==========================

We have 2 external interrupt examples. One is on application core. The other is on network core. By reading the code, you would find API usage 
on network core is just the same as that of application core. Regarding application core external interrupt example, it's used to trigger I2C communication. 
In terms of network core external interrupt example, you just press **Button4** which would trigger an external interrupt on network core. 
After pressing **Button4**, network would send a message to the mobile app directly without appcore’s awareness. 

The logging of application core external interrupt example is like below.

.. code-block:: console

	<inf> i2c_thread: external interrupt occurs at 676640	

The logging of network core external interrupt example is like below.

.. code-block:: console

	<inf> main: button4 pressed and going to send nus packet	

Flash access example
====================

There are 3 layers(sets) of Flash access APIs in NCS: Flash area API, NVS API and Settings API. The bottom layer is Flash area API which access Flash directly 
without additional headers or tails. NVS API invokes Flash area API to achieve the Flash access purpose. To have a better reliability and readability, 
NVS would add some additional info at the end of a page.  Settings API calls NVS API to access Flash memory. Thus, Settings module has a further encapsulation 
of raw serialized data. All data is managed by key/value pair in Settings module.

In this example, we use both NVS API and Settings API to do the same thing: store a secret and reboot counter onto the internal Flash. The logging is like below.

.. code-block:: console

	<inf> flash_thread: Key value in NVS:
            ff fe fd fc fb fa f9 f8                                
	<inf> flash_thread: *** Reboot counter in NVS: 6 ***
	<inf> flash_thread: *** Reboot counter in Settings: 6 ****
	<inf> flash_thread: Key value in Settings:
            30 31 32 33 34 35 36 37                           

Device PM example
=================

We can use PM to turn on/off peripherals dynamically to save power consumption. 
In this example, press **Button3** to turn on/off UART0/UART1 repeatedly. If the logging backend is UART0, the logging message would be gone after pressing **Button3**.	
The logging is like below.

.. code-block:: console

	<inf> main: button3 isr
	<inf> main: UART0 is in active state. We suspend it
	<inf> main: button3 isr
	<inf> main: UART0 is in suspend state. We activate it
	<inf> main: ## UART0 is active now ##

High speed UART example
=======================

In this example, you can achieve 1Mbps baud rate. UART has 3 working mode: poll, interrupt and asynchronize. To achieve high speed UART, asyn mode must be used.  
To test the reliability of 1Mbps UART, you can transfer a file from PC end to the device end. In this example, when PC sends some data to the device, the device 
would send the same data back to the PC. In this way, you can verify the reliability of 1Mbps UART.

When doing the loopback test of 1Mbps UART, make sure BLE connection is disconnected and RTT logging terminal is closed since they would have a great
impact on the UART communication. You can use ``Serial Debug Assistant`` from Microsoft Store for the test. 

To make 1Mbps UART work, you need to change the default configurations.
 
* Change ``nrf5340dk_nrf5340_cpuapp.overlay`` to set the 1Mbps baud rate

.. code-block:: console

	current-speed = < 1000000 >;
 
* Change ``prj.conf`` to change logging backend to RTT

.. code-block:: console

	CONFIG_LOG_BACKEND_UART=n
	CONFIG_LOG_BACKEND_RTT=y
	CONFIG_USE_SEGGER_RTT=y
	CONFIG_RTT_CONSOLE=y
	CONFIG_UART_CONSOLE=n

Build the project and program it to the board.

You can use ``Serial Debug Assistant`` to send a file to the board. The board would forward the same file back to the PC. Verify whether they are the same.


nrfx driver API direct use example
==================================

Many users want to invoke nrfx drivers API directly so that they can skip Zephyr layers to speed up the access or not to use kconfig or deviceTree to 
have a back compatibility of his old projects. This example shows how to call SPI and RTC bottom layer driver API directly without the awareness of Zephyr system.

* Change ``prj.conf`` before the building process.

.. code-block:: console

	## SPI master example ##
	# CONFIG_EXAMPLE_SPIM=y
	# CONFIG_SPI=y
	# CONFIG_NRFX_SPIM3=y

	## raw nrfx(spim3 & rtc0) driver API usage example ##
	CONFIG_EXAMPLE_RAW_NRFX=y
	CONFIG_NRFX_SPIM3=y
	CONFIG_NRFX_RTC0=y

* Change ``nrf5340dk_nrf5340_cpuapp.overlay`` to disable spi3

.. code-block:: console

	status = "disabled";
 
Regarding SPI example, it serves the same function as `SPI master example`_. See `SPI master example`_ for the testing steps. 

Regarding RTC example, it’s just the same function as nRF5_SDK\examples\peripheral\rtc: after 5 seconds, LED2 is turned on by RTC ISR. 

The logging is like below.

.. code-block:: console

	<inf> raw_nrfx_thread: raw RTC cc0 evt
	<inf> raw_nrfx_thread: raw spi master thread
	<inf> raw_nrfx_thread: Transfer completed.	
	<inf> raw_nrfx_thread: Received:
			4e 6f 72 64 69 63 00
