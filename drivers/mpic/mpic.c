// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#include "types.h"
#include "common.h"
#include "config.h"
#include "platform_def.h"
#include "mpic_regs.h"
#include "spinlock_api.h"
#include <mpic.h>
#include "Time.h"
#include "soc.h"

static const u32 ulPrAry[ ] = {
		MPIC_REGS_IIVPR0,
		MPIC_REGS_IPIVPR0,
		MPIC_REGS_MIVPRA0,
		MPIC_REGS_MSIVPR0,
		MPIC_REGS_GTVPRA0,
		MPIC_REGS_GTVPRB0,
		MPIC_REGS_EIVPR0,
		MPIC_REGS_MSIVPRB0,
		MPIC_REGS_MSIVPRC0
};

static const u32 ulDrAry[ ] = {
		MPIC_REGS_IIDR0,
		MPIC_REGS_IPIDR0,
		MPIC_REGS_MIDRA0,
		MPIC_REGS_MSIDR0,
		MPIC_REGS_GTDRA0,
		MPIC_REGS_GTDRB0,
		MPIC_REGS_EIDR0,
		MPIC_REGS_MSIDRB0,
		MPIC_REGS_MSIDRC0
};

typedef struct bMPICRegsStatus
{
	bool_t MPIC_REGS_EIVPR[12];
	bool_t MPIC_REGS_IIVPR[128];
	bool_t MPIC_REGS_IPIVPR[4];
	bool_t MPIC_REGS_MIVPRAB[8];
	bool_t MPIC_REGS_MSIVPR[8];
	bool_t MPIC_REGS_GTVPRA[4];
	bool_t MPIC_REGS_GTVPRB[4];
	bool_t MPIC_REGS_MSIVPRB[8];
	bool_t MPIC_REGS_MSIVPRC[8];
} bMPICRegsStatus_t;

static bMPICRegsStatus_t bMPICRegsStatusEnable;
static struct SpinLock *pxMPICLock;
void vMpicRaiseMessageSharedInterrupt( uint32_t ulIrqNo )
{
	if ( ulIrqNo <= 7 )
	{
		mpic_out32( MPIC_REGS_MSIIR, ( ulIrqNo << 29 ) | ( ulIrqNo << 24 ));
	}
}

u32 ulMpicCurrentCore( void )
{
	u32 ulCoreId = mpic_in32( MPIC_REGS_WHOAMI );

	return ulCoreId;
}

