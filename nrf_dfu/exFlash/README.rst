.. exFlash:

nrf_dfu OTA example(external secondary slot)
############################################

.. contents::
   :local:
   :depth: 2

Use DFU module from nRF5 SDK v17.0.2 to do OTA in nRF Connect SDK. This DFU module is called nrf_dfu in this document. The OTA procedure is exactly the
same as that of nRF5 SDK. See https://infocenter.nordicsemi.com/index.jsp?topic=%2Fsdk_nrf5_v17.0.2%2Fble_sdk_app_dfu_bootloader.html
for a detailed description of nRF5 SDK DFU steps if you don't have too much knowledge of it.

Overview
********

In this sample, the secondary slot is on the external Flash. That is, the new image will be stored in the secondary slot first. After that, MCUBoot would perform
the swap operation to finish the whole DFU process. Regarding ``nRF5340``, the BLE host is running on the application core in this sample(``zephyr/samples/bluetooth/hci_rpmsg`` runs on the netcore). 

Build & Programming
*******************

By default, this sample works with NCS ``v1.6.0``. To work with other versions of NCS, read **prj.conf** carefully. Open the configurations relating to the specified version
and close the configurations of other versions. Search **NCS** in **prj.conf** to locate the configurations quickly.
	
Before building this sample, enter folder ``sdk_change/ncs_v1.6.0`` and overwrite the same files in the correspondent NCS ``v1.6.0`` folders. If you want to build this sample
in NCS ``v1.5.1`` or ealier. Use folder ``sdk_change/ncs_v1.5.1`` instead. 

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
#. |connect_terminal|
#. Optionally, connect the RTT console to display logging messages.
#. Reset the kit. It shall advertise ``Nordic_DFU``
#. Enter ``zephyr folder`` of the ``build`` folder. Copy app_signed.hex and net_core_app_signed.hex(this file is not present for nRF52) to folder ``update_zip``. Double click ``zip_generate.bat``.
#. If you want to update net core image, use 53_netcore_extFlash_norpc.zip. if you want to update app core image, use 53_appcore_extFlash_norpc.zip
#. Perform the DFU steps as nRF5 SDK do
