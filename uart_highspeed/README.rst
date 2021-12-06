.. uart_highspeed:

high speed uart example
##################

.. contents::
   :local:
   :depth: 2


Overview
********

In this example, you can achieve 1Mbps baud rate. UART has 3 working modes: poll, interrupt and async. To achieve high speed UART, async mode must be used.  
To test the reliability of 1Mbps UART, you can transfer a file from PC end to the device end. In this example, when PC sends some data to the device, the device 
would send the same data back to the PC. In this way, you can verify the reliability of 1Mbps UART.

When doing the loopback test of 1Mbps UART, make sure RTT logging terminal is closed since it would have a impact on the UART communication. 
You can use ``Serial Debug Assistant`` from Microsoft Store for the test. 

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

1. Connect the kit to the computer using a USB cable. The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. If UART0 is used, no extra wiring is needed. if UART1 is used, you need to connect the UART pins to tx and rx on the same board.
#. Open ``Serial Debug Assistant``. Choose the right COM port. And do the following settings.

.. code-block:: console

   baud rate: 1000000
   data bits: 8
   parity: none
   stop bits: 1
   no hardware flow control or no DTR/RTS
   
#. Click ``Open serial port``
#. Select ``Send a file``. You can choose zephyr.hex for example. The same file would be sent back to the terminal by the kit. Compare the contents of the 2 files.