bool_t bMpicInitialize( u32 ulTargetCpu )
{

	u32 ulReg, ulVector;

	/* Reset MPIC */
	mpic_out32( MPIC_REGS_GCR, ( U_INT_ONE << MPIC_RST ) );
	while( mpic_in32( MPIC_REGS_GCR ) & ( U_INT_ONE << MPIC_RST ) );

	/* Enable MPIC_REGS mixed mode */
	mpic_out32( MPIC_REGS_GCR, ( U_INT_ONE << MPIC_MODE ) );

	ulTargetCpu = 1 << ulTargetCpu;

	/* External interrupts configuration */
	/*
	 * Mask all external interrupts.
	 * Set default priority as 8, Polarity as Active high
	 * and sense as Edge sensitive for all external interrupts.
	 * Set External Interrupts from Vector(Irq) No :- 0-11
	 */

	/* Mpic recomendation to avoid false edge interrupt */
	for( ulReg = MPIC_REGS_EIVPR0, ulVector = 0; ulReg <= MPIC_REGS_EIVPR11;
			ulReg += 0x20, ulVector += VECTOR_TBL_SIZE )
	{
		mpic_out32( ulReg, ( MASK_DISABLE << MSK ) |
				( DEFAULT_POLARITY << DEFAULT_POLARITY_OFFSET ) |
				( LEVEL_SENSITIVE << EXT_INT_SENSE_OFFSET ) |
				( DEFAULT_PRIORITY << DEFAULT_PRIORITY_OFFSET ) | ulVector );
	}

	for( ulReg = MPIC_REGS_EIVPR0, ulVector = 0; ulReg <= MPIC_REGS_EIVPR11;
	     ulReg += 0x20, ulVector += VECTOR_TBL_SIZE )
	{
		mpic_out32( ulReg, ( MASK_DISABLE << MSK ) |
				( DEFAULT_POLARITY << DEFAULT_POLARITY_OFFSET ) |
				( EDGE_SENSITIVE << EXT_INT_SENSE_OFFSET ) |
				( DEFAULT_PRIORITY << DEFAULT_PRIORITY_OFFSET ) | ulVector );
	}

	/* Redirect all external interrupts to target cpu int0 */
	for( ulReg = MPIC_REGS_EIDR0; ulReg <= MPIC_REGS_EIDR11; ulReg += 0x20 )
	{
		mpic_out32( ulReg, ulTargetCpu );
	}

	/* Internal interrupts configuration */
	/*
	 * Set default priority as 8, Polarity as Active high for all
	 * internal interrupts. Mask all internal interrupts
	 * Internal Interrupts from Vector(Irq) No :- 16-143
	 */
	for( ulReg = MPIC_REGS_IIVPR0, ulVector = INTERNAL_IRQ_OFFSET;
			ulReg <= MPIC_REGS_IIVPR127;
			ulReg += 0x20, ulVector += VECTOR_TBL_SIZE )
	{
		mpic_out32( ulReg, ( DEFAULT_PRIORITY << DEFAULT_PRIORITY_OFFSET ) |
		ulVector | ( DEFAULT_POLARITY << DEFAULT_POLARITY_OFFSET ) |
		 MASK_DISABLE << MSK );
	}

	/* Redirect all internal interrupts to target cpu int0 */
	for( ulReg = MPIC_REGS_IIDR0; ulReg <= MPIC_REGS_IIDR127; ulReg += 0x20 )
	{
		mpic_out32( ulReg, ulTargetCpu );
	}

	/* Interprocessor interrupts configuration */
	/*
	 * Set default priority as 8, Polarity as Active high for all
	 * interprocessor interrupts. Mask all IPIs
	 * IPIs from Vector(Irq) No :- 144 to 147
	 */
	for( ulReg = MPIC_REGS_IPIVPR0;ulReg <= MPIC_REGS_IPIVPR3; ulReg+= 0x10,
		ulVector += VECTOR_TBL_SIZE )
	{
		mpic_out32( ulReg, ( DEFAULT_PRIORITY << DEFAULT_PRIORITY_OFFSET ) |
			 ulVector | MASK_DISABLE << MSK );
	}

	/* Redirect all inter-processor interrupts to target cpu int0 */
	for( ulReg = MPIC_REGS_IPIDR0+PER_CPU_PRIVATE_REG_WORKARD;
	     ulReg <= MPIC_REGS_IPIDR3 + PER_CPU_PRIVATE_REG_WORKARD; ulReg += 0x10 )
	{
		mpic_out32( ulReg, ulTargetCpu );
	}

	/* Messaging interrupts configuration */
	/*
	 * Set default priority as 8, Polarity as Active high for all
	 * messaging interrupts. Mask all messaging interrupts
	 * Messaging Interrupts from Vector(Irq) No :- 148-155
	 */
	for( ulReg = MPIC_REGS_MIVPRA0; ulReg <= MPIC_REGS_MIVPRB3; ulReg += 0x20,
	     ulVector += VECTOR_TBL_SIZE )
	{
		mpic_out32( ulReg, ( DEFAULT_PRIORITY << DEFAULT_PRIORITY_OFFSET ) |
			 ulVector | MASK_DISABLE << MSK );
	}

	/* Redirect all messaging interrupts to target cpu int0 */
	for( ulReg = MPIC_REGS_MIDRA0; ulReg <= MPIC_REGS_MIDRB3; ulReg += 0x20 )
	{
		mpic_out32( ulReg, ulTargetCpu );
	}

	/* Message Signaled interrupts configuration */
	/*
	 * Set default priority as 8, Polarity as Active high for all
	 * MSI interrupts. Mask all MSI interrupts
	 * Messaging Interrupts from Vector(Irq) No :- 156-163
	 */
	for( ulReg = MPIC_REGS_MSIVPR0; ulReg <= MPIC_REGS_MSIVPR7; ulReg+= 0x20,
		ulVector += VECTOR_TBL_SIZE )
	{
		mpic_out32( ulReg, ( DEFAULT_PRIORITY << DEFAULT_PRIORITY_OFFSET ) |
			 ulVector | MASK_DISABLE << MSK );
	}

	/* Redirect all MSI interrupts to target cpu int0 */
	for( ulReg = MPIC_REGS_MSIDR0; ulReg <= MPIC_REGS_MSIDR7; ulReg += 0x20 )
	{
		mpic_out32( ulReg, ulTargetCpu );
	}

	/* Global timer interrupts configuration */
	/*
	 * Set priority as 15, Polarity as Active high for all
	 * global timer interrupts (GRP A). Mask all global timer interrupts
	 * Global Timer Interrupts Grp A from Vector(Irq) No :- 164-167
	 */
	for( ulReg = MPIC_REGS_GTVPRA0; ulReg <= MPIC_REGS_GTVPRA3; ulReg+= 0x40,
		ulVector += VECTOR_TBL_SIZE )
	{
		mpic_out32( ulReg, ( TIMER_PRIORITY << DEFAULT_PRIORITY_OFFSET ) |
				ulVector | MASK_DISABLE << MSK );
	}

	/* Redirect all global timer interrupts to target cpu int0 */
	for( ulReg = MPIC_REGS_GTDRA0; ulReg <= MPIC_REGS_GTDRA3; ulReg += 0x40 )
	{
		mpic_out32( ulReg, ulTargetCpu );
	}

	/* Global timer interrupts configuration */
	/*
	 * Set priority as 15, Polarity as Active high for all
	 * global timer interrupts (GRP B). Mask all global timer interrupts
	 * Global Timer Interrupts Grp B from Vector(Irq) No :- 168-171
	 */
	for( ulReg = MPIC_REGS_GTVPRB0; ulReg <= MPIC_REGS_GTVPRB3; ulReg+= 0x40,
		ulVector += VECTOR_TBL_SIZE )
	{
		mpic_out32( ulReg, ( TIMER_PRIORITY << DEFAULT_PRIORITY_OFFSET ) |
				ulVector | MASK_DISABLE << MSK );
	}

	/* Redirect all global timer interrupts to target cpu int0 */
	for( ulReg = MPIC_REGS_GTDRB0; ulReg <= MPIC_REGS_GTDRB3; ulReg += 0x40 )
	{
		mpic_out32( ulReg, ulTargetCpu );
	}

	/* Message Signaled interrupts configuration for Bank B*/
	/*
	 * Set default priority as 8, Polarity as Active high for all
	 * MSI interrupts. Mask all MSI interrupts
	 * Messaging Interrupts from Vector(Irq) No :- 172-179
	 */
	for( ulReg = MPIC_REGS_MSIVPRB0; ulReg <= MPIC_REGS_MSIVPRB7; ulReg+= 0x20,
		ulVector += VECTOR_TBL_SIZE )
	{
		mpic_out32( ulReg, ( DEFAULT_PRIORITY << DEFAULT_PRIORITY_OFFSET ) |
			 ulVector | MASK_DISABLE << MSK );
	}

	/* Redirect all MSI interrupts to target cpu int0 */
	for( ulReg = MPIC_REGS_MSIDRB0; ulReg <= MPIC_REGS_MSIDRB7; ulReg += 0x20 )
	{
		mpic_out32( ulReg, ulTargetCpu );
	}

	/* Message Signaled interrupts configuration for Bank C*/
	/*
	 * Set default priority as 8, Polarity as Active high for all
	 * MSI interrupts. Mask all MSI interrupts
	 * Messaging Interrupts from Vector(Irq) No :- 180-187
	 */
	for( ulReg = MPIC_REGS_MSIVPRC0; ulReg <= MPIC_REGS_MSIVPRC7; ulReg+= 0x20,
		ulVector += VECTOR_TBL_SIZE )
	{
		mpic_out32( ulReg, ( DEFAULT_PRIORITY << DEFAULT_PRIORITY_OFFSET ) |
			 ulVector | MASK_DISABLE << MSK );
	}

	/* Redirect all MSI interrupts to target cpu int0 */
	for( ulReg = MPIC_REGS_MSIDRC0; ulReg <= MPIC_REGS_MSIDRC7; ulReg += 0x20 )
	{
		mpic_out32( ulReg, ulTargetCpu );
	}

	return TRUE;
}

