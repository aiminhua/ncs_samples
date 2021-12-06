::merge bootloader and mbr
mergehex --merge secure_bootloader_uart_mbr_pca10056.hex mbr_nrf52_2.4.1_mbr.hex --output nrf5_image.hex

nrfjprog --eraseall -f NRF52
nrfjprog --program nrf5_image.hex --verify -f NRF52
nrfjprog --reset 
pause

