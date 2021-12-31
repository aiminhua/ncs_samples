.. i2c_master:

I2C master example
##################

.. contents::
   :local:
   :depth: 2


Overview
********

This example shows how to use Zephyr I2C APIs to communicate with a I2C slave. The I2C slave image can be directly obtained from ``nRF5_SDK/examples/peripheral/twi_master_with_twis_slave``.
To facilitate the test, we put the twis images at ``resources``. The twis pin definitions are shown below.

.. code-block:: console

	EEPROM_SIM_SCL_S         31   
	EEPROM_SIM_SDA_S         30  
   
**See your DTS file for I2C master pin definitions.**
 
Build & Programming
*******************

The following development kits are tested for this sample. However, other nRF52 SoC should work too.

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

After programming the sample to your development kit, test it by performing the following steps:

1. Program the twis hex file to a nRF52832DK or nRF52840DK from ``resources``. 
#. Connect the twis related pins to their counterparts in your board.
#. Connect the kit to the computer using a USB cable. The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. This example starts to communicate with twis automatically. 

Once the communication is successful, the logging looks similar to the following output.

.. code-block:: console

	<inf> main: i2c master thread
			15 e0 41 46 58 46 ff f7  ec f8 00 28 f4 d0 21 46
	<inf> main: i2c master thread
			58 46 01 f0 a1 fa 20 6a  b8 42 f2 d3 57 46 41 46
	<inf> main: i2c master thread
			28 68 ff f7 de f8 00 28  f9 d0 28 68 c7 60 c4 f8

