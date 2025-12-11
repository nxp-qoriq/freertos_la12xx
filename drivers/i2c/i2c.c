// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2022 NXP
 */

#include "FreeRTOS.h"
#include "task.h"
#include "i2cAPI.h"
#include "i2c.h"
#include "io.h"
#include "sync.h"
#include "Time.h"

#ifdef NXP_ERRATUM_A010650
    #include "gpio.h"
#endif

/* Clock rate and Frequency Divider Register settings */
const uint16_t usFsl_I2C_Speed_Map[] =
{
    20,    22,   24,   26,   28,   30,   34,   40,   28,   32,   36,    40,   44,   48,  56,  68,
    48,    56,   64,   72,   80,   88,   104,  128,  80,   96,   112,   128,  144,  160, 192,
    240,   160,  192,  224,  256,  288,  320,  384,  480,  320,  384,   448,  512,  576,
    640,   768,  960,  640,  768,  896,  1024, 1152, 1280, 1536, 1920,  1280,
    1536,  1792, 2048, 2304, 2560, 3072, 3840, 40,   44,   48,   52,    56,   60,   68,
    80,    56,   64,   72,   80,   88,   96,   112,  136,  96,   112,   128,  144,  160, 176, 208,
    256,   160,  192,  224,  256,  288,  320,  384,  480,  320,  384,   448,  512,  576,
    640,   768,  960,  640,  768,  896,  1024, 1152, 1280, 1536, 1920,  1280, 1536,
    1792,  2048, 2304, 2560, 3072, 3840, 2560, 3072, 3584, 4096, 4608,  5120,
    6144,  7680, 80,   88,   96,   104,  112,  120,  136,  160,  112,   128,  144,  160,
    176,   192,  224,  272,  192,  224,  256,  288,  320,  352,  416,   512,  320,  384,
    448,   512,  576,  640,  768,  960,  640,  768,  896,  1024, 1152,  1280, 1536,
    1920,  1280, 1536, 1792, 2048, 2304, 2560, 3072, 3840, 2560, 3072,  3584,
    4096,  4608, 5120, 6144, 7680, 5120, 6144, 7168, 8192, 9216, 10240,
    12288, 15360
};

static void prvSet_I2C_Bus_Speed( i2c_regs_t * i2c_regs,
                                  uint32_t ulSys_Freq,
                                  uint32_t ulI2C_Bus_Freq )
{
    uint8_t ucFdr = 0;

    /* The ucFdr can be set to 0 which would reslut in
     * lowest divider (20) on emulator. The closk is lowered
     * down on emulator and such low divider is sufficient
     * for efficient tetsting on emulator.
     */
    uint32_t ulDivider;

    ulDivider = ulSys_Freq / ulI2C_Bus_Freq;

    /* Find the First Divider value freater than or equal to the
     * required Divider
     */
    for( ucFdr = 0; ucFdr < I2C_MAX_FDR; ucFdr++ )
    {
        if( usFsl_I2C_Speed_Map[ ucFdr ] >= ( uint16_t ) ulDivider )
        {
            break;
        }
    }

    DPRINTF( "ucFdr is::::%d\n\r", ucFdr );
    OUT_8( &i2c_regs->ucI2C_Ibfd, ucFdr ); /* Write divider value */
}

static void prvClear_Status_Flags( i2c_regs_t * i2c_regs,
                                   uint8_t ucFlag )
{
    /* w1c to clear */
    OUT_8( &i2c_regs->ucI2C_Ibsr, ucFlag );
}

static void prvEnable_Digital_Filter( i2c_regs_t * i2c_regs )
{
    /* Enable Digital Filter - In I2C Debug Regiter */
    OUT_8( &i2c_regs->ucI2C_Ibdbg, ( I2C_DBG_GLITCH_EN |
                                     IN_8( &i2c_regs->ucI2C_Ibdbg ) ) );
}

