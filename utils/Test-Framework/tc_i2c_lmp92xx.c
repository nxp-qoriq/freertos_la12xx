// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2025 NXP
 */

#include "FreeRTOS.h"
#include "immap.h"
#include "i2cAPI.h"
#include "platform_def.h"
#include "test_framework.h"
#include "ina220_api.h"
#include "tmu_i2c.h"
#include "Time.h"

void vI2Ctestlmp92xx(void)

{
    unsigned uiDeviceBaseAddress, uiOffSet, uiLen, uiOffLen2;
    uint32_t uiBusBaseAddress;
    uint8_t tempVal, reg;
    int ulRet;
    uiOffLen2 = 1;
    uiLen = 1;

    // Expected values for validation
    uint8_t expectedReg0B = 0x05;
    uint8_t expectedReg0C = 0xAA;
    // Array of I2C base addresses
    uint32_t i2cBases[] = {I2C2_BASE_ADDR, I2C3_BASE_ADDR};
    // Array of I2C slave addresses
    uint8_t slaveAddresses[] = {0x40, 0x3F, 0x42};

    ulRet = iI2C_Init(I2C3_BASE_ADDR, I2C_CLK_FREQ, I2C_FREQ);
    if (ulRet == 1){
	    log_info("I2C3 init successful\n\r");}
    else{
	    log_info("I2C3 init failed\n\r");
	    return;
    }

    // Run for both I2C2 and I2C3
    for (int k = 0; k < 2; k++)
    {
        uiBusBaseAddress = i2cBases[k];  // Set I2C base address
        for (int i = 0; i < 3; i++)
        {
            uiDeviceBaseAddress = slaveAddresses[i];  // Set slave address
            log_info("\n========================\n\r");
            log_info("Testing LMP92066 on I2C%d\n\r", k + 2);
            log_info("Slave Address: 0x%X\n\r", uiDeviceBaseAddress);
            log_info("========================\n\r");
            // Writing sequence
            log_info("Writing: Reg[0x11] = 0xCD (Set Access Level to L2)\n\r");
            reg = 0x11; tempVal = 0xCD;
            iI2C_Write(uiBusBaseAddress, uiDeviceBaseAddress, reg, I2C_DEV_OFFSET_LEN_1_BYTE, &tempVal, 1);
            vUdelay(10);
            log_info("Writing: Reg[0x11] = 0xF0\n\r");
            reg = 0x11; tempVal = 0xF0;
            iI2C_Write(uiBusBaseAddress, uiDeviceBaseAddress, reg, I2C_DEV_OFFSET_LEN_1_BYTE, &tempVal, 1);
            vUdelay(10);
            log_info("Writing: Reg[0x10] = 0xC3 (Configure Power-On Reset Value)\n\r");
            reg = 0x10; tempVal = 0xC3;
            iI2C_Write(uiBusBaseAddress, uiDeviceBaseAddress, reg, I2C_DEV_OFFSET_LEN_1_BYTE, &tempVal, 1);
            vUdelay(10);
            log_info("Writing: Reg[0x11] = 0xCD (Set Access Level to L2 after POR)\n\r");
            reg = 0x11; tempVal = 0xCD;
            iI2C_Write(uiBusBaseAddress, uiDeviceBaseAddress, reg, I2C_DEV_OFFSET_LEN_1_BYTE, &tempVal, 1);
            vUdelay(10);
            log_info("Writing: Reg[0x11] = 0xF0\n\r");
            reg = 0x11; tempVal = 0xF0;
            iI2C_Write(uiBusBaseAddress, uiDeviceBaseAddress, reg, I2C_DEV_OFFSET_LEN_1_BYTE, &tempVal, 1);
            vUdelay(10);
            log_info("Writing: Reg[0x08] = 0x02 (Enable DAC & Temp)\n\r");
            reg = 0x08; tempVal = 0x02;
            iI2C_Write(uiBusBaseAddress, uiDeviceBaseAddress, reg, I2C_DEV_OFFSET_LEN_1_BYTE, &tempVal, 1);
            vUdelay(10);
            log_info("Writing: Reg[0x0B] = 0x%x (writing 0x%x on DAC0M_OVRD)\n\r",expectedReg0B,expectedReg0B);
            reg = 0x0B; tempVal = expectedReg0B;
            iI2C_Write(uiBusBaseAddress, uiDeviceBaseAddress, reg, I2C_DEV_OFFSET_LEN_1_BYTE, &tempVal, 1);
            vUdelay(10);
            log_info("Writing: Reg[0x0C] = 0x%x (writing 0x%x on DAC0L_OVRD)\n\r",expectedReg0C,expectedReg0C);
            reg = 0x0C; tempVal = expectedReg0C;
            iI2C_Write(uiBusBaseAddress, uiDeviceBaseAddress, reg, I2C_DEV_OFFSET_LEN_1_BYTE, &tempVal, 1);
            vUdelay(10);
            log_info("\nReading Back DAC Registers\n\r");
            // Reading sequence
            uiOffSet = 0x0B;
            iI2C_Read(uiBusBaseAddress, uiDeviceBaseAddress, uiOffSet, uiOffLen2, &tempVal, uiLen);
            vUdelay(10);
            log_info("Read: Slave[0x%X], Reg[0x0B] = 0x%X\n\r", uiDeviceBaseAddress, tempVal);
            bool passReg0B = (tempVal == expectedReg0B); // Validate Reg 0B
            uiOffSet = 0x0C;
            iI2C_Read(uiBusBaseAddress, uiDeviceBaseAddress, uiOffSet, uiOffLen2, &tempVal, uiLen);
            vUdelay(10);
            log_info("Read: Slave[0x%X], Reg[0x0C] = 0x%X\n\r", uiDeviceBaseAddress, tempVal);
            bool passReg0C = (tempVal == expectedReg0C); // Validate Reg 0C
            // Pass/Fail Check
            if (passReg0B && passReg0C) {
                log_info("\nTEST PASSED for Slave[0x%X] on I2C%d \n\r", uiDeviceBaseAddress, k + 2);
            } else {
                log_info("\nTEST FAILED for Slave[0x%X] on I2C%d \n\r", uiDeviceBaseAddress, k + 2);
            }
            log_info("\n------------------------\n\r");
        }
    }
}
