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

By default, smp BT DFU is used for the BLE OTA example. And external Flash is used to store the update image.
Read `Testing`_ for more information of each separate module of this sample.


Build & Programming
*******************

The following NCS tags are tested for this sample. By default, NCS ``v2.9.0`` is used.

+---------------------------------------------------------------------------------------------------+
|NCS tags                                                                                           +
+===================================================================================================+
|v1.5.x/v1.6.x/v1.7.x/v1.8.x/v1.9.x/v2.0.x/v2.2.x/v2.3.x/v2.4.x/v2.5.x/v2.6.x/v2.7.x/v2.8.0/v2.9.0  |
+---------------------------------------------------------------------------------------------------+

Use ``git tag`` to see supported tags. For ncs versions later than v1.9.0, for example ncs v2.0.0, 
use ``git checkout v2.0`` to switch to the specified NCS tag. Use ``git checkout v1.5_v1.9`` to switch to 
ncs versions earlier than v1.9.0. After the checkout operation, open this README.rst again and follow 
the instructions. 
	
This example **may** modify the original NCS source code. Refer to ``sdk_change`` for the detailed changes. 
For example, to work with NCS v1.9.1, copy folder ``sdk_change/ncs_v1.9.x`` and overwrite the same files 
in the correspondent NCS ``v1.9.1`` folders.

The following development kits are tested for this sample. 

+-------------------------------------------------------------------------------------------------+
|Build target                                                                                     +
+=================================================================================================+
|nrf54l15dk_nrf54l15_cpuapp nrf5340dk/nrf5340/cpuapp nrf52840dk/nrf52840 nrf7002dk/nrf5340/cpuapp |
+-------------------------------------------------------------------------------------------------+

For example, enter the following command to build ``nrf54l15dk_nrf54l15_cpuapp``.

.. code-block:: console

   west build -b nrf54l15dk_nrf54l15_cpuapp --sysbuild -p
   
To flash the images to the board, use the following command:

.. code-block:: console

   west flash

Testing
*******

After programming the sample to your development kit, you can test different modules by the following steps.

BLE NUS Service
===============

Perform the following steps for the test.

1. Connect the kit to the computer using a USB cable. The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. Open ``Serial Debug Assistant`` (which is available in Microsoft Store). Set baud rate as 1000000 and choose the second COM port of nRF5340 DK 
   Note: the latest nrf5340dk, we only have 2 COM ports. So you need to have a USB-to-UART bridge or you can change source code to make use of uart0.
#. Reset the kit. It shall advertise ``comprehensive``
#. Connect to the device using nRF Connect for Mobile. Tap **Enable CCCDs**.
#. Select the UART RX characteristic value in nRF Connect.
   You can write to the UART RX and get the text displayed on the COM listener.
#. Type '0123456789' and tap **Write**.
   Verify that the text "0123456789" is displayed on the COM listener.
#. To send data from the device to your phone or tablet, enter any text, for example, "Hello", and press Enter.
   Observe that a notification is sent to the phone or tablet.

Regarding old nRF5340DK, you need to connect the following pins together. UART1 will enumerate as the second COM of nRF5340 DK.
In this case, you don't need to have a USB-to-UART bridge.

* Connect P0.07 to TxD of P24
* Connect P0.26 to RxD of P24

BLE OTA DFU
===========

By default, we use BLE smp protocol to do OTA. You can update application core and network core images in one go, or update them separately.
Perform the following steps for the test.

1. Connect the kit to the computer using a USB cable. The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. Open nRF Connect or Device Manager on your phone. 
#. Connect the board. 
#. Copy ``build/dfu_application.zip`` or ``build/ble_comprehensive/zephyr/zephyr.signed.bin`` to your phone. (Change some code lines before a second build)
#. Tap **DFU** button on the right top corner of nRF Connect.
#. Select ``dfu_application.zip`` or ``zephyr.signed.bin`` on your phone.
#. Wait until the DFU process is done.
#. nRF Connect would auto connect the device to confirm the new image if applicable. Otherwise, the old image would restore after a second reset.

**note: In this sample, MCUboot uses the default signing key, which must be replaced with your own key before production.** Do it like below:

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

	[00:04:38.533,844] <inf> spi_thread: spi master thread
	[00:04:38.534,155] <inf> spi_thread: Received SPI dev0 data: 
										4e 6f 72 64 69 63 00         
	[00:04:38.534,454] <inf> spi_thread: SPI dev1 write success

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

	[00:01:55.881,248] <inf> i2c_thread: i2c master thread
	[00:01:55.881,849] <inf> i2c_thread: EEPROM: 
										f8 f7 66 ff 1e b9 25 a1  f4 20 f8 f7 61 ff 28 46 
	[00:01:55.882,450] <inf> i2c_thread: EEPROM: 
										00 f0 60 f8 10 b1 11 20  bd e8 f0 9f 66 61 4f f0
	[00:01:55.883,041] <inf> i2c_thread: EEPROM: 
										00 09 c4 f8 20 90 a7 60  84 f8 28 90 4f f4 8e 78
	
ADC usage
=========

ADC has 2 working modes: sync and async mode. And it can sample many channels simultaneously. This module samples 2 channels (VDD and P0.05) together, 
and work in both sync and async mode. If you change the voltage on P0.05, you would see a changing ADC value from the log.

.. code-block:: console

	[00:04:30.853,300] <inf> adc_thread: ADC thread
	- adc@d5000, channel 0: 2 = 4 mV
	- adc@d5000, channel 1: 1023 = 2247 mV