bool_t bMpicEnable( u32 ulDeviceGroup, u32 ulDeviceNum )
{

	bool_t bResult;
	u32 ulPr, ulDr, ulOffset, ulDst = 0;
	u32 cpu_mask;

	if( pxMPICLock == NULL)
	{
		pxMPICLock = pxSpinLockGet( NULL, NULL, SPINLOCK_MPIC);
	}

	if( DEVICE_INTER_PROC == ulDeviceGroup )
		ulOffset = 0x10;
	else if( ( DEVICE_TIMER_GRP_A == ulDeviceGroup ) ||
		( ( DEVICE_TIMER_GRP_B == ulDeviceGroup )) )
		ulOffset = 0x40;
	else
		ulOffset = 0x20;

	ulDr = ulDrAry[ ulDeviceGroup ] + ulDeviceNum * ulOffset +
		( u32 ) PER_CPU_PRIVATE_REG_WORKARD * ( DEVICE_INTER_PROC == ulDeviceGroup );
	ulPr = ulPrAry[ ulDeviceGroup ] + ulDeviceNum * ulOffset;

	ulDst = ( U_INT_ONE << ulMpicCurrentCore( ) );

	if (get_soc_revision() == GEUL_SVR_REVB_VAL) {
		cpu_mask = P0 | P1 | P2 | P3 | P4 | P5;
	} else {
		cpu_mask = P0 | P1 | P2 | P3;
	}

	if( ( u32 )( cpu_mask | EP | CI0 | CI1 ) & ulDst )
	{
		if( ( DEVICE_TIMER_GRP_A == ulDeviceGroup ) ||
				( ( DEVICE_TIMER_GRP_B == ulDeviceGroup ) ) )
			/* Processor core# 0-n starts receiving interrupt.
			 * This interrupt is multicasting, so multiple bits can be set. */
                     {
                                /* Processor core# 0-n starts receiving interrupt.
                                 * This interrupt is multicasting, so multiple bits can be set. */
			        vSpinLockAcquire( pxMPICLock );
                                mpic_out32( ulDr, ( mpic_in32( ulDr ) | ulDst ) );
                                vSpinLockRelease( pxMPICLock );
                     }
		else {
			/* Processor core# 0-n starts receiving interrupt */
			mpic_out32( ulDr, ulDst );
		}

		/* Interrupts from the source are unmasked */
		mpic_out32( ulPr, ( mpic_in32( ulPr ) & ~( MASK_DISABLE << MSK ) ) );
		bResult = TRUE;
	}
	else
	{
		bResult = FALSE;
	}

	return bResult;
}

