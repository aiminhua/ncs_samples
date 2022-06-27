nrfjprog -f NRF53 --coprocessor CP_NETWORK --recover
nrfjprog -f NRF53 --recover

nrfjprog -f NRF53 --eraseall
nrfjprog -f NRF53 --program build/zephyr/zephyr.hex --verify

nrfjprog -r
pause