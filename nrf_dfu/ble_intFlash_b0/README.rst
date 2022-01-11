.. nrf_dfu_inFlash_b0:

nrf_dfu OTA example(b0 bootloader)
##################################

.. contents::
   :local:
   :depth: 2

Use DFU module from nRF5 SDK v17.0.2 to do OTA in nRF Connect SDK. This DFU module is called nrf_dfu in this document. The OTA procedure is exactly the
same as that of nRF5 SDK. See https://infocenter.nordicsemi.com/index.jsp?topic=%2Fsdk_nrf5_v17.0.2%2Fble_sdk_app_dfu_bootloader.html
for a detailed description of nRF5 SDK DFU steps if you don't have too much knowledge of it.

Overview
********

In this sample, B0 is used for the bootloader. Once boot up, B0 would verify 2 image slots: slot0 and slot1. If the version of slot1 image
is higher than slot0, slot1 image would be loaded. Otherwise, slot0 image would be loaded and run. When updating image, if the running image
is in slot0, your update image should be uploaded to slot1; if the running image is in slot1, your update image should be uploaded to slot0. 
To achieve this kind of DFU, you need have a dedicated mobile app to upload ``dfu_application.zip`` to the device. We use nRF connect for mobile
to simulate the DFU process in this sample. 

**note: In this sample, B0 uses the default signing key, which must be replaced with your own key before production.** Do it like below:

.. code-block:: console

	CONFIG_SB_SIGNING_KEY_FILE="b0_private.pem"	

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

The following development kits are tested for this sample. However, other nRF52 SoC should work too.

+------------------------------------------------------------------+
|Build target                                                      +
+==================================================================+
|nrf52840dk_nrf52840                                               |
+------------------------------------------------------------------+

For example, enter the following command to build ``nrf52840dk_nrf52840``.

.. code-block:: console

   west build -b nrf52840dk_nrf52840 -d build_nrf52840dk_nrf52840 -p

To flash the images to the board, just double click ``program.bat``, or use the following command:

.. code-block:: console

   west flash -d build_nrf52840dk_nrf52840     

Testing
*******

After programming the sample to your development kit, test it by performing the following steps:

1. Connect the kit to the computer using a USB cable. The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. Reset the kit. It shall advertise ``Nordic_DFU``
#. Copy signed_by_b0_s1_image.hex and signed_by_b0_s0_image.hex in folder ``build*/zephyr`` to folder ``update``.
#. Double click ``zip_generate.bat`` in ``update``.You will get ble_intFlash_b0_s0.zip or ble_intFlash_b0_s1.zip as the new image bundle. 
#. Observe the logging of the firmware, select the right zip file for the DFU process.
#. Perform the DFU steps as nRF5 SDK do