bool_t bMpicDisable( u32 ulDeviceGroup, u32 ulDeviceNum )
{
	bool_t bResult;
	u32 ulPr, ulDr, ulOffset, ulDst = 0;
	u32 cpu_mask;

	if( DEVICE_INTER_PROC == ulDeviceGroup )
		ulOffset = 0x10;
	else if( ( DEVICE_TIMER_GRP_A == ulDeviceGroup ) ||
		( ( DEVICE_TIMER_GRP_B == ulDeviceGroup ) ) )
		ulOffset = 0x40;
	else
		ulOffset = 0x20;

	ulDr = ulDrAry[ ulDeviceGroup ] + ulDeviceNum * ulOffset +
		( u32 )PER_CPU_PRIVATE_REG_WORKARD * ( DEVICE_INTER_PROC == ulDeviceGroup );
	ulPr = ulPrAry[ ulDeviceGroup ] + ulDeviceNum * ulOffset;

	ulDst = ( U_INT_ONE << ulMpicCurrentCore( ) );

	if (get_soc_revision() == GEUL_SVR_REVB_VAL) {
		cpu_mask = P0 | P1 | P2 | P3 | P4 | P5;
	} else {
		cpu_mask = P0 | P1 | P2 | P3;
	}

	if( ( u32 )( cpu_mask | EP | CI0 | CI1 ) & ulDst )
	{
		/* Further interrupts from the source are disabled */
		mpic_out32( ( ulPr ), mpic_in32( ulPr ) | ( MASK_DISABLE << MSK ) );

		/* Processor core# 0-3 does not receive interrupt */
		mpic_out32( ulDr, ( mpic_in32( ulDr ) & ~( ulDst ) ) );

		bResult = TRUE;
	}
	else
	{
		bResult = FALSE;
	}

	return bResult;
}