int iI2C_Init( uint32_t ulI2C_Regs_P,
               uint32_t ulSys_Freq,
               uint32_t
               ulI2C_Bus_Freq )
{
    i2c_regs_t * i2c_regs = NULL;
    int timeout = 0;
    i2c_regs = ( i2c_regs_t * ) ulI2C_Regs_P;
    OUT_8( &i2c_regs->ucI2C_Ibcr, ( uint8_t ) ( I2C_CR_MDIS |
                                                ( IN_8( &i2c_regs->ucI2C_Ibcr ) & 0x03 ) ) ); /* Stop I2C controller */

    /* Setting bus speed*/
    prvSet_I2C_Bus_Speed( i2c_regs, ulSys_Freq, ulI2C_Bus_Freq );
    /* Ticket TKT256758 - DesignPDM */
    prvEnable_Digital_Filter( i2c_regs );
    /* Clear status register */
    prvClear_Status_Flags( i2c_regs, I2C_SR_IBIF | I2C_SR_IBAL );
    DPRINTF( "in INIT, value of ibsr is :: %x\n\r", i2c_regs->ucI2C_Ibsr );

    /* Start I2C controller */
    if( IN_8( &i2c_regs->ucI2C_Ibcr ) & I2C_CR_MDIS )
    {
        OUT_8( &i2c_regs->ucI2C_Ibcr, ( I2C_CR_MENA |
                                        ( IN_8( &i2c_regs->ucI2C_Ibcr ) &
                                          0x03 ) ) );
    }

    sync_dmb();

    do
    {
        if( !( IN_8( &i2c_regs->ucI2C_Ibcr ) & I2C_CR_MDIS ) )
        {
            DPRINTF( "in INIT,I2C initialised\n\r" );
            return 1;
        }
        vUdelay( I2C_TIMEOUT_DELAY );
    } while ( ++timeout < I2C_TIMEOUT_COUNT );

    PRINTF( "in INIT,No I2C wakeup\n\r" );
    return -I2C_NO_WAKEUP_INIT;
}

static int prvWait_For_Bus_Free( i2c_regs_t * i2c_regs,
                                 int iCurrent_Bus_Master )
{
    uint8_t ucStatus = 0;
    uint8_t ucFlag = 0;
    int timeout = 0;

    log_dbg( "in WFBF,val of ibsr and ibcr is %x and %x\n\r",
             i2c_regs->ucI2C_Ibsr, i2c_regs->ucI2C_Ibcr );

    do
    {
        ucStatus = IN_8( &i2c_regs->ucI2C_Ibsr );

        if( iCurrent_Bus_Master == IS_CURRENT_MASTER )
        {
            if( ucStatus & I2C_SR_IBAL )
            {
                log_dbg( "in WFBF,IBAL Lost ,val of ibsr anf ibcr is %x and %x\n\r",
                         i2c_regs->ucI2C_Ibsr,
                         i2c_regs->ucI2C_Ibcr );
                ucFlag = ( I2C_SR_IBIF | I2C_SR_IBAL );
                prvClear_Status_Flags( i2c_regs, ucFlag );
            }
        }

        if( !( ucStatus & I2C_SR_IBB_BUSY ) )
        {
            return 1;
        }
        vUdelay( I2C_TIMEOUT_DELAY );
    } while ( ++timeout < I2C_TIMEOUT_COUNT );

    log_dbg( "in WFBF,Bus not IDLE\n\r" );
    return -I2C_NOT_IDLE;
}

static int prvWait_For_Status_Change( i2c_regs_t * i2c_regs,
                                      uint8_t ucState )
{
    uint8_t ucStatus = 0;
    int timeout = 0;

    do
    {
        ucStatus = IN_8( &i2c_regs->ucI2C_Ibsr );

        if( ucStatus & I2C_SR_IBAL )
        {
            prvClear_Status_Flags( i2c_regs, I2C_SR_IBIF |
                                   I2C_SR_IBAL );
            log_dbg( "in WFSC,Arbitration lost with status\n\r" );
            return -I2C_RESTART;

        }

        if( ( ucStatus & ucState ) == ucState )
        {
            return 1;
        }
        vUdelay( I2C_TIMEOUT_DELAY );
    } while ( ++timeout < I2C_TIMEOUT_COUNT );

    log_dbg( "in WFSC,I2C timeout with status\n\r" );
    return -I2C_TIMEOUT;
}

