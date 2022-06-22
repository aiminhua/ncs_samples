::python C:/Nordic/NCS/Master/bootloader/mcuboot/scripts/imgtool.py sign --key root-rsa-2048.pem --header-size 0x200 --align 4 --version 0.0.0+0 --pad-header --slot-size 0xf0000 rtthread.bin app_signed.bin
python C:/Nordic/NCS/Master/bootloader/mcuboot/scripts/imgtool.py sign --key root-ec-p256.pem --header-size 0x200 --align 4 --version 0.0.0+0 --pad-header --slot-size 0xf0000 rtthread.hex app_signed.hex
::arm-none-eabi-objcopy --input-target=binary --output-target=ihex --change-address 0xC000 --gap-fill=0xff  app_signed.bin app_signed.hex
python C:/Nordic/NCS/Master/zephyr/scripts/mergehex.py -o merged.hex mcuboot.hex app_signed.hex

@REM nrfjprog -f NRF53 --coprocessor CP_NETWORK --recover
@REM nrfjprog -f NRF53 --recover

nrfjprog -f NRF53 --coprocessor CP_NETWORK --eraseall
nrfjprog -f NRF53 --coprocessor CP_NETWORK --program merged_CPUNET.hex --verify

nrfjprog -f NRF53 --eraseall
nrfjprog -f NRF53 --program merged.hex --verify

nrfjprog -r

pause