::nrfutil device recover
nrfutil device erase

mergehex -m build_1\vpr_launcher\zephyr\zephyr.hex build_2\uart_highspeed\zephyr\zephyr.hex -o merged.hex
::nrfutil device program --firmware build_1/merged.hex
nrfutil device program --firmware merged.hex
pause