#if NXP_ERRATUM_A010650
    static void prvCheck_SDA_Low( i2c_regs_t * i2c_regs,
                                  uint32_t * pmux_addr,
                                  GpioModule_t eGpioModule,
                                  uint8_t ucPin )
    {
        uint32_t ulPMUXCR = 0;
        int i = 0, count = 640;
        volatile int j;
        int ret = -1;

        count = ( configCPU_CLOCK_HZ * 20 ) / ( 1000000 * 5 );
        ulPMUXCR = IN_32(pmux_addr);
        OUT_32(pmux_addr, (ulPMUXCR | 1 << ucPin));

        for( j = 0; j < count; j++ )
        {
        }

        /*Set SCL pin in Output mode*/
        ret = exGpioInit( eGpioModule, ucPin, GPIO_OUTPUT );
        if( ret != 0 )
        {
            log_err("Error while gpio init to output mode: %d", ret);
            return;
        }

        /*Set SCL pin as open drain*/
        ret = exGpioInit( eGpioModule, ucPin, GPIO_OPENDRAIN );
        if( ret != 0 )
        {
            log_err("Error while gpio init to open drain mode: %d", ret);
            return;
        }

        for( i = 0; i < 9; i++ )
        {
            /*set SCL pin data high*/
            ret = exGpioSetData( eGpioModule, ucPin, 1 );
            if( ret != 0 )
            {
                log_err("Error while gpio set data to 1: %d", ret);
                return;
            }

            for( j = 0; j < count; j++ )
            {
            }

            /*set SCL pin data low*/
            ret = exGpioSetData( eGpioModule, ucPin, 0 );
            if( ret != 0 )
            {
                log_err("Error while gpio set data to 0: %d", ret);
                return;
            }

            for( j = 0; j < count; j++ )
            {
            }
        }

        ret = exGpioSetData( eGpioModule, ucPin, 1 );
        if( ret != 0 )
        {
            log_err("Error while gpio set data to 1: %d", ret);
            return;
        }

        for( j = 0; j < count; j++ )
        {
        }

        /*enable I2C module*/
        OUT_8( &i2c_regs->ucI2C_Ibcr, ( uint8_t ) ( I2C_CR_MENA |
                                                    ( IN_8( &i2c_regs->ucI2C_Ibcr ) &
                                                      0x03 ) ) );

        for( j = 0; j < count; j++ )
        {
        }

        /*disable I2C module*/
        OUT_8( &i2c_regs->ucI2C_Ibcr, ( uint8_t ) ( I2C_CR_MDIS |
                                                    ( IN_8( &i2c_regs->ucI2C_Ibcr ) &
                                                      0x03 ) ) );

        for( j = 0; j < count; j++ )
        {
        }

        /* Storing back value */
        OUT_32( pmux_addr, ulPMUXCR );
    }
#endif /* if NXP_ERRATUM_A010650 */

static int prvTransmit_Byte( i2c_regs_t * i2c_regs,
                             uint8_t ucByte )
{
    int ret = 0;
    uint8_t ucTemp = 0;

    /* Clear interrupt flag */
    prvClear_Status_Flags( i2c_regs, I2C_SR_IBIF );
    OUT_8( &i2c_regs->ucI2C_Ibdr, ucByte );
    ret = prvWait_For_Status_Change( i2c_regs, I2C_SR_IBIF |
                                     I2C_SR_TCF );

    if( ret != 1 )
    {
        return ret;
    }

    ucTemp = IN_8( &i2c_regs->ucI2C_Ibsr );

    if( ucTemp & I2C_SR_NO_RXAK )
    {
        prvClear_Status_Flags( i2c_regs, I2C_SR_IBIF );
        log_dbg( "in TB,NO slave ACK\n\r" );
        return -I2C_NOACK;
    }

    return 1;
}

