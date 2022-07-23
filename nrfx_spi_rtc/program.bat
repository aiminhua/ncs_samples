::nrfjprog -f NRF53 --coprocessor CP_NETWORK --recover
::nrfjprog -f NRF53 --recover

nrfjprog --eraseall
nrfjprog --program build/zephyr/zephyr.hex --verify

nrfjprog -r
pause