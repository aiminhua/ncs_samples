nrfjprog --eraseall
nrfjprog --program build/zephyr/zephyr.hex --verify

nrfjprog -r
pause