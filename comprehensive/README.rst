.. comprehensive:

Comprehensive example
#####################

.. contents::
   :local:
   :depth: 2

This comprehensive example integrates BLE NUS service, BLE OTA service, high speed UART(1Mbps) example, SPI master example, I2C master example, 
ADC example, external interrupt example, Flash access example and device pm example. 

Overview
********

By default, nrf_dfu is used for the BLE OTA example (change ``prj.conf`` to support smp OTA DFU). And external Flash is used to store the update image.
Read `Testing`_ for more information of each separate module of this sample.


Build & Programming
*******************

The following NCS tags are tested for this sample. By default, NCS ``v1.8.0`` is used.

+------------------------------------------------------------------+
|NCS tags                                                          +
+==================================================================+
|v1.5.x/v1.6.x/v1.7.x/v1.8.x                                       |
+------------------------------------------------------------------+

To work with a specified NCS tag, read **prj.conf** carefully. Open the configurations relating to the specified version
and close the configurations of other versions. Search **NCS** in **prj.conf** to locate the configurations quickly.
	
This example may modify the original NCS source code. Refer to ``sdk_change`` for the detailed changes. For example, to work with NCS v1.7.1, 
enter folder ``sdk_change/ncs_v1.7.x`` and overwrite the same files in the correspondent NCS ``v1.7.1`` folders.

The following development kits are tested for this sample. 

+------------------------------------------------------------------+
|Build target                                                      +
+==================================================================+
|nrf5340dk_nrf5340_cpuapp/nrf52840dk_nrf52840                      |
+------------------------------------------------------------------+

For example, enter the following command to build ``nrf5340dk_nrf5340_cpuapp``.

.. code-block:: console

   west build -b nrf5340dk_nrf5340_cpuapp -d build_nrf5340dk_nrf5340_cpuapp -p
   
To flash the images to the board, just double click ``program.bat``, or use the following command:

.. code-block:: console

   west flash -d build_nrf5340dk_nrf5340_cpuapp

Testing
*******

After programming the sample to your development kit, you can test different modules by the following steps.

BLE NUS Service
===============

Perform the following steps for the test.

1. Connect the kit to the computer using a USB cable. The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. Open ``Serial Debug Assistant`` (which is available in Microsoft Store). Set baud rate as 1000000 and choose the second COM port of nRF5340 DK. 
#. Reset the kit.
#. Connect to the device using nRF Connect for Mobile. Tap **Enable CCCDs**.
#. Select the UART RX characteristic value in nRF Connect.
   You can write to the UART RX and get the text displayed on the COM listener.
#. Type '0123456789' and tap **Write**.
   Verify that the text "0123456789" is displayed on the COM listener.
#. To send data from the device to your phone or tablet, enter any text, for example, "Hello", and press Enter.
   Observe that a notification is sent to the phone or tablet.

Regarding nRF5340 DK, you need to connect the following pins together. UART1 will enumerate as the second COM of nRF5340 DK.

* Connect P0.07 to TxD of P24
* Connect P0.26 to RxD of P24

BLE OTA DFU
===========

By default, we use DFU module from nRF5 SDK v17.0.2 to do OTA in this sample. This DFU module is called nrf_dfu in this document. The OTA procedure is exactly
the same as that of nRF5 SDK. Perform the following steps for the test.

1. Connect the kit to the computer using a USB cable. The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. Optionally, connect the RTT console to display logging messages.
#. Reset the kit. It shall advertise ``nus_netcore``
#. Copy app_signed.hex (and net_core_app_signed.hex if you want to upgrade network core of nRF5340) in folder ``build*/zephyr`` to folder ``update``.
#. Double click ``zip_generate.bat`` in ``update``. You will get comprehensive_extFlash.zip as the new image bundle. (If you want to update nRF5340 network core too, edit ``zip_generate.bat`` to uncomment the second line)
#. Perform the DFU steps as nRF5 SDK do

Refer to https://github.com/aiminhua/ncs_samples/tree/master/nrf_dfu/ble_extFlash for a detailed description.

**note: In this sample, MCUBoot uses the default signing key, which must be replaced with your own key before production.** Do it like below:

.. code-block:: console

	CONFIG_BOOT_SIGNATURE_KEY_FILE="my_mcuboot_private.pem"	

SPI master
==========

This module shows how to call Zephyr SPI APIs to communicate with a SPI slave. The SPI slave image can be directly obtained from ``nRF5_SDK/examples/peripheral/spis``. 
To facilitate the test, we put the SPI slave images at ``resources``. The SPI slave pin definitions are shown below.

.. code-block:: console

   APP_SPIS_SCK_PIN 26
   APP_SPIS_MISO_PIN 30
   APP_SPIS_MOSI_PIN 29
   APP_SPIS_CS_PIN 31
   
**See your DTS file for SPI master pin definitions.**
 
Program the SPI slave hex file to a nRF52832DK or nRF52840DK. Connect the SPI slave related pins to their counterparts in your board.
After pressing **Button2**, this module starts to communicate with the SPI slave board. The logging looks similar to the following output.

.. code-block:: console

	<inf> spi_thread: Received SPI data:
			4e 6f 72 64 69 63 00

I2C master
==========

