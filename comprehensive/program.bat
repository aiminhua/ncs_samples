::nrfjprog -f NRF53 --coprocessor CP_NETWORK --recover
::nrfjprog -f NRF53 --recover
::west flash -d build_nrf5340dk_nrf5340_cpuapp\hci_rpmsg
nrfjprog -f NRF53 --coprocessor CP_NETWORK --eraseall
nrfjprog -f NRF53 --coprocessor CP_NETWORK --program build_nrf5340dk_nrf5340_cpuapp/hci_rpmsg/zephyr/merged_CPUNET.hex --verify

nrfjprog -f NRF53 --eraseall
nrfjprog -f NRF53 --program build_nrf5340dk_nrf5340_cpuapp/zephyr/merged.hex --verify

nrfjprog -r
pause