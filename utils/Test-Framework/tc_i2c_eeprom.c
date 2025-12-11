// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2021 NXP
 */

#include "FreeRTOS.h"
#include "immap.h"
#include "i2cAPI.h"
#include "platform_def.h"
#include "test_framework.h"
#include "tc_i2c_eeprom.h"

#if GEUL_DEMO_I2C_TEST

    #ifdef GEUL_LA1246

        #if TEST_COMPLETE_EEPROM_RW
            void vI2CeepromTest( void )
            {
                int i, ret;
                uint32_t offset;
                uint8_t ucval[ 4 ];
                const uint8_t ucdata[ 4 ] = { 0x55, 0xAA, 0x55, 0xAA };

                u32 current_core = ulMpicCurrentCore();

                for( offset = 0; offset < EEPROM_SIZE; offset += 4 )
                {
                    ret = iI2C_Write( I2C2_BASE_ADDR, EEPROM_BASE_ADDRESS, offset,
                                      I2C_DEV_OFFSET_LEN_2_BYTE, ucdata, 4 );

                    if( ret < 0 )
                    {
                        log_info( "EEPROM RW: i2c_write failed %d\n\r", ret );
                        RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
                        return;
                    }

                    log_info( "EEPROM RW: i2c_write:: No of bytes = %d\n\r", ret );

                    ret = iI2C_Read( I2C2_BASE_ADDR, EEPROM_BASE_ADDRESS, offset,
                                     I2C_DEV_OFFSET_LEN_2_BYTE, ucval, 4 );

                    if( ret < 0 )
                    {
                        log_info( "EEPROM RW: i2c_read failed %d\n\r", ret );
                        RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
                        return;
                    }

                    log_info( "EEPROM RW: i2c_read:: No of bytes = %d\n\r", ret );
                    log_info( "Data read from EEPROM:\n\r" );

                    for( i = 0; i < 4; i++ )
                    {
                        if( ucval[ i ] != ucdata[ i ] )
                        {
                            log_info( "Data mismatch at offset: 0x%X, i = %d\n\r", offset, i );
                            log_info( "Expected 0x%X, Read 0x%X\n\r", ucdata[ i ], ucval[ i ] );
                            RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
                            return;
                        }

                        log_info( "0x%X ", ucval[ i ] );
                    }

                    log_info( "\n\r" );
                }

                log_info( "EEPROM RW: I2C Test Case PASSED!!\n\n\r" );
                SET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
            }
        #endif /* TEST_COMPLETE_EEPROM_RW */

        void vI2CeepromTestRead( void )
        {
            int ret, i;
            uint8_t ucdata[ 5 ] = { 11, 22, 33, 44, 55 };
            uint8_t ucval[ 5 ];

            u32 current_core = ulMpicCurrentCore();

            /*write 5 bytes at slave device(EEPROM_BASE_ADDRESS) at 0 offset*/
            ret = iI2C_Write( I2C2_BASE_ADDR, EEPROM_BASE_ADDRESS, 0,
                              I2C_DEV_OFFSET_LEN_2_BYTE, ucdata, 5 );

            if( ret < 0 )
            {
                log_info( "EEPROM TC1: i2c_write failed %d\n\r", ret );
                RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
                return;
            }

            log_info( "EEPROM TC1: i2c_write:: No of bytes = %d\n\r", ret );

            /*read 5 bytes from slave device(EEPROM_BASE_ADDRESS) from 0 offset*/
            ret = iI2C_Read( I2C2_BASE_ADDR, EEPROM_BASE_ADDRESS, 0,
                             I2C_DEV_OFFSET_LEN_2_BYTE, ucval, 5 );

            if( ret < 0 )
            {
                log_info( "EEPROM TC1: i2c_read failed %d\n\r", ret );
                RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
                return;
            }

            log_info( "EEPROM TC1: i2c_read:: No of bytes = %d\n\r", ret );
            log_info( "Data read from EEPROM:\n\r" );

            for( i = 0; i < 5; i++ )
            {
                log_info( "%d ", ucval[ i ] );
            }

            log_info( "\n\r" );

            #if TEST_NEGATIVE_I2C_CASES
                /* write 5 bytes at slave device(EEPROM_BASE_ADDRESS) at 0x22 offset
                 * using 1 byte as address space
                 */
                ret = iI2C_Write( I2C2_BASE_ADDR, EEPROM_BASE_ADDRESS, 0x22,
                                  I2C_DEV_OFFSET_LEN_1_BYTE, ucdata, 5 );

                if( ret < 0 )
                {
                    log_info( "EEPROM TC2: i2c_write failed %d\n\r", ret );
                    RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
                    return;
                }

                log_info( "EEPROM TC2: i2c_write:: No of bytes = %d\n\r", ret );
                log_info( "------------------------\n\r" );

                /*read 4 bytes from slave device(EEPROM_BASE_ADDRESS) from 0x220b offset
                 * using 2 bytes as address space
                 */
                ret = iI2C_Read( I2C2_BASE_ADDR, EEPROM_BASE_ADDRESS, 0x220b,
                                 I2C_DEV_OFFSET_LEN_2_BYTE, ucval, 4 );

                if( ret < 0 )
                {
                    log_info( "EEPROM TC2: i2c_read failed %d\n\r", ret );
                    RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
                    return;
                }

                log_info( "EEPROM TC2: i2c_read:: No of bytes = %d\n\r", ret );
                log_info( "-----------------------\n\r" );

                for( i = 0; i < 4; i++ )
                {
                    log_info( "%d ", ucval[ i ] );
                }

                log_info( "\n\r" );


                /*write 5 bytes at slave device(EEPROM_BASE_ADDRESS) at 0x33 offset
                 * using 3 bytes as address space
                 */
                ret = iI2C_Write( I2C2_BASE_ADDR, EEPROM_BASE_ADDRESS, 0x33,
                                  I2C_DEV_OFFSET_LEN_3_BYTE, ucdata, 5 );

                if( ret < 0 )
                {
                    log_info( "EEPROM TC3: i2c_write failed %d\n\r", ret );
                    RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
                    return;
                }

                log_info( "EEPROM TC3: i2c_write:: No of bytes = %d\n\r", ret );
                log_info( "----------------------\n\r" );

                /*read 5 bytes from slave device(EEPROM_BASE_ADDRESS) at 0x00 offset
                 * using 2 bytes as address space
                 */
                ret = iI2C_Read( I2C2_BASE_ADDR, EEPROM_BASE_ADDRESS, 0x00,
                                 I2C_DEV_OFFSET_LEN_2_BYTE, ucval, 5 );

                if( ret < 0 )
                {
                    log_info( "EEPROM TC3: i2c_read failed %d\n\r", ret );
                    RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
                    return;
                }

                log_info( "EEPROM TC3: i2c_read:: No of bytes = %d\n\r", ret );
                log_info( "--------------------\n\r" );

                for( i = 0; i < 5; i++ )
                {
                    log_info( "%d ", ucval[ i ] );
                }

                log_info( "\n\r" );

                /*write 5 bytes at slave device(EEPROM_BASE_ADDRESS) at 0x228989 offset
                 * using 2 bytess address space
                 */
                ret = iI2C_Write( I2C2_BASE_ADDR, EEPROM_BASE_ADDRESS, 0x228989,
                                  I2C_DEV_OFFSET_LEN_2_BYTE, ucdata, 5 );

                if( ret > 0 )
                {
                    log_info( "EEPROM TC4: i2c_write failed %d\n\r", ret );
                    RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
                    return;
                }

                log_info( "EEPROM TC4: i2c_write:: %d\n\r", ret );
                log_info( "----------------------\n\r" );

                /*read 5 bytes from slave device(EEPROM_BASE_ADDRESS) from 0x228989 offset
                 * using 2 bytes address space
                 */
                ret = iI2C_Read( I2C2_BASE_ADDR, EEPROM_BASE_ADDRESS, 0x228989,
                                 I2C_DEV_OFFSET_LEN_2_BYTE, ucval, 5 );

                if( ret > 0 )
                {
                    log_info( "EEPROM TC4: i2c_read failed %d\n\r", ret );
                    RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
                    return;
                }

                log_info( "EEPROM TC4: i2c_read:: %d\n\r", ret );
                log_info( "--------------------\n\r" );
                log_info( "\n" );

                /*write 5 bytes at slave device(EEPROM_BASE_ADDRESS) at 0x67 offset
                 * using 5 bytes address space
                 */
                ret = iI2C_Write( I2C2_BASE_ADDR, EEPROM_BASE_ADDRESS, 0x67, 5, ucdata, 5 );

                if( ret > 0 )
                {
                    log_info( "EEPROM TC5: i2c_write failed %d\n\r", ret );
                    RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
                    return;
                }

                log_info( "EEPROM TC5: i2c_write:: %d\n\r", ret );
                log_info( "----------------------\n\r" );

                /*read FFFFFF bytes from slave device(EEPROM_BASE_ADDRESS) from 0x67 offset
                 * using 2 bytes address space
                 */
                ret = iI2C_Read( I2C2_BASE_ADDR, EEPROM_BASE_ADDRESS, 0x67,
                                 I2C_DEV_OFFSET_LEN_2_BYTE, ucval, 0xFFFFFF );

                if( ret > 0 )
                {
                    log_info( "EEPROM TC5: i2c_read failed %d\n\r", ret );
                    return;
                }

                log_info( "EEPROM TC5: i2c_read:: %d\n\r", ret );
                log_info( "--------------------\n\r" );
                log_info( "\n\r" );
            #endif //TEST_NEGATIVE_I2C_CASES
            log_info( "EEPROM : I2C Test Case PASSED!!\n\n\r" );

            SET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
        }
    #endif //GEUL_LA1246
#endif //GEUL_DEMO_I2C_TEST