static void prvStop_I2C_Cntrl( i2c_regs_t * i2c_regs )
{
    int ret = 0;
    uint8_t ucTemp = 0;

    ucTemp = IN_8( &i2c_regs->ucI2C_Ibcr );
    ucTemp &= ( uint8_t ) ( ~( I2C_CR_MSSL | I2C_CR_TXRX ) );

    log_dbg( "in SIC ,val of ibsr and ibcr is %x and %x\n\r",
             i2c_regs->ucI2C_Ibsr, i2c_regs->ucI2C_Ibcr );
    OUT_8( &i2c_regs->ucI2C_Ibcr, ucTemp );
    ret = prvWait_For_Bus_Free( i2c_regs, IS_CURRENT_MASTER );

    if( ret != 1 )
    {
        log_dbg( "in SIC,STOP failed\n\r" );
    }
}

static uint32_t prvWrite_Data( i2c_regs_t * i2c_regs,
                               uint8_t * psrc,
                               uint32_t
                               ulD_Len )
{
    int ret = 0;
    uint8_t ucTemp = 0;
    uint32_t i = 0;

    for( i = 0; i < ulD_Len; i++ )
    {
        /* Clear interrupt flag */
        prvClear_Status_Flags( i2c_regs, I2C_SR_IBIF );
        OUT_8( &i2c_regs->ucI2C_Ibdr, psrc[ i ] );
        ret = prvWait_For_Status_Change( i2c_regs, I2C_SR_IBIF |
                                         I2C_SR_TCF );

        if( ret != 1 )
        {
            log_dbg( "in WD, only %u bytes wrote\n\r", i );
            return i;
        }

        ucTemp = IN_8( &i2c_regs->ucI2C_Ibsr );

        if( ucTemp & I2C_SR_NO_RXAK )
        {
            prvClear_Status_Flags( i2c_regs, I2C_SR_IBIF );
            log_dbg( "in WD,No Ack from Slave,Thus only %u bytes\
				 wrote\n\r", i );
            return i;
        }
    }

    return i;
}



static uint32_t prvRead_Data( i2c_regs_t * i2c_regs,
                              uint8_t * pdes,
                              uint32_t
                              ulSize )
{
    uint8_t ucTemp = 0;
    uint32_t i = 0;
    int ret = 0;
    uint8_t ucFlag = 0;

    /* Prepare bus to Read data */
    ucTemp = IN_8( &i2c_regs->ucI2C_Ibcr );
    ucTemp &= ( uint8_t ) ( ~( I2C_CR_TXRX | I2C_CR_NOACK ) );

    if( ulSize == 1 )
    {
        ucTemp |= I2C_CR_NOACK;
    }

    OUT_8( &i2c_regs->ucI2C_Ibcr, ucTemp );

    /* Clear interrupt flag */
    prvClear_Status_Flags( i2c_regs, I2C_SR_IBIF );
    IN_8( &i2c_regs->ucI2C_Ibdr ); /* Dummy read */

    for( i = 0; i < ulSize; i++ )
    {
        ucFlag = I2C_SR_IBIF | I2C_SR_TCF;
        ret = prvWait_For_Status_Change( i2c_regs, ucFlag );

        if( ret != 1 )
        {
            log_dbg( "in RD,Only %u bytes read\n\r", i );
            return i;
        }

        if( i == ( ulSize - 1 ) )
        {
            /* Send STOP to controller */
            prvStop_I2C_Cntrl( i2c_regs );
        }
        else if( i == ( ulSize - 2 ) )
        {
            ucTemp = IN_8( &i2c_regs->ucI2C_Ibcr );
            ucTemp |= I2C_CR_NOACK;
            OUT_8( &i2c_regs->ucI2C_Ibcr, ucTemp );
        }

        /* Not really need to clear flag. Already cleared */
        prvClear_Status_Flags( i2c_regs, I2C_SR_IBIF );
        pdes[ i ] = IN_8( &i2c_regs->ucI2C_Ibdr );
    }

    return i;
}

