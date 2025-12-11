// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#include "watchdog.h"

uint32_t iLoadValue;
int ( * L1_ref_ptr )() = NULL;

uint8_t wdog_status (void)
{
	if(watchdog_in32(PCTBEN_REG) & 1)
		return 0;
	else
		return 1;
}

void vWatchdogStart( uint32_t xWdogLoadValue,
                     int ( * L1C_callback_func )( void ) )
{
    /* TBD: Consider changing loadvalue to time in ms */
    iLoadValue = xWdogLoadValue;
    L1_ref_ptr = L1C_callback_func;

    log_dbg( "%s:\n\r", __func__ );

    /* Enable Physical Core TimeBase Enable  */
    watchdog_out32( PCTBEN_REG, 1 | watchdog_in32( PCTBEN_REG ) );

    /* UnMask WDOGRES */
    CLEAR_BIT( SCFG_CONFIG_CONTROL0_WDOG, SCFG_WDOGTOUT_RES );
    CLEAR_BIT( SCFG_CONFIG_CONTROL0_WDOG, SCFG_WDOGTOUT_RES_CORE0 );

    /* Register WDOG IRQ & ISR Handler for watchdog */
    lRegisterIrq( WDOG_IRQ_NUM + INTERNAL_IRQ_OFFSET, xWatchdogIrqHandler, NULL );
    bMpicEnable( DEVICE_INTERNAL, WDOG_IRQ_NUM );

    /* WDOG Register Unlock */
    watchdog_out32( WDOG_LOCK_REG, WDOG_UNLOCK );

    watchdog_out32( WDOG_LOAD_REG, iLoadValue );
    watchdog_out32( WDOG_CONTROL_REG, WDOG_CONTROL_RESEN | WDOG_CONTROL_INTEN |
                    watchdog_in32( ( uint32_t * ) WDOG_CONTROL_REG ) );
    /* WDOG Register Lock */
    watchdog_out32( WDOG_LOCK_REG, WDOG_LOCK );
    log_info( "Wdog STARTED !\n\r" );
}

void vWatchdogStop( void )
{
    /* WDOG Register Unlock */
    watchdog_out32( WDOG_LOCK_REG, WDOG_UNLOCK );
    watchdog_out32( WDOG_CONTROL_REG, 0xfffffffc &
                    watchdog_in32( ( ( uint32_t * ) WDOG_CONTROL_REG ) ) );
    /* WDOG Register Lock */
    watchdog_out32( WDOG_LOCK_REG, WDOG_LOCK );

    bMpicDisable( DEVICE_INTERNAL, WDOG_IRQ_NUM );
    vUnregisterIrq( WDOG_IRQ_NUM + INTERNAL_IRQ_OFFSET );

    log_info( "%s: Wdog STOPPED\n\r", __func__ );
}

void vWatchdogReload( uint32_t xWdogReloadValue,
                      int ( * new_L1C_callback_func )( void ) )
{
    iLoadValue = xWdogReloadValue;
    L1_ref_ptr = new_L1C_callback_func;

    /* WDOG Register Unlock */
    watchdog_out32( WDOG_LOCK_REG, WDOG_UNLOCK );
    watchdog_out32( WDOG_INTCLR_REG, 0x1 |
                    watchdog_in32( ( uint32_t * ) WDOG_INTCLR_REG ) );
    watchdog_out32( WDOG_LOAD_REG, xWdogReloadValue );
    watchdog_out32( WDOG_CONTROL_REG, WDOG_CONTROL_INTEN | WDOG_CONTROL_RESEN |
                    watchdog_in32( ( uint32_t * ) WDOG_CONTROL_REG ) );
    /* WDOG Register Lock */
    watchdog_out32( WDOG_LOCK_REG, WDOG_LOCK );

    log_dbg( "%s: Wdog Reloaded with value = 0x%x\n\r", __func__, xWdogReloadValue );
}

bool xWatchdogIrqHandler( uint32_t ulIrq_No  __attribute__((unused)),
                          void * vDev_Data  __attribute__((unused)))
{
    int iCallback_status;

    log_isr( "\n\rWDOG_IRQ_HANDLER: %s(irq = %d), param data = %p \r\n", __func__, ( ulIrq_No - INTERNAL_IRQ_OFFSET ), vDev_Data );

    if( NULL != L1_ref_ptr )
    {
        iCallback_status = L1_ref_ptr();

        if( iCallback_status == 1 )
        {
            log_isr( "WDOG_IRQ_HANDLER: Reloading watchdog\n\r" );

            /* Clear interrupt & reload WDOG */
            vWatchdogReload( iLoadValue, L1_ref_ptr );
            log_isr( "WDOG_IRQ_HANDLER: bye !!, see you later...\n\r\n\r" );
            return 0;
        }
        else
        {
            log_isr( "WDOG_IRQ_HANDLER: PROCESSOR GOING INTO RESET..\n\r" );

            while( 1 )
            {
            }

            return 1;
        }
    }
    else
    {
        log_err( "WDOG_IRQ_HANDLER: XXXX ERROR XXX ISR- No valid Callback function pointer registered\n\r" );
    }

    return 1;
}
