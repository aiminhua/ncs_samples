@REM nrfjprog -f NRF53 --coprocessor CP_NETWORK --recover
@REM nrfjprog -f NRF53 --recover

nrfjprog -f NRF53 --coprocessor CP_NETWORK --eraseall
nrfjprog -f NRF53 --coprocessor CP_NETWORK --program build/hci_rpmsg/zephyr/merged_CPUNET.hex --verify

nrfjprog --eraseall
nrfjprog --program build/zephyr/merged.hex --verify

nrfjprog -r
pause