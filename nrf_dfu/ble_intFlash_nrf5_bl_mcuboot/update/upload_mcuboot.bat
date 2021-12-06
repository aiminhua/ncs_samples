::nrfutil pkg generate --application zephyr.hex --application-version 1 --hw-version 52 --sd-req 0x00 mcuboot_update.zip
::nrfutil dfu usb-serial -pkg mcuboot_update.zip -p COM13
nrfutil pkg generate --application zephyr.hex --application-version 1 --hw-version 52 --sd-req 0x00 --key-file priv.pem mcuboot_update.zip
nrfutil dfu serial -pkg mcuboot_update.zip -p COM4
pause