static int prvAttempt_Send_Addr( i2c_regs_t * i2c_regs,
                                 uint8_t ucDev_Addr,
                                 uint32_t ulDev_Offset,
                                 uint8_t ucDev_Offset_Len,
                                 uint8_t ucRead_Write )
{
    uint8_t ucTemp = 0;
    int ret = 0;
    uint8_t ucControl = 0;
    int timeout = 0;

    ucControl = IN_8( &i2c_regs->ucI2C_Ibcr );
    log_dbg( "in ASA,value of ibsr and ibcr is %x and %x\n\r",
             i2c_regs->ucI2C_Ibsr, ucControl );

    /* Enable I2C controller */
    if( ucControl & I2C_CR_MDIS )
    {
        ucControl = ( uint8_t ) ( ( ucControl | I2C_CR_NOACK ) & ~I2C_CR_MDIS );
        OUT_8( &i2c_regs->ucI2C_Ibcr, ucControl );
    }

    do
    {
        if( !( IN_8( &i2c_regs->ucI2C_Ibcr ) & I2C_CR_MDIS ) )
        {
            break;
        }
        vUdelay( I2C_TIMEOUT_DELAY );
    } while( ++timeout < I2C_TIMEOUT_COUNT );

    if( IN_8( &i2c_regs->ucI2C_Ibcr ) & I2C_CR_MDIS )
    {
        log_dbg( "in ASA,I2C No wakeup\n\r" );
        return -I2C_NO_WAKEUP_READ;
    }

    /* This is to check whether the SLAVE addr is same as
     * the address in i2c_ibad register
     */
    if( IN_8( &i2c_regs->ucI2C_Ibad ) == ( uint8_t ) ( ucDev_Addr << 1 ) )
    {
        log_dbg( "in ASA,ibad and device address matches,ibad is %x\n\r",
                 i2c_regs->ucI2C_Ibad );
        OUT_8( &i2c_regs->ucI2C_Ibad, ( I2C_RESERVED_ID << 1 ) );
    }

    /* Clear All flags */
    prvClear_Status_Flags( i2c_regs, I2C_SR_IBIF | I2C_SR_IBAL );
    ret = prvWait_For_Bus_Free( i2c_regs, ISNOT_CURRENT_MASTER );

    if( ret != 1 )
    {
        #if NXP_ERRATUM_A010650
            int j = 0;
            u8 ucPin = -1;
            GpioModule_t gpio = GPIO_1;
            uint32_t * pmux_addr = (uint32_t *) (PMUX2CR_BASE_ADDR_IIC);

            if( i2c_regs == ( i2c_regs_t * ) I2C1_BASE_ADDR )
            {
                ucPin = I2C1_IIC1_GPIO_SCL;
            }
            else if( i2c_regs == ( i2c_regs_t * ) I2C2_BASE_ADDR )
            {
                ucPin = I2C2_IIC2_GPIO_SCL;
            }
			else if(i2c_regs == ( i2c_regs_t * ) I2C3_BASE_ADDR )
			{
				ucPin = I2C3_IIC3_GPIO_SCL;
			}
			else if(i2c_regs == ( i2c_regs_t * ) I2C4_BASE_ADDR )
			{
				ucPin = I2C4_IIC4_GPIO_SCL;
				gpio = GPIO_4;
				pmux_addr = (uint32_t *) (PMUX8CR_BASE_ADDR_IIC);
			}
			else if(i2c_regs == ( i2c_regs_t * ) I2C5_BASE_ADDR )
			{
				ucPin = I2C5_IIC5_GPIO_SCL;
				gpio = GPIO_4;
				pmux_addr = (uint32_t *) (PMUX8CR_BASE_ADDR_IIC);
			}
			else if(i2c_regs == ( i2c_regs_t * ) I2C6_BASE_ADDR )
			{
				ucPin = I2C6_IIC6_GPIO_SCL;
				gpio = GPIO_4;
				pmux_addr = (uint32_t *) (PMUX8CR_BASE_ADDR_IIC);
			}

            prvCheck_SDA_Low( i2c_regs, pmux_addr, gpio, ucPin );

            OUT_8( &i2c_regs->ucI2C_Ibcr, ( I2C_CR_MENA |
                                            ( IN_8( &i2c_regs->ucI2C_Ibcr ) &
                                              0x03 ) ) );

            for( j = 0; j < 10000; j++ )
            {
            }

            if( IN_8( &i2c_regs->ucI2C_Ibcr ) & I2C_CR_MDIS )
            {
                log_dbg( "in ASA,I2C No wakeup\n\r" );
                return -I2C_NO_WAKEUP_READ;
            }

            prvClear_Status_Flags( i2c_regs, I2C_SR_IBAL );
            ret = prvWait_For_Bus_Free( i2c_regs, ISNOT_CURRENT_MASTER );
        #endif /* if NXP_ERRATUM_A010650 */

        if( ret != 1 )
        {
            return ret;
        }
    }

    /* Set MASTER mode */

    ucTemp = IN_8( &i2c_regs->ucI2C_Ibcr );
    ucTemp |= I2C_CR_MSSL;
    OUT_8( &i2c_regs->ucI2C_Ibcr, ucTemp );
    log_dbg( "in ASA,after master set ,val of ibsr ad ibcr is %x and %x\n\r",
             i2c_regs->ucI2C_Ibsr, i2c_regs->ucI2C_Ibcr );
    sync_dmb();

    ret = prvWait_For_Status_Change( i2c_regs, I2C_SR_IBB_BUSY );

    log_dbg( "in ASA,after WFSC ,val of ibsr and ibcr is %x and %x\n\r",
             i2c_regs->ucI2C_Ibsr, i2c_regs->ucI2C_Ibcr );

    if( ret != 1 )
    {
        log_dbg( "in ASA,Bus not BUSY\n\r" );
        return -I2C_NOT_BUSY;
    }

    /* Set transmit mode */
    ucTemp = IN_8( &i2c_regs->ucI2C_Ibcr );
    ucTemp |= I2C_CR_TXRX | I2C_CR_NOACK;
    OUT_8( &i2c_regs->ucI2C_Ibcr, ucTemp );

    /* Write Slave address */
    timeout = 0;
    do
    {
        if( ( ucRead_Write == 1 ) && ( ucDev_Offset_Len == 0 ) )
        {
            ret = prvTransmit_Byte( i2c_regs, ( uint8_t ) ( ( ucDev_Addr << 1 ) |
                                                            0x1 ) );
        }
        else
        {
            ret = prvTransmit_Byte( i2c_regs, ( uint8_t ) ( ucDev_Addr << 1 ) );
        }

        if( ret == 1 )
        {
            break;
        }
        else
        {
            ucTemp = IN_8( &i2c_regs->ucI2C_Ibcr );
            ucTemp |= I2C_CR_RSTA;
            OUT_8( &i2c_regs->ucI2C_Ibcr, ucTemp );
        }
        vUdelay( I2C_TIMEOUT_DELAY );
    } while ( ++timeout < I2C_TIMEOUT_COUNT );

    if( ret != 1 )
    {
        if( ret == -I2C_NOACK )
        {
            log_dbg( "in ASA,No slave ACK for SLAVE ID\n\r" );
            return -I2C_NODEV;
        }
        else if( ret == -I2C_TIMEOUT )
        {
            return -I2C_SLAVE_ADDR_TIMEOUT;
        }
        else
        {
            return ret;
        }
    }

    /* Transmit source offset */
    while( ucDev_Offset_Len-- )
    {
        ret = prvTransmit_Byte( i2c_regs,
                                ( ulDev_Offset >> ( ucDev_Offset_Len * 8 ) ) &
                                0xff );

        if( ret != 1 )
        {
            if( ret == -I2C_TIMEOUT )
            {
                return -I2C_MEM_ADDR_TIMEOUT;
            }
            else
            {
                return ret;
            }
        }
    }

    return 1;
}