This module shows how to use Zephyr I2C APIs to communicate with a I2C slave. The I2C slave image can be directly obtained from ``nRF5_SDK/examples/peripheral/twi_master_with_twis_slave``.
To facilitate the test, we put the I2C slave images at ``resources``. The I2C slave pin definitions are shown below.

.. code-block:: console

	SCL_S         31   
	SDA_S         30  
   
**See your DTS file for I2C master pin definitions.**
 
Program the I2C slave hex file to a nRF52832DK or nRF52840DK. Connect the I2C slave related pins to their counterparts in your board.
After **Button4** is pushed down, this module starts to communicate with I2C slave. The logging looks like below.

.. code-block:: console

	<inf> i2c_thread: EEPROM:
			f8 6f 32 5f e4 21 80 65 e3 a3 4b 3c 8d 91 03 7f
	
ADC usage
=========

ADC has 2 working modes: sync and async mode. And it can sample many channels simultaneously. This module samples 2 channels (VDD and P0.05) together, 
and work in both sync and async mode. If you change the voltage on P0.05, you would see a changing ADC value from the log.

.. code-block:: console

	<inf> adc_thread: ADC thread
	<inf> adc_thread: Voltage0: 2988 mV / 3400
	<inf> adc_thread: Voltage1: 259 mV / 295
	<inf> adc_thread: Voltage0: 2988 mV / 3400 async
	<inf> adc_thread: Voltage1: 259 mV / 295 async

External interrupt
==================

In I2C example, we use **Button4** to trigger I2C communication. In fact, **Button4** is configured as an external interrupt.
The logging is like below.

.. code-block:: console

	<inf> i2c_thread: external interrupt occurs at 676640	

Flash access
============

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

Device PM
=========

We can use PM to turn on/off peripherals dynamically to save power consumption. 
In this example, press **Button3** to turn on/off UART0/UART1 repeatedly. If the logging backend is UART0, the logging message would be gone after pressing **Button3**.	
The logging is like below.

.. code-block:: console

	<inf> main: button3 isr
	<inf> main: UART1 is in active state. We suspend it	
	<inf> main: UART0 is in active state. We suspend it
	<inf> main: ## UART1 is suspended now ##	
	<inf> main: button3 isr
	<inf> main: UART1 is in low power state. We activate it	
	<inf> main: UART0 is in low power state. We activate it
	<inf> main: ## UART1 is active now ##	
	<inf> main: ## UART0 is active now ##


High speed UART
===============

In this module, you can achieve 1Mbps baud rate. UART has 3 working mode: poll, interrupt and async. To achieve high speed UART, async mode must be used.  
To test the reliability of 1Mbps UART, you can transfer a file from PC end to the device end. In this example, when PC sends some data to the device, the device 
would send the same data back to the PC. In this way, you can verify the reliability of 1Mbps UART.

When doing the loopback test of 1Mbps UART, make sure BLE connection is disconnected and logging terminal is closed since they would have a great
impact on the UART communication. You can use app: ``Serial Debug Assistant`` from Microsoft Store for the test. 

Use ``Serial Debug Assistant`` to send a file to the board. The board would forward the same file back to the PC. Verify whether they are identical.

Regarding nRF5340 DK, you need to connect the following pins together. UART1 will enumerate as the second COM of nRF5340 DK.

* Connect P0.07 to TxD of P24
* Connect P0.26 to RxD of P24

BT SMP DFU
==========

We can also do OTA by BT SMP protocol which is an inherent module of NCS. Change the default configurations before the building process.

* Change ``prj.conf``.

.. code-block:: console

	## Open the following configs to run nrf_dfu ##
	# CONFIG_NRF_DFU=y
	# CONFIG_NRF_DFU_RPC_APP=y
	# # CONFIG_NRF_DFU_LOG_LEVEL=3
	# CONFIG_IMG_MANAGER=y
	# CONFIG_MCUBOOT_IMG_MANAGER=y
	# CONFIG_IMG_BLOCK_BUF_SIZE=4096

	## Open the following configs to run SMP DFU ##
	CONFIG_MCUMGR=y
	CONFIG_MCUMGR_CMD_IMG_MGMT=y
	CONFIG_MCUMGR_CMD_OS_MGMT=y
	CONFIG_OS_MGMT_TASKSTAT=n
	CONFIG_OS_MGMT_ECHO=y
	CONFIG_IMG_BLOCK_BUF_SIZE=2048
	CONFIG_MCUMGR_BUF_SIZE=256
	CONFIG_MCUMGR_BUF_COUNT=4
	CONFIG_MGMT_CBORATTR_MAX_SIZE=512
	CONFIG_RPC_SMP_BT=y

Then build the project and program it to the board. Perform the following steps to test SMP DFU.

1. Connect the kit to the computer using a USB cable. The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. Copy ``build*/zephyr/app_update.bin`` to your mobile phone. (If you want to update the net core image, use **net_core_app_update.bin** instead)
#. Open nRF Connect for Mobile on your phone. 
#. Connect the board. 
#. Tap **DFU** button on the right top corner of the mobile app.
#. Select **app_update.bin** in your phone. (If you want to update the net core image, use **net_core_app_update.bin** instead)
#. Complete the DFU process.

Refer to https://github.com/aiminhua/ncs_samples/tree/master/smp_dfu/ble_extFlash for an independent SMP DFU example.