External interrupt
==================

In I2C example, we use **Button4** to trigger I2C communication. In fact, **Button4** is configured as an external interrupt.
The logging is like below.

.. code-block:: console

	[00:00:22.533,525] <inf> extint_thread: external interrupt occurs on pin 0x10 at 0x1f589

Flash access
============

There are 3 layers(sets) of Flash access APIs in NCS: Flash area API, NVS API and Settings API. The bottom layer is Flash area API which access Flash directly 
without additional headers or tails. NVS API invokes Flash area API to achieve the Flash access purpose. To have a better reliability and readability, 
NVS would add some additional info at the end of a page.  Settings API calls NVS API to access Flash memory. Thus, Settings module has a further encapsulation 
of raw serialized data. All data is managed by key/value pair in Settings module.

In this example, we use both NVS API and Settings API to do the same thing: store a secret and reboot counter onto the internal Flash. The logging is like below.

.. code-block:: console

	[00:00:00.843,753] <inf> settings_thread: settings subsys initialization: OK.
	[00:00:00.843,764] <inf> settings_thread: Load all key-value pairs using registered handlers
	[00:00:00.843,829] <inf> settings_thread: set handler name=boot_cnt, len=4 
	[00:00:00.843,854] <inf> settings_thread: *** Reboot counter in Settings: 3 ****
	[00:00:00.843,930] <inf> settings_thread: set handler name=key, len=8 
	[00:00:00.843,977] 0m<inf> settings_thread: Key value in Settings:
											30 31 32 33 34 35 36 37                          
	[00:00:00.843,990] <inf> settings_thread: Settings thread
	[00:00:00.843,996] <inf> settings_thread: save new reboot counter by Settings API                       

Device PM
=========

We can use PM to turn on/off peripherals dynamically to save power consumption. 
In this example, press **Button0** to turn on/off peripherals repeatedly. If the logging backend is UART, the logging message would be gone after pressing **Button0**.	
The logging is like below.

.. code-block:: console

	[00:02:23.346,708] <inf> main: button1 isr
	[00:02:23.346,728] <inf> main: Turning off UART/SPI/I2C to save power
	[00:02:23.346,832] <inf> uart_thread: UART_RX_BUF_RELEASED
	[00:02:23.346,844] <inf> uart_thread: UART_RX_BUF_RELEASED
	[00:02:23.346,854] <inf> uart_thread: UART_RX_DISABLED
	[00:02:23.356,858] <inf> main: Entered lowe power
	
    [00:03:29.875,444] <inf> main: button1 isr
    [00:03:29.875,458] <inf> main: Turning on UART/SPI/I2C
    [00:03:29.875,492] <inf> main: Entered active state


High speed UART
===============

In this module, you can achieve 1Mbps baud rate. UART has 3 working mode: poll, interrupt and async. To achieve high speed UART, async mode must be used.  
To test the reliability of 1Mbps UART, you can transfer a file from PC end to the device end. In this example, when PC sends some data to the device, the device 
would send the same data back to the PC. In this way, you can verify the reliability of 1Mbps UART.

When doing the loopback test of 1Mbps UART, make sure BLE connection is disconnected and logging terminal is closed since they would have a great
impact on the UART communication. You can use app: ``Serial Debug Assistant`` from Microsoft Store for the test. 

Use ``Serial Debug Assistant`` to send a file to the board. The board would forward the same file back to the PC. Verify whether they are identical.

Note: please shut down your logging terminal to achieve the 1Mbps baud rate.

.. code-block:: console

	[00:01:50.627,425] <inf> uart_thread: UART_RX_RDY 255
	[00:01:50.627,442] <inf> uart_thread: UART_RX_BUF_RELEASED
	[00:01:50.627,541] <inf> uart_thread: uart received:
										44 65 61 72 20 61 6c 6c  2c 0d 0a 20 0d 0a 41 73 
										20 64 69 73 63 75 73 73  65 64 20 6a 75 73 74 20 
										6e 6f 77 2c 20 77 65 e2  80 99 6c 6c 20 73 74 61 
										72 74 20 4e 43 20 77 65  65 6b 6c 79 20 75 70 64 
										61 74 65 20 66 72 6f 6d  20 6e 65 78 74 20 77 65 
										65 6b 2c 20 40 44 69 6e  67 2c 20 45 72 69 63 40 
										5a 68 61 6e 67 2c 20 4f  6c 69 76 65 72 70 6c 65 
										61 73 65 20 6d 61 6b 65  20 73 75 72 65 20 74 68 
										61 74 20 74 68 65 20 6f  6e 65 6e 6f 74 65 20 49 
										20 73 68 61 72 65 64 20  74 6f 20 79 6f 75 20 e2 
										80 98 4e 43 20 77 65 65  6b 6c 79 20 75 70 64 61 
										74 65 e2 80 99 20 69 73  20 75 70 64 61 74 65 64 
										20 62 65 66 6f 72 65 20  74 68 65 20 6d 65 65 74 
										69 6e 67 2e 20 54 68 61  6e 6b 20 79 6f 75 21 0d 
										0a 54 68 65 20 6d 65 65  74 69 6e 67 20 77 69 6c 
										6c 20 6c 61 73 74 20 66  6f 72 20 61 62 6f 75    
	[00:01:50.630,096] <inf> uart_thread: UART_TX_DONE 255 
