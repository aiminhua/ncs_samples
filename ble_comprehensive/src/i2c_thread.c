/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME i2c_thread
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

const struct device *i2c_dev;
extern struct k_sem sem_iic_op;

#define EEPROM_SIM_SIZE                   (320u) //!< Simulated EEPROM size.
#define EEPROM_SIM_ADDR                   0x50    //!< Simulated EEPROM TWI slave address.
/* Slave memory addressing byte length */
#define EEPROM_SIM_ADDRESS_LEN_BYTES    2
#define IN_LINE_PRINT_CNT   (16u)   //!< Number of data bytes printed in a single line.


static int eeprom_read(uint16_t addr, uint8_t * pdata, size_t size)
{
    int ret;
    if (size > (EEPROM_SIM_SIZE))
    {
        return EINVAL;
    }
    do
    {
       uint16_t addr16 = addr;
       ret = i2c_write(i2c_dev, (uint8_t *)&addr16, EEPROM_SIM_ADDRESS_LEN_BYTES, EEPROM_SIM_ADDR);
       if (0 != ret)
       {
           break;
       }
       ret = i2c_read(i2c_dev, pdata, size, EEPROM_SIM_ADDR);
    }while (0);
    return ret;
}

static void eeprom_cmd_read(void)
{    
    uint8_t buf[IN_LINE_PRINT_CNT+1];

    for (uint16_t addr = 0; addr < EEPROM_SIM_SIZE; addr += IN_LINE_PRINT_CNT)
    {
        int err_code;
        err_code = eeprom_read(addr, buf, IN_LINE_PRINT_CNT);
        buf[IN_LINE_PRINT_CNT] = '\0';
        if (0 != err_code)
        {
            LOG_ERR("EEPROM transmission error %d\n", err_code);
            return;
        }

        LOG_HEXDUMP_INF(buf, IN_LINE_PRINT_CNT, "EEPROM: ");        
      
    }
}

void iic_thread(void)
{	
	
	LOG_INF("** I2C master example **");
	LOG_INF("This example is ported from nRF5_SDK\\examples\\peripheral\\twi_master_with_twis_slave");
	LOG_INF("The related twis example is from nRF5_SDK\\examples\\peripheral\\twi_master_with_twis_slave");	
	
	i2c_dev = DEVICE_DT_GET(DT_ALIAS(myi2c));
    
	if (!i2c_dev) {
		LOG_ERR("I2C Device driver not found");
		return;
	}
    uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_FAST) | I2C_MODE_CONTROLLER;
    if (i2c_configure(i2c_dev, i2c_cfg)) {
        LOG_ERR("I2C config failed");
        return;
    }
    
	while (1) {       
        k_sem_take(&sem_iic_op, K_FOREVER);         
        LOG_INF("i2c master thread");
        eeprom_cmd_read();
        k_sleep(K_SECONDS(3));
	}
}

K_THREAD_DEFINE(iic_thread_id, 1024, iic_thread, NULL, NULL,
		NULL, 8, 0, 0);