static int prvInit_Source_Addr( i2c_regs_t * i2c_regs,
                                uint8_t ucDev_Addr,
                                uint32_t ulDev_Offset,
                                uint8_t ucDev_Offset_Len,
                                uint8_t ucRead_Write )
{
    int ret = -I2C_TIMEOUT;
    int retry = 0;

    /* Changed from 2 to 5 on the basis of code from Metanoia */
    for( retry = 0; retry < 5; retry++ )
    {
        ret = prvAttempt_Send_Addr( i2c_regs, ucDev_Addr, ulDev_Offset,
                                    ucDev_Offset_Len, ucRead_Write );

        if( ret == 1 )
        {
            log_dbg( "ISA,src_offset/alen init done\n\r" );
            return 1;
        }

        prvStop_I2C_Cntrl( i2c_regs );

        if( ret == -I2C_NODEV )
        {
            return ret;
        }

        log_dbg( "in ASA,Retry failed on iteration and value is %d\n\r",
                 retry );

        /* Disabling Module
         * And waiting for it to be disabled
         */
        OUT_8( &i2c_regs->ucI2C_Ibcr, ( uint8_t ) ( I2C_CR_MDIS |
                                                    ( IN_8( &i2c_regs->ucI2C_Ibcr ) &
                                                      0x03 ) ) );
        log_dbg( "in ISA,after disabling ,val of ibsr and ibcr is %x and %x\n\r",
                 i2c_regs->ucI2C_Ibsr, i2c_regs->ucI2C_Ibcr );
    }

    log_dbg( "Failed to initialize transfer\n\r" );
    return ret;
}


