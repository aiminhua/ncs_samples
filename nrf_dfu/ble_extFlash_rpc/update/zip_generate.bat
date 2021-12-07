nrfutil pkg generate --application app_signed.hex --application-version 2 --hw-version 52 --sd-req 0x00 ble_extFlash_rpc.zip
nrfutil pkg generate --application net_core_app_signed.hex --application-version 2 --hw-version 52 --sd-req 0x00 ble_extFlash_rpc_netcore.zip
pause