bool_t bMpicConfigTimer( u32 ulValue )
{
	if (ulMpicCurrentCore() == GEUL_E200_MASTER_CORE)
		mpic_out32( MPIC_REGS_GTBCRA0, ulValue );

	bMpicEnable( DEVICE_TIMER_GRP_A, 0 );

	return TRUE;
}

bool_t bMpicConfigGlobalTimerB( void )
{
	/* Configure Global Timer B to cascade Timer 0 and 1 to get one 63-bit timer */
	mpic_out32( MPIC_REGS_TCRB, ( mpic_in32( MPIC_REGS_TCRB ) | ( TIMER_ROVR_0_1 << TIMER_ROVR_BIT ) |
				( TIMER_CLKR_DEFAULT << TIMER_CLKR_BIT ) | ( TIMER_CASC_0_1 << TIMER_CASC_BIT ) ) );

	/* Start Counting */
	if ( ulMpicCurrentCore() == GEUL_E200_MASTER_CORE )
		mpic_out32( MPIC_REGS_GTBCRB1, 0x7FFFFFFF );

	return TRUE;
}

u64 ulGetMpicGloablTimerBCurrentCount( void )
{
	u64 ullCurrentCount = 0;
	u32 ulCurrentCountHi = 0, ulCurrentCountLo = 0;

	ulCurrentCountHi = ( mpic_in32( MPIC_REGS_GTCCRB1 ) & 0x7FFFFFFF );
	ulCurrentCountLo =  mpic_in32( MPIC_REGS_GTCCRB0 );


	ullCurrentCount = ( 0x7FFFFFFF - ulCurrentCountHi );
	ullCurrentCount = ullCurrentCount * 0xFFFFFFFF;
	ullCurrentCount = ullCurrentCount + ( 0xFFFFFFFF - ulCurrentCountLo );

	return TIMER_COUNT_TO_US( ullCurrentCount );
}

bool_t bMpicEoi( u32 ulCpu )
{
	u32 ulOffset;

	ulOffset = MPIC_REGS_CPU1_OFF * ulCpu + PER_CPU_PRIVATE_REG_WORKARD;

	/* write EOI */
	mpic_out32( MPIC_REGS_EOI0 + ulOffset, 0 );

	return TRUE;
}

u32 ulMpicAck( u32 ulCpu )
{
	u32 ulVector, ulOffset;

	ulOffset = MPIC_REGS_CPU1_OFF * ulCpu + PER_CPU_PRIVATE_REG_WORKARD;
	// read IACK ulVector
	ulVector = mpic_in32( MPIC_REGS_IACK0 + ulOffset );

	return ulVector;
}