static int prvCheck_Offset( uint32_t ulDev_Offset,
                            uint8_t ucDev_Offset_Len,
                            uint32_t ulD_Len )
{
    int ovf_cond1, ovf_cond2, ovf_cond3;

    if( !( ( ucDev_Offset_Len == I2C_DEV_OFFSET_LEN_0_BYTE ) ||
           ( ucDev_Offset_Len == I2C_DEV_OFFSET_LEN_1_BYTE ) ||
           ( ucDev_Offset_Len == I2C_DEV_OFFSET_LEN_2_BYTE ) ||
           ( ucDev_Offset_Len == I2C_DEV_OFFSET_LEN_3_BYTE ) ||
           ( ucDev_Offset_Len == I2C_DEV_OFFSET_LEN_4_BYTE ) ) )
    {
        log_dbg( "in CO,Invalid offset\n\r" );
        return -I2C_INVALID_OFFSET;
    }

    switch( ucDev_Offset_Len )
    {
        case ( I2C_DEV_OFFSET_LEN_1_BYTE ):
            ovf_cond1 = ( ulDev_Offset > I2C_MAX_SRC_OFFSET_1_BYTES );
            ovf_cond2 = ( ( ulDev_Offset + ulD_Len - 1 ) >
                          I2C_MAX_SRC_OFFSET_1_BYTES );
            ovf_cond3 = ( ulD_Len > I2C_MAX_READ_SIZE_1_BYTES );

            if( ovf_cond1 || ovf_cond2 || ovf_cond3 )
            {
                log_dbg( "in CO,Offset greater than 8-bit or invalid\
				 data length\n\r" );
                return -I2C_INVALID_OFFSET;
            }

            break;

        case ( I2C_DEV_OFFSET_LEN_2_BYTE ):
            ovf_cond1 = ( ulDev_Offset > I2C_MAX_SRC_OFFSET_2_BYTES );
            ovf_cond2 = ( ( ulDev_Offset + ulD_Len - 1 ) >
                          I2C_MAX_SRC_OFFSET_2_BYTES );
            ovf_cond3 = ( ulD_Len > I2C_MAX_READ_SIZE_2_BYTES );

            if( ovf_cond1 || ovf_cond2 || ovf_cond3 )
            {
                log_dbg( "in CO,Offset greater than 16-bit or invalid\
				 data length\n\r" );
                return -I2C_INVALID_OFFSET;
            }

            break;

        case ( I2C_DEV_OFFSET_LEN_3_BYTE ):
            ovf_cond1 = ( ulDev_Offset > I2C_MAX_SRC_OFFSET_3_BYTES );
            ovf_cond2 = ( ( ulDev_Offset + ulD_Len - 1 ) >
                          I2C_MAX_SRC_OFFSET_3_BYTES );
            ovf_cond3 = ( ulD_Len > I2C_MAX_READ_SIZE_3_BYTES );

            if( ovf_cond1 || ovf_cond2 || ovf_cond3 )
            {
                log_dbg( "in CO,Offset greater than 24-bit or invalid\
				 data length\n\r" );
                return -I2C_INVALID_OFFSET;
            }

            break;

        case ( I2C_DEV_OFFSET_LEN_4_BYTE ):
            ovf_cond1 = ( ulDev_Offset > I2C_MAX_SRC_OFFSET_4_BYTES );
            ovf_cond2 = ( ( ulDev_Offset + ulD_Len - 1 ) >
                          I2C_MAX_SRC_OFFSET_4_BYTES );
            ovf_cond3 = ( ulD_Len > I2C_MAX_READ_SIZE_4_BYTES );

            if( ovf_cond1 || ovf_cond2 || ovf_cond3 )
            {
                log_dbg( "in CO,Offset greater than 32-bit or invalid\
				 data length\n\r" );
                return -I2C_INVALID_OFFSET;
            }

            break;

        default:
            log_dbg( "in CO,valid offset\n\r" );
    }

    return 1;
}

