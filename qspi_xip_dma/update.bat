::::arm-none-eabi-objcopy --input-target=ihex --output-target=ihex --change-address 0x230000 --gap-fill=0xff  build/zephyr/xip_signed.hex build/zephyr/xip_moved_test_update.hex
::python C:/Nordic/NCS/Master/bootloader/mcuboot/scripts/imgtool.py sign --key C:/Nordic/NCS/Master/bootloader/mcuboot/root-ec-p256.pem --header-size 0x200 --align 4 --version 0.0.0+0 --pad-header --slot-size 0x100000 build/zephyr/xip.bin build/zephyr/xip_signed.bin
::arm-none-eabi-objcopy --input-target=binary --output-target=ihex --change-address 0x10230000 --gap-fill=0xff  build/zephyr/xip_signed.bin build/zephyr/xip_moved_test_update.hex

::mergehex --merge build/zephyr/xip_moved_test_update.hex build/zephyr/app_moved_test_update.hex --output build/zephyr/merged_update.hex
mergehex --merge build/zephyr/xip_moved_test_update.hex build/zephyr/net_core_app_moved_test_update.hex build/zephyr/app_moved_test_update.hex --output build/zephyr/merged_update.hex

::nrfjprog --halt
nrfjprog --program build/zephyr/merged_update.hex --qspisectorerase
nrfjprog --debugreset
pause