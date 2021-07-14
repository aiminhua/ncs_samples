to compile the example, follow the steps below
1. before compiling this example, please prepare the envinronment according to nrf53_ble/README.pdf
2. unzip nrfDFU_rpc_inFlash.rar and ble_netcore.rar to folder nrf53_ble. the folder structure should be nrf53_ble\nrfDFU_rpc_inFlash and nrf53_ble\ble_netcore after the operation
3. unzip subsys.rar and overwrite the same files under nrf\subsys. you will get nrf\subsys\nrf_dfu after the operation
4. enter folder nrf53_ble\nrfDFU_rpc_inFlash and build project. you will get both application core and netcore images.
5. double click program_53.bat to flash both app and net core
6. copy app_signed.hex and net_core_app_signed.hex from nrf53_ble\nrfDFU_rpc_inFlash\build_nrf5340dk_nrf5340_cpuapp\zephyr to nrf53_ble\nrfDFU_rpc_inFlash\update_zip. double click zip_generate.bat to generate the update zip files
7. if you want to update net core image, use 53_net_update_ext.zip. if you want to update app core image, use 53_app_update_ext.zip
8. do the DFU steps as nRF5 SDK does