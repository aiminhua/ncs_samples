::generate settings page for current image: zephyr.hex
nrfutil settings generate --family NRF52840 --application zephyr.hex --application-version 1 --bootloader-version 1 --bl-settings-version 2 settings.hex
::merge bootloader and settings
mergehex --merge bootloader_debug.hex settings.hex --output bl_temp.hex
::merge bootloader, app and mbr
mergehex --merge bl_temp.hex zephyr.hex mbr_nrf52_2.4.1_mbr.hex --output whole.hex

nrfjprog --eraseall -f NRF52
nrfjprog --program whole.hex --verify -f NRF52
nrfjprog --reset 
pause

