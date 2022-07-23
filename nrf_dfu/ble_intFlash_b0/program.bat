nrfjprog --eraseall
nrfjprog --program build/zephyr/merged.hex --verify

nrfjprog -r
pause