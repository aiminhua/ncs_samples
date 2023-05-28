nrfjprog -f NRF53 --coprocessor CP_NETWORK --recover
nrfjprog --recover

nrfjprog --coprocessor CP_NETWORK --eraseall
::nrfjprog --program build/hci_rpmsg/zephyr/merged_CPUNET.hex --sectorerase --verify
nrfjprog --program build_1/hci_rpmsg/zephyr/merged_CPUNET.hex --verify
nrfjprog --eraseall
::nrfjprog --program build/zephyr/merged.hex --sectorerase --verify
nrfjprog --program build_1/zephyr/merged.hex --verify
nrfjprog --program build_1/zephyr/xip_signed.hex --qspiini gdqspi.ini --qspisectorerase --verify
 
nrfjprog --debugreset

pause