nrf_dfu example
###############

nrf_dfu module is ported from nRF5 SDK v17.0.2. It now can work in nRF Connect SDK too. The DFU procedure is exactly the same as that of nRF5 SDK. 
See https://infocenter.nordicsemi.com/index.jsp?topic=%2Fsdk_nrf5_v17.0.2%2Fble_sdk_app_dfu_bootloader.html
for a detailed description of nRF5 SDK DFU steps if you don't have too much knowledge of it.

In this folder, we have many examples. By default, MCUboot is used as the bootloader. However ``inFlash_nrf5_bl`` uses nRF5 bootloader in nRF5 SDK. 
``inFlash`` indicates the secondary slot or bank1 is put in the internal SoC Flash. ``exFlash`` indicates the secondary slot is put in the external QSPI Flash.
``rpc`` can only apply to nRF53 series, which indicates both BLE host and controller run on the network core. 
