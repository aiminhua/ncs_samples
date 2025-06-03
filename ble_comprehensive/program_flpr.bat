::python ../../zephyr/scripts/build/mergehex.py ../uart_highspeed/build_2/uart_highspeed/zephyr/zephyr.hex build_1/merged.hex -o app_and_flpr_merged.hex
mergehex -m ../uart_highspeed/build_2/uart_highspeed/zephyr/zephyr.hex build_1/merged.hex -o app_and_flpr_merged.hex
::nrfutil device recover
nrfutil device erase
nrfutil device program --firmware app_and_flpr_merged.hex
pause