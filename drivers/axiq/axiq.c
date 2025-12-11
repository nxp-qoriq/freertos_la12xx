// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2017-2021 NXP
 */

#include <types.h>
#include <FreeRTOS.h>
#include <task.h>
#include "axiq.h"
#include "common.h"

/******************************************************************************
*************************** 	API Functions 		 **************************
******************************************************************************/
static AxiqLibHndlr_t *pAxiqLibHndlr = NULL;
static uint32_t AxiqIntLines[NUM_AXIQ_INTERRUPTS] = {40, 36, 37};

void vEnableAxiqHSubsystem2( AxiqLibHndlr_t * pxAxiqLib )
{
	volatile DcfgDeviceDisableReg3_t * pxDddr3 = ( DcfgDeviceDisableReg3_t * ) DEVDISR3_BASE;
	volatile uint32_t * puiRegAddr = ( uint32_t * ) pxDddr3;

	/* pxDddr3->PHY_SUBSYSTEM2 = SUBSYSTEM_ENABLE */
	clrbits_le32( puiRegAddr, ( uint32 )( 0x1 << ( uint32 ) DCFG_DEVICE_DISABLE_REG3_BIT_PHY_SUBSYSTEM2 ) );
	pxAxiqLib->eSubsystemState[ AXIQ_H ] = SUBSYSTEM_ENABLE;

	log_dbg("\n\r[AXIQ] After Enabling AXIQ_H : DDR3 = 0x%x State %d",
			in_le32( puiRegAddr ), pxAxiqLib->eSubsystemState[ AXIQ_H ] );

	return;
}

void vDisableAxiqHSubsystem2( AxiqLibHndlr_t * pxAxiqLib )
{
	volatile DcfgDeviceDisableReg3_t * pxDddr3 = ( DcfgDeviceDisableReg3_t * ) DEVDISR3_BASE;
	volatile uint32_t * puiRegAddr = ( uint32_t * ) pxDddr3;

	/* pxDddr3->PHY_SUBSYSTEM2 = SUBSYSTEM_DISABLE */
	setbits_le32( puiRegAddr, ( uint32 )( 0x1 << ( uint32 ) DCFG_DEVICE_DISABLE_REG3_BIT_PHY_SUBSYSTEM2 ) );
	pxAxiqLib->eSubsystemState[ AXIQ_H ] = SUBSYSTEM_DISABLE;

	log_dbg("\n\r[AXIQ] After Disabling AXIQ_H : DDR3 = 0x%x State %d",
				in_le32( puiRegAddr ), pxAxiqLib->eSubsystemState[ AXIQ_H ] );

	return;
}

void vEnableAxiqLSubsystem1( AxiqLibHndlr_t * pxAxiqLib )
{
	volatile DcfgDeviceDisableReg3_t * pxDddr3 = ( DcfgDeviceDisableReg3_t * ) DEVDISR3_BASE;
	volatile uint32_t * puiRegAddr = ( uint32_t * ) pxDddr3;

	/* pxDddr3->PHY_SUBSYSTEM1 = SUBSYSTEM_ENABLE */
	clrbits_le32( puiRegAddr, ( uint32 )( 0x1 << ( uint32 ) DCFG_DEVICE_DISABLE_REG3_BIT_PHY_SUBSYSTEM1 ) );
	pxAxiqLib->eSubsystemState[ AXIQ_L0 ] = SUBSYSTEM_ENABLE;

	log_dbg("\n\r[AXIQ] After Enabling AXIQ_L : DDR3 = 0x%x State %d",
	in_le32( puiRegAddr ), pxAxiqLib->eSubsystemState[ AXIQ_L0 ] );

	return;
}

