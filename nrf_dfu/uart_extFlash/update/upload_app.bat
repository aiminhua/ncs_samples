nrfutil pkg generate --application app_signed.hex --application-version 2 --hw-version 52 --sd-req 0x00 uart_nrf_dfu_ext.zip
nrfutil dfu serial -pkg uart_nrf_dfu_ext.zip -p COM5 -b 115200

pause

::nrfutil pkg generate --application net_core_app_signed.hex --application-version 2 --hw-version 52 --sd-req 0x00 uart_nrf_dfu_ext_net.zip
::nrfutil dfu serial -pkg uart_nrf_dfu_ext_net.zip -p COM5 -b 115200

::pause