nrfutil pkg generate --application zephyr.hex --application-version 1 --hw-version 52 --sd-req 0x00 ncs_840dongle.zip
nrfutil dfu usb-serial -pkg ncs_840dongle.zip -p COM23
pause