int iI2C_Read( uint32_t ulI2C_Regs_P,
               uint8_t ucDev_Addr,
               uint32_t
               ulDev_Offset,
               uint8_t ucDev_Offset_Len,
               uint8_t * pdst,
               uint32_t ulD_Len )
{
    i2c_regs_t * i2c_regs = NULL;
    int ret = 0;
    uint8_t ucTemp = 0;
    uint32_t ulRet_Bytes = 0;

    i2c_regs = ( i2c_regs_t * ) ulI2C_Regs_P;
    ret = prvCheck_Offset( ulDev_Offset, ucDev_Offset_Len, ulD_Len );

    if( ret != 1 )
    {
        return ret;
    }

    ret = prvInit_Source_Addr( i2c_regs, ucDev_Addr, ulDev_Offset,
                               ucDev_Offset_Len, 1 );

    if( ret != 1 )
    {
        return ret;
    }

    /* Repeat START */
    if( ucDev_Offset_Len != 0 )
    {
        ucTemp = IN_8( &i2c_regs->ucI2C_Ibcr );
        ucTemp |= I2C_CR_RSTA;
        OUT_8( &i2c_regs->ucI2C_Ibcr, ucTemp );

        /* Set READ bit for read operation */
        ret = prvTransmit_Byte( i2c_regs, ( uint8_t ) ( ( ucDev_Addr << 1 ) | 0x1 ) );

        if( ret != 1 )
        {
            /* Send STOP to controller */
            prvStop_I2C_Cntrl( i2c_regs );
            return ret;
        }
    }

    ulRet_Bytes = prvRead_Data( i2c_regs, pdst, ulD_Len );

    if( ulRet_Bytes != ulD_Len )
    {
        log_dbg( "in RIR,only %u bytes read\n\r", ulRet_Bytes );
        prvStop_I2C_Cntrl( i2c_regs );
        return ( int ) ulRet_Bytes;
    }

    prvStop_I2C_Cntrl( i2c_regs );
    log_dbg( "in RIR,%u bytes read successfully\n\r", ulD_Len );
    return ( int ) ulRet_Bytes;
}

int iI2C_Write( uint32_t ulI2C_Regs_P,
                uint8_t ucDev_Addr,
                uint32_t
                ulDev_Offset,
                uint8_t ucDev_Offset_Len,
                uint8_t * psrc,
                uint32_t ulD_Len )
{
    i2c_regs_t * i2c_regs = NULL;
    int ret = 0;
    uint32_t ulRet_Bytes = 0;

    i2c_regs = ( i2c_regs_t * ) ulI2C_Regs_P;
    ret = prvCheck_Offset( ulDev_Offset, ucDev_Offset_Len, ulD_Len );

    if( ret != 1 )
    {
        return ret;
    }

    ret = prvInit_Source_Addr( i2c_regs, ucDev_Addr, ulDev_Offset,
                               ucDev_Offset_Len, 0 );

    if( ret != 1 )
    {
        return ret;
    }

    ulRet_Bytes = prvWrite_Data( i2c_regs, psrc, ulD_Len );

    if( ulRet_Bytes != ulD_Len )
    {
        log_dbg( "in RIW,only %u bytes transferred\n\r", ulRet_Bytes );
        prvStop_I2C_Cntrl( i2c_regs );
        return ( int ) ulRet_Bytes;
    }

    prvStop_I2C_Cntrl( i2c_regs );
    log_dbg( "in RIW,%u bytes wrote successfully\n\r", ulD_Len );
    return ( int ) ulRet_Bytes;
}

/* XXX: Removed the iI2C_Disable() which disables the I2C environment
 * for application.
 */
