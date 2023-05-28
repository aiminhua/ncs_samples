::python C:/Nordic/NCS/Master/bootloader/mcuboot/scripts/imgtool.py sign --key C:/Nordic/NCS/Master/bootloader/mcuboot/root-ec-p256.pem --header-size 0x200 --align 4 --version 0.0.0+0 --pad-header --slot-size 0x100000 build/zephyr/xip.hex build/zephyr/xip_signed.hex
::::python C:/Nordic/NCS/Master/bootloader/mcuboot/scripts/imgtool.py sign --key C:/Nordic/NCS/Master/bootloader/mcuboot/root-ec-p256.pem --header-size 0x200 --align 4 --version 0.0.0+0 --pad-header --slot-size 0x100000 build/zephyr/xip.bin build/zephyr/xip_signed.bin
::::arm-none-eabi-objcopy --input-target=binary --output-target=ihex --change-address 0x10000000 --gap-fill=0xff  build/zephyr/xip_signed.bin build/zephyr/xip_signed.hex

::mergehex --merge build/zephyr/xip_signed.hex build/zephyr/merged.hex --output build/zephyr/merged.hex
nrfjprog -f NRF53 --coprocessor CP_NETWORK --recover
nrfjprog --recover
nrfjprog --coprocessor CP_NETWORK --eraseall
nrfjprog --program build/hci_rpmsg/zephyr/merged_CPUNET.hex --verify
nrfjprog --eraseall
nrfjprog --program build/zephyr/merged.hex --verify
nrfjprog --program build/zephyr/xip_signed.hex --qspisectorerase --verify
 
nrfjprog --debugreset
::mergehex --merge build/zephyr/xip.hex build/zephyr/merged.hex --output build/zephyr/merged.hex

pause