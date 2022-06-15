nrfjprog --eraseall
nrfjprog --program build_nrf52840dk_nrf52840/zephyr/merged.hex --verify

nrfjprog -r
pause