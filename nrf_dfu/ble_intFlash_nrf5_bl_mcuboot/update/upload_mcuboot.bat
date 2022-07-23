nrfutil pkg generate --application zephyr.hex --application-version 1 --hw-version 52 --sd-req 0x00 --key-file priv.pem mcuboot_update.zip
nrfutil dfu serial -pkg mcuboot_update.zip -fc 0 -t 3 -p COM3
pause