bool_t bSetCurrentTskPrio( u32 ulCpu, u32 ulPrio )
{
	u32 ulCtprOff;

	ulCtprOff = MPIC_REGS_CPU1_OFF * ulCpu + PER_CPU_PRIVATE_REG_WORKARD;

	mpic_out32( MPIC_REGS_CTPR0 + ulCtprOff, ulPrio );

	return TRUE;

}

bool_t bMpicSetDevicePriority( u32 ulDeviceGroup, u32 ulDeviceNum,
		u32 ulPriority )
{
	bool ulResult;
	u32 ulPr, ulOffset;

	if( DEVICE_INTER_PROC == ulDeviceGroup )
		ulOffset = 0x10;
	else if( ( DEVICE_TIMER_GRP_A == ulDeviceGroup ) ||
			( ( DEVICE_TIMER_GRP_B == ulDeviceGroup ) ) )
		ulOffset = 0x40;
	else
		ulOffset = 0x20;

	ulPr = ulPrAry[ ulDeviceGroup ] + ulDeviceNum * ulOffset;

	if( HIGHEST_PRIORITY >= ulPriority )
	{
		mpic_out32( ulPr, ( mpic_in32( ulPr ) & ( u32 ) ( ~( MPIC_PRIORITY_MASK ) ) )
				| ( ulPriority << DEFAULT_PRIORITY_OFFSET ) );
		ulResult = TRUE;
	}
	else
	{
		log_err("\r\n[%s] Invalid priority, valid priority level is between 1 to 15\r\n"
				, __func__);
		ulResult = FALSE;
	}

	return ulResult;
}

u32 ulMpicGetDevicePriority( u32 ulDeviceGroup, u32 ulDeviceNum )
{
	u32 ulResult, ulPr, ulOffset;

	if( DEVICE_INTER_PROC == ulDeviceGroup )
		ulOffset = 0x10;
	else if( ( DEVICE_TIMER_GRP_A == ulDeviceGroup ) ||
			( ( DEVICE_TIMER_GRP_B == ulDeviceGroup ) ) )
		ulOffset = 0x40;
	else
		ulOffset = 0x20;

	ulPr = ulPrAry[ ulDeviceGroup ] + ulDeviceNum * ulOffset;

	ulResult = ( ( mpic_in32( ulPr ) & ( u32 ) ( ( MPIC_PRIORITY_MASK ) ) )
			>> DEFAULT_PRIORITY_OFFSET );

	return ulResult;
}

u32 ulSetMpicDivFactor( u32 ulDivFactor )
{
    u32 ulFactor = 0;
    switch ( ulDivFactor )
    {
        case 8:
            ulFactor = 0;
            break;
        case 16:
            ulFactor = 1;
            break;
        case 32:
            ulFactor = 2;
            break;
        case 64:
            ulFactor = 3;
            break;
        default:
            ulFactor = 0;
            break;
    }
    mpic_out32( MPIC_REGS_TCRA, ( ulFactor << DIV_FACT_SHIFT ) );

    return ulDivFactor;
}

u32 ulGetMpicCurrentTickTimerCount( void )
{
	return ( mpic_in32( MPIC_REGS_GTCCRA0 ) & 0x7FFFFFFF );
}

