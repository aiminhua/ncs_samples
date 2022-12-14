::external interrupt

External interrupt
###################

.. contents::
   :local:
   :depth: 2

low power external interrupt example.

Overview
********

Use **Button1** as the external interrupt source. Once pushing the **Button1**, it will trigger an external interrupt

	
Build & Programming
*******************

The following NCS tags are tested for this sample. By default, NCS ``v2.2.0`` is used.

+------------------------------------------------------------------+
|NCS tags                                                          +
+==================================================================+
|v1.9.x/v2.0.x/v2.2.x                                              |
+------------------------------------------------------------------+

Use ``git tag`` to see supported tags. For ncs versions later than v1.9.0, for example ncs v2.0.0, 
use ``git checkout v2.0`` to switch to the specified NCS tag. Use ``git checkout v1.5_v1.9`` to switch to 
ncs versions earlier than v1.9.0. After the checkout operation, open this README.rst again and follow 
the instructions. 
	
The following development kits are tested for this sample. 

+------------------------------------------------------------------+
|Build target                                                      +
+==================================================================+
|nrf5340dk_nrf5340_cpuapp/nrf52840dk_nrf52840/nrf9160dk_nrf9160ns  |
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

1. Connect the kit to the computer using a USB cable. 
#. Open RTT viewer
#. Push **Button1**. An external interrupt was triggered.

The log looks similar to the below. 

.. code-block:: console

	[00:00:00.474,365] <inf> main: ** External interrupt example **
	[00:00:00.474,395] <inf> main: External interrupt example at Pin:23
	[00:00:00.474,426] <inf> main: main thread
	[00:00:05.474,517] <inf> main: main thread
	[00:00:09.549,438] <inf> main: external interrupt occurs at 800000