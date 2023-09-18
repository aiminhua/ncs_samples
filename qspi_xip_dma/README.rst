.. qspi_xip_dma:

QSPI_XIP_DMA OTA Sample
#######################

Overview
********

This sample demonstrates how QSPI DMA coexists with QSPI XIP. In addition, it sets up the whole OTA framework.

In this sample, we have 3 primary images for OTA: app image of application core, xip image of application core and app image of network core.
3 secondary slots are needed to store the update images of the 3 primary images. Once you transfer the update images to the secondary slots,
the update process starts after a reset. The detailed update process is shown below.

1. Upload the app_update.bin to partition:mcuboot_secondary if you want to update the app image of application core.
#. Upload the xip_signed.bin to partition:mcuboot_secondary_2 if you want to update the xip image of application core.
#. Upload the net_core_app_update.bin to partition:mcuboot_secondary_1 if you want to update the app image of network core.
#. Write a magic word to mcuboot_secondary. you can use boot_request_upgrade_multi(0, 0) or boot_request_upgrade_multi(1, 0) to do the trick.
#. Reset the device.
#. MCUboot would perform the subsequent updating process.

Refer to main.c on how to operate a partition. For example, if you want to transfer a image to mcuboot_secondary by WiFi or BT,
use the following API sequence.

1. flash_area_open(PM_MCUBOOT_SECONDARY_ID, &fap);
#. flash_area_erase(fap, offset, size);
#. flash_area_write(fap, offset, buf, len);
#. flash_area_read(fap, offset, buf, len);

Note that flash_area operations don't have alignment and buffer features. You can use stream_flash_buffered_write to handle alignment and buffer
if you don't want to touch it by your own application code.


Building and Running
********************

NCS ``v2.3.0`` is used when developing the sample.

This example need to modify the original NCS source code. Refer to ``sdk_change`` for the detailed changes.

The following development kits are tested for this sample. 

+------------------------------------------------------------------+
|Build target                                                      +
+==================================================================+
|nrf5340dk_nrf5340_cpuapp                                          |
+------------------------------------------------------------------+

You can build the sample by VS code. Or enter the following command.

.. code-block:: console

   west build -b nrf5340dk_nrf5340_cpuapp -p
   
if you relocate constants to xip area too, it has a bug for the linker script. **Change _EXTFLASH_RODATA_SECTION_NAME of build/zephyr/linker.cmd** 

Use ``nordic_program.bat`` to program the images to the board.

To support a custom board, you need to pay attention to the following points.

1. Change overlay file of the board.
#. Change src/ext_mem_init.c to suit the board's qspi Flash.
#. Change nordic_program.bat to accommodate the board's needs.


Sample Output
=============

1. Connect the kit to the computer using a USB cable. The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. Open a serial terminal.
#. You would see the following output.

.. code-block:: console

	*** Booting Zephyr OS build v3.2.99-ncs2-1-g2cb73a925d23 ***
	I: Starting bootloader
	I: Swap type: none
	I: Swap type: none
	primary XIP hdr size:0x200 img size:544
	I: Bootloader chainload address offset: 0xc000
	I: Jumping to the first image slot
	init xip over
	*** Booting Zephyr OS build v3.2.99-ncs2-1-g2cb73a925d23 ***

	### reset reason: 0x00000001  ####

	[00:00:00.422,821] [0m<inf> adc_thread: **ADC sampling example[0m
	[00:00:00.422,851] [0m<inf> adc_thread: ADC thread[0m
	[00:00:00.423,034] [0m<inf> adc_thread: Voltage0: 2648 mV / 3013[0m
	[00:00:00.423,034] [0m<inf> adc_thread: Voltage1: 111 mV / 127[0m
	Bluetooth initialized
	XIP variable: 0x10000400
	XIP variable calculation result: 21


	##### Loop: 1  start ######

	suspended xip thread!

	#####Switched to QSPI DMA #####
	###############################

	Turn off UART0/SPI4/QSPI. Watch the power consumption for 10 seconds
	Turning off UART0/SPI4/QSPI to save power
	Turning on UART0/SPI4/QSPI
	Entered active state
	###############################

	Turn ON UART0/SPI4/QSPI

	Test 1: Flash erase
	Flash erase succeeded!

	Test 2: Flash write
	Attempting to write 4 bytes
	Data read matches data written. Good!!
	init xip over

	####### Switched to QSPI XIP ########
	resumed xip thread!
	get xip var:0x22
	XIP variable calculation result: 23

	##### Loop: 1  end ######

OTA Test
========

Follow the steps below to complete DFU process.

1. Change your codes and build again.
#. Click ``update.bat`` to transfer the update images to the secondary slots by J-Link.
#. MCUboot will finish the rest update steps.
#. You would see logs like below.

.. code-block:: console

	*** Booting Zephyr OS build v3.2.99-ncs2-1-g2cb73a925d23 ***
	I: Starting bootloader
	I: Swap type: test
	secondary XIP hdr size:200 img size:6e0
	I: Swap type: test
	I: Image upgrade secondary slot -> primary slot
	I: Erasing the primary slot
	I: Copying the secondary slot to the primary slot: 0x20680 bytes
	Copying secondary XIP to primary XIP
	I: Image upgrade secondary slot -> primary slot
	I: Erasing the primary slot
	I: Copying the secondary slot to the primary slot: 0x2d90c bytes
	I: Turned on network core
	I: Turned off network core
	primary XIP hdr size:0x200 img size:1760
	I: Bootloader chainload address offset: 0xc000
	I: Jumping to the first image slot
	init xip over
	*** Booting Zephyr OS build v3.2.99-ncs2-1-g2cb73a925d23 ***

	### NEW  reset reason: 0x0000000c  ####

	[00:00:00.459,564] [0m<inf> adc_thread: **ADC sampling example[0m

	[00:00:00.459,625] [0m<inf> adc_thread: ADC thread[0m
	[00:00:00.459,777] [0m<inf> adc_thread: Voltage0: 2910 mV / 3312[0m
	[00:00:00.459,808] [0m<inf> adc_thread: Voltage1: 210 mV / 240[0m
	Bluetooth initialized
	XIP variable: 0x100008c0
	XIP variable calculation result: 21
	
	##### Loop: 1  start ######