bool_t bMpicMaskInterrupts( void )
{
	u32 ulReg;
	u32 ulIndex;

	memset( &bMPICRegsStatusEnable, 0x0, sizeof( bMPICRegsStatus_t ) );

	/* Mask all external interrupts */
	for( ulIndex = 0, ulReg = MPIC_REGS_EIVPR0; ulReg <= MPIC_REGS_EIVPR11; ulReg += 0x20, ulIndex++ )
	{
		if ( !( ( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) ) )
		{
			bMPICRegsStatusEnable.MPIC_REGS_EIVPR[ulIndex] = 1;
			mpic_out32( ulReg, ( MASK_DISABLE << MSK ) | mpic_in32( ulReg ) );
		}
	}

	/* Mask all Internal interrupts */
	for( ulIndex = 0, ulReg = MPIC_REGS_IIVPR0; ulReg <= MPIC_REGS_IIVPR127; ulReg += 0x20, ulIndex++ )
	{
		if ( !( ( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) ) )
		{
			bMPICRegsStatusEnable.MPIC_REGS_IIVPR[ulIndex] = 1;
			mpic_out32( ulReg, ( MASK_DISABLE << MSK ) | mpic_in32( ulReg ) );
		}
	}

	/* Mask all Interprocessor interrupts configuration */
	for( ulIndex = 0, ulReg = MPIC_REGS_IPIVPR0; ulReg <= MPIC_REGS_IPIVPR3; ulReg+= 0x10, ulIndex++ )
	{
		if ( !( ( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) ) )
		{
			bMPICRegsStatusEnable.MPIC_REGS_IPIVPR[ulIndex] = 1;
			mpic_out32( ulReg, ( MASK_DISABLE << MSK ) | mpic_in32( ulReg ) );
		}
	}

	/* Mask all Messaging interrupts configuration */
	for( ulIndex = 0, ulReg = MPIC_REGS_MIVPRA0; ulReg <= MPIC_REGS_MIVPRB3; ulReg += 0x20, ulIndex++ )
	{
		if ( !( ( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) ) )
		{
			bMPICRegsStatusEnable.MPIC_REGS_MIVPRAB[ulIndex] = 1;
			mpic_out32( ulReg, ( MASK_DISABLE << MSK ) | mpic_in32( ulReg ) );
		}
	}

	/* Mask all Message Signaled interrupts configuration */
	for( ulIndex = 0, ulReg = MPIC_REGS_MSIVPR0; ulReg <= MPIC_REGS_MSIVPR7; ulReg+= 0x20, ulIndex++ )
	{
		if ( !( ( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) ) )
		{
			bMPICRegsStatusEnable.MPIC_REGS_MSIVPR[ulIndex] = 1;
			mpic_out32( ulReg, ( MASK_DISABLE << MSK ) | mpic_in32( ulReg ) );
		}
	}

	/* Mask all Message Signaled interrupts configuration Bank C */
	for( ulIndex = 0, ulReg = MPIC_REGS_MSIVPRC0; ulReg <= MPIC_REGS_MSIVPRC7; ulReg+= 0x20, ulIndex++ )
	{
		if ( !( ( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) ) )
		{
			bMPICRegsStatusEnable.MPIC_REGS_MSIVPR[ulIndex] = 1;
			mpic_out32( ulReg, ( MASK_DISABLE << MSK ) | mpic_in32( ulReg ) );
		}
	}

	/* Mask all Global timer Group A interrupts */
	for( ulIndex = 0, ulReg = MPIC_REGS_GTVPRA0; ulReg <= MPIC_REGS_GTVPRA3; ulReg+= 0x40, ulIndex++ )
	{
		if ( !( ( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) ) )
		{
			bMPICRegsStatusEnable.MPIC_REGS_GTVPRA[ulIndex] = 1;
			mpic_out32( ulReg, ( MASK_DISABLE << MSK ) | mpic_in32( ulReg ) );
		}
	}

	/* Mask all Global timer Group B interrupts */
	for( ulIndex = 0, ulReg = MPIC_REGS_GTVPRB0; ulReg <= MPIC_REGS_GTVPRB3; ulReg+= 0x40, ulIndex++ )
	{
		if ( !( ( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) ) )
		{
			bMPICRegsStatusEnable.MPIC_REGS_GTVPRB[ulIndex] = 1;
			mpic_out32( ulReg, ( MASK_DISABLE << MSK ) | mpic_in32( ulReg ) );
		}
	}

	return TRUE;
}

