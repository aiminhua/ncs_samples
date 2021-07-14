nrfutil pkg generate --application app_signed.hex --application-version 2 --hw-version 52 --sd-req 0x0101 53_app_update_ext.zip
nrfutil pkg generate --application net_core_app_signed.hex --application-version 2 --hw-version 52 --sd-req 0x0101 53_net_update_ext.zip
pause