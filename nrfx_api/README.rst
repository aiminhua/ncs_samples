.. nrfx_api:

nrfx API example
################

.. contents::
   :local:
   :depth: 2


Overview
********

This example shows how to call nrfx API directly. It contains a SPI master and RTC example. The SPI master example works the same as 
``nRF5_SDK/examples/peripheral/spi``, which can communicate with ``nRF5_SDK/examples/peripheral/spis``. The RTC example works the same as 
``nRF5_SDK/examples/peripheral/rtc``.

To facilitate the test, we put the spis images at ``resources``. The spis pin definitions are shown below.

.. code-block:: console

   APP_SPIS_SCK_PIN 26
   APP_SPIS_MISO_PIN 30
   APP_SPIS_MOSI_PIN 29
   APP_SPIS_CS_PIN 31
   
The SPIM pin definitions are shown below.

.. code-block:: console

	SPI_SS_PIN 44
	SPI_MOSI_PIN 45
	SPI_MISO_PIN 46
	SPI_SCK_PIN 47

Build & Programming
*******************

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

After programming the sample to your development kit, test it by performing the following steps:

1. Program the spis hex file to a nRF52832DK or nRF52840DK from ``resources``. 
#. Connect the spis related pins to their counterparts in your board.
#. Connect the kit to the computer using a USB cable. The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. This example starts to communicate with spis automatically.
#. LED2 or PIN29 would go off after 5 seconds

Once the communication is successful, the logging looks similar to the following output.

.. code-block:: console

	<inf> main: nrfx api thread
	<inf> main: Transfer completed.
	<inf> main: Received: 
			4e 6f 72 64 69 63 00
	<inf> main: nrfx api thread
	<inf> main: Transfer completed.
	<inf> main: Received: 
			4e 6f 72 64 69 63 00
	<inf> main: ## nrfx RTC cc0 evt ##