bool_t bMpicMaskInterruptsEnable( void )
{
	u32 ulReg;
	u32 ulIndex;

	/* UnMask all external interrupts */
	for( ulIndex = 0, ulReg = MPIC_REGS_EIVPR0; ulReg <= MPIC_REGS_EIVPR11; ulReg += 0x20, ulIndex++ )
	{
		if ( bMPICRegsStatusEnable.MPIC_REGS_EIVPR[ulIndex] )
		{
			mpic_out32( ulReg, ~( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) );
		}
	}

	/* UnMask all Internal interrupts */
	for( ulIndex = 0, ulReg = MPIC_REGS_IIVPR0; ulReg <= MPIC_REGS_IIVPR127; ulReg += 0x20, ulIndex++ )
	{
		if ( bMPICRegsStatusEnable.MPIC_REGS_IIVPR[ulIndex] )
		{
			mpic_out32( ulReg, ~( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) );
		}
	}

	/* UnMask all Interprocessor interrupts configuration */
	for( ulIndex = 0, ulReg = MPIC_REGS_IPIVPR0; ulReg <= MPIC_REGS_IPIVPR3; ulReg+= 0x10, ulIndex++ )
	{
		if ( bMPICRegsStatusEnable.MPIC_REGS_IPIVPR[ulIndex] )
		{
			mpic_out32( ulReg, ~( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) );
		}
	}

	/* UnMask all Messaging interrupts configuration */
	for( ulIndex = 0, ulReg = MPIC_REGS_MIVPRA0; ulReg <= MPIC_REGS_MIVPRB3; ulReg += 0x20, ulIndex++ )
	{
		if ( bMPICRegsStatusEnable.MPIC_REGS_MIVPRAB[ulIndex] )
		{
			mpic_out32( ulReg, ~( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) );
		}
	}

	/* UnMask all Message Signaled interrupts configuration */
	for( ulIndex = 0, ulReg = MPIC_REGS_MSIVPR0; ulReg <= MPIC_REGS_MSIVPR7; ulReg+= 0x20, ulIndex++ )
	{
		if ( bMPICRegsStatusEnable.MPIC_REGS_MSIVPR[ulIndex] )
		{
			mpic_out32( ulReg, ~( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) );
		}
	}
	/* UnMask all Message Signaled interrupts configuration Bank B*/
	for( ulIndex = 0, ulReg = MPIC_REGS_MSIVPRB0; ulReg <= MPIC_REGS_MSIVPRB7; ulReg+= 0x20, ulIndex++ )
	{
		if ( bMPICRegsStatusEnable.MPIC_REGS_MSIVPRB[ulIndex] )
		{
			mpic_out32( ulReg, ~( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) );
		}
	}

	/* UnMask all Message Signaled interrupts configuration Bank C*/
	for( ulIndex = 0, ulReg = MPIC_REGS_MSIVPRC0; ulReg <= MPIC_REGS_MSIVPRC7; ulReg+= 0x20, ulIndex++ )
	{
		if ( bMPICRegsStatusEnable.MPIC_REGS_MSIVPRC[ulIndex] )
		{
			mpic_out32( ulReg, ~( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) );
		}
	}

	/* UnMask all Global timer Group A interrupts */
	for( ulIndex = 0, ulReg = MPIC_REGS_GTVPRA0; ulReg <= MPIC_REGS_GTVPRA3; ulReg+= 0x40, ulIndex++ )
	{
		if ( bMPICRegsStatusEnable.MPIC_REGS_GTVPRA[ulIndex] )
		{
			mpic_out32( ulReg, ~( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) );
		}
	}

	/* UnMask all Global timer Group B interrupts */
	for( ulIndex = 0, ulReg = MPIC_REGS_GTVPRB0; ulReg <= MPIC_REGS_GTVPRB3; ulReg+= 0x40, ulIndex++ )
	{
		if ( bMPICRegsStatusEnable.MPIC_REGS_GTVPRB[ulIndex] )
		{
			mpic_out32( ulReg, ~( MASK_DISABLE << MSK ) & mpic_in32( ulReg ) );
		}
	}

	return TRUE;
}
