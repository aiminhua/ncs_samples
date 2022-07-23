nrfutil pkg generate --application app_signed.hex --application-version 2 --hw-version 52 --sd-req 0x00 uart_nrf_dfu_int.zip
nrfutil dfu serial -fc 0 -t 3 -pkg uart_nrf_dfu_int.zip -p COM6 -b 115200
pause
nrfutil pkg generate --application net_core_app_signed.hex --application-version 2 --hw-version 52 --sd-req 0x00 uart_nrf_dfu_int_net.zip
nrfutil dfu serial -fc 0 -t 6 -pkg uart_nrf_dfu_int_net.zip -p COM6 -b 115200

pause