void vDisableAxiqLSubsystem1( AxiqLibHndlr_t * pxAxiqLib )
{
	volatile DcfgDeviceDisableReg3_t * pxDddr3 = ( DcfgDeviceDisableReg3_t * ) DEVDISR3_BASE;
	volatile uint32_t * puiRegAddr = ( uint32_t * ) pxDddr3;

	/* pxDddr3->PHY_SUBSYSTEM1 = SUBSYSTEM_DISABLE */
	setbits_le32( puiRegAddr, ( uint32 )( 0x1 << ( uint32 ) DCFG_DEVICE_DISABLE_REG3_BIT_PHY_SUBSYSTEM1 ) );
	pxAxiqLib->eSubsystemState[ AXIQ_L0 ] = SUBSYSTEM_DISABLE;

	log_dbg( "\n\r[AXIQ] After Disabling AXIQ_L : DDR3 = 0x%x State %d",
						in_le32( puiRegAddr ), pxAxiqLib->eSubsystemState[ AXIQ_L0 ] );

	return;

	log_dbg( "\n\r[AXIQ] After Disabling AXIQ_L : DDR3 = 0x%x", in_le32( puiRegAddr ) );
}


/* Control Loopback between AXIQ-H and HS-DCS */
void vLbConfigAxiqH( AxiqLibHndlr_t * pxAxiqLib, AxiqMode_t eMode )
{
	volatile ScfgConfigControl0_t * pxScc0 = ( ScfgConfigControl0_t * ) SCFG_CONFIG_CONTROL0;
	volatile uint32_t * puiRegAddr = ( uint32_t * ) pxScc0;

	log_dbg( "\n\r[AXIQ] Address SCC0 = 0x%x", puiRegAddr );

    /* pxScc0->LB_AXIQ_H = mode */
    switch( eMode )
    {
        case LOOPBACK_MODE_DISABLED_0:
        case LOOPBACK_MODE_DISABLED_3:
            clrbits_le32( puiRegAddr, ( uint32 )(0x1 << ( uint32 ) SCFG_CONFIG_CONTROL0_BIT_LB_AXIQ_H) );
            break;
        case INTER_LOOPBACK_MODE_ENABLED_1:
            setbits_le32( puiRegAddr, ( uint32 )(0x1 << (SCFG_CONFIG_CONTROL0_BIT_LB_AXIQ_H)) );
            break;
        case INTRA_LOOPBACK_MODE_ENABLED_2:
            log_err("\n\r[AXIQ] Invalid Parameter passed to lb_config_axiq_h() ");
            configASSERT( 0 );
            break;
	default:
	    break;
    }

    pxAxiqLib->eAxiqMode[ AXIQ_H ] = eMode;
	log_dbg( "\n\r[AXIQ] Loopback mode(%d) configured for AXIQ_H : SCFG_CONFIG_CONTROL0 val = 0x%x",
	   pxAxiqLib->eAxiqMode[ AXIQ_H ], in_le32( puiRegAddr ) );

	return;
}

/* Control Loopback between AXIQ-L0 and LS-DCS0 */
void vLbConfigAxiqL0( AxiqLibHndlr_t * pxAxiqLib, AxiqMode_t eMode )
{
	volatile ScfgConfigControl0_t * pxScc0 = ( ScfgConfigControl0_t * ) SCFG_CONFIG_CONTROL0;
	volatile uint32 * puiRegAddr = ( uint32 * ) pxScc0;
	uint32 uiMask = 0, uiRegValue = 0;

	/* pxScc0->LB_AXIQ_L0 = mode */
	uiRegValue = in_le32( puiRegAddr ) & ~( ( uint32 ) FLAG_0b11 << SCFG_CONFIG_CONTROL0_BIT_LB_AXIQ_L0 );
	uiMask = eMode & FLAG_0b11;
	uiMask = uiMask << SCFG_CONFIG_CONTROL0_BIT_LB_AXIQ_L0;
	uiRegValue = uiRegValue | uiMask;
	out_le32( puiRegAddr, uiRegValue );

    pxAxiqLib->eAxiqMode[ AXIQ_L0 ] = eMode;
	log_dbg("\n\r[AXIQ] Loopback mode(%d) configured for AXIQ_L0 : SCFG_CONFIG_CONTROL0 val = 0x%x",
						pxAxiqLib->eAxiqMode[ AXIQ_L0 ], in_le32( puiRegAddr ) );
}

/* Control Loopback between AXIQ-L1 and LS-DCS1 */
void vLbConfigAxiqL1( AxiqLibHndlr_t * pxAxiqLib, AxiqMode_t eMode )
{
	volatile ScfgConfigControl0_t * pxScc0 = ( ScfgConfigControl0_t * ) SCFG_CONFIG_CONTROL0;
	volatile uint32 * puiRegAddr = ( uint32 * ) pxScc0;
	uint32 uiMask = 0, uiRegValue = 0;

	/* pxScc0->LB_AXIQ_L1 = mode */
	uiRegValue = in_le32( puiRegAddr ) & ~( ( uint32 ) FLAG_0b11 << SCFG_CONFIG_CONTROL0_BIT_LB_AXIQ_L1 );
	uiMask = eMode & FLAG_0b11;
	uiMask = uiMask << SCFG_CONFIG_CONTROL0_BIT_LB_AXIQ_L1;
	uiRegValue = uiRegValue | uiMask;
	out_le32( puiRegAddr, uiRegValue );

    pxAxiqLib->eAxiqMode[ AXIQ_L1 ] = eMode;
	log_dbg( "\n\r[AXIQ] Loopback mode(%d) configured for AXIQ_L1 : SCFG_CONFIG_CONTROL0 val = 0x%x",
	   pxAxiqLib->eAxiqMode[ AXIQ_L1 ], in_le32( puiRegAddr ) );
}

void vRegisterAxiqErrorInterrupt( AxiqLibHndlr_t * pxAxiqLib, bIsrFunc AxiqIntHandler, void *AxiqData, AxiqNum_t eAxiqNum )
{
    uint32_t uiAxiqIrqNum = AxiqIntLines[ eAxiqNum ];

    /* TODO: Spinlock syncronization may be required
     * */
    if ( !pxAxiqLib->eIrqRegistered[ eAxiqNum ] ) {
        lRegisterIrq( uiAxiqIrqNum + INTERNAL_IRQ_OFFSET, AxiqIntHandler, AxiqData );
		bMpicEnable( DEVICE_INTERNAL, uiAxiqIrqNum );
        pxAxiqLib->eIrqRegistered[ eAxiqNum ] = AXI_IRQ_REGISTERED;
    }

	return;
}

void vUnRegisterAxiqErrorInterrupt( AxiqLibHndlr_t * pxAxiqLib, AxiqNum_t eAxiqNum )
{
    uint32_t uiAxiqIrqNum = AxiqIntLines[ eAxiqNum ];

    /* TODO: Spinlock syncronization may be required
     * */
    if ( pxAxiqLib->eIrqRegistered[ eAxiqNum ] ) {
		bMpicDisable( DEVICE_INTERNAL, uiAxiqIrqNum );
        vUnregisterIrq( uiAxiqIrqNum + INTERNAL_IRQ_OFFSET );
        pxAxiqLib->eIrqRegistered[ eAxiqNum ] = AXI_IRQ_UNREGISTERED;
    }

	return;
}

AxiqLibHndlr_t * pxAxiqInit( void )
{
    int i = 0;

    log_dbg( "\n%s:Initializing AXIQ library ...\r\n", __func__ );
    if ( NULL == pAxiqLibHndlr )
    {
		pAxiqLibHndlr = pvGeulMalloc( sizeof( AxiqLibHndlr_t ) );
		if ( NULL == pAxiqLibHndlr )
		{
			log_err( "%s: Memory allocation failed for "
					"AXIQ\r\n", __func__ );
			goto hndl_retval;
		}

		for (; i < NUM_AXIQ_INTERRUPTS; i++)
		    pAxiqLibHndlr->eIrqRegistered[ i ] = AXI_IRQ_UNREGISTERED;

		for (i = 0; i < NUM_AXIQ_SUBSYSTEM; i++)
		    pAxiqLibHndlr->eSubsystemState[ i ] = SUBSYSTEM_DISABLE;

		for (i = 0; i < NUM_AXIQ_SUBSYSTEM; i++)
		    pAxiqLibHndlr->eAxiqMode[ i ] = LOOPBACK_MODE_DISABLED_0;
    }

hndl_retval:
    return pAxiqLibHndlr;
}

void vAxiqClose( void )
{
    log_dbg( "\n%s:Closing AXIQ library ...\r\n", __func__ );
    if ( NULL != pAxiqLibHndlr )
    {
        vGeulFree( pAxiqLibHndlr );
    }

    return;
}

/******************************************************************************
*************************** Static / Local Functions **************************
***************************     Helper Functions     **************************
******************************************************************************/
