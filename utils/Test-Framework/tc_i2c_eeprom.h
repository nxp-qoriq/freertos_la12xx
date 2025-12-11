// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2021 NXP
 */

#ifndef _TEST_CASES_I2C_EEPROM_H_
#define _TEST_CASES_I2C_EEPROM_H_

#include "test_framework.h"

#if GEUL_DEMO_I2C_TEST

    #ifdef GEUL_LA1246
        #define TEST_COMPLETE_EEPROM_RW    0
        #define TEST_NEGATIVE_I2C_CASES    0
        #define EEPROM_SIZE                0x10000
    #endif /* GEUL_LA1246 */

    void vI2CeepromTestRead( void );

    #if TEST_COMPLETE_EEPROM_RW
        void vI2CeepromTest( void );
    #endif /* TEST_COMPLETE_EEPROM_RW */

#endif /* GEUL_DEMO_I2C_TEST */

#endif /* _TEST_CASES_I2C_EEPROM_H_ */
