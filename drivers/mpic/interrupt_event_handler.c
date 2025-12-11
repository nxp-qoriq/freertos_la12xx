// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2023 NXP
 */

#include "types.h"
#include "common.h"
#include "config.h"
#include "platform_def.h"
#include "mpic_regs.h"
#include <mpic.h>
#include "gul_bsp_init.h"


static void *pvIsrData[ MAX_IRQ_INDEX ];

static bIsrFunc prvInterruptVectorTable[ MAX_IRQ_INDEX ];

void vInterruptEventHandler( uint32_t ulIrq )
{
	if( ulIrq >= MAX_IRQ_INDEX )
	{
		log_err( "%s : Received invalid IRQ number : %u\n\r", __func__, ulIrq );
		return;
	}
#if ( STATS_INTERRUPT_RAISED == 1 )
	volatile struct gul_hif *pHif = pGulModPriv->pHif;
	volatile struct gul_stats *pGulStats = &pHif->stats;

	if(( ulIrq >= MSI_INTR_START && ulIrq <= MSI_INTR_END ) || ( ulIrq >= MSIB_INTR_START && ulIrq <= MSIB_INTR_END )
			|| ( ulIrq >= MSIC_INTR_START && ulIrq <= MSIC_INTR_END ))
	{
		out_le32( &pGulStats->mpic_stats.msi_irq_cnt,
				( in_le32( &pGulStats->mpic_stats.msi_irq_cnt ) + 1 ) );
	}

	if( ulIrq >= TIMER_A_INTR_START && ulIrq <= TIMER_B_INTR_END )
	{
		out_le32( &pGulStats->mpic_stats.timer_irq_cnt,
				( in_le32( &pGulStats->mpic_stats.timer_irq_cnt ) + 1 ) );
	}

#endif
	if( NULL != prvInterruptVectorTable[ ulIrq ] )
		( *prvInterruptVectorTable[ ulIrq ] ) ( ulIrq, pvIsrData[ ulIrq ] );
	else
		log_isr( "%s : spurious interrupt : %u\n\r", __func__, ulIrq );
}

int lRegisterIrq( uint32_t ulIrqVectNo, bIsrFunc bIsr, void *pvData )
{
        if( ulIrqVectNo >= MAX_IRQ_INDEX )
        {
                return -1;
        }

        prvInterruptVectorTable[ ulIrqVectNo ] = bIsr;
        pvIsrData[ ulIrqVectNo ] = pvData;

        return 1;
}

void vUnregisterIrq( uint32_t ulIrqVectNo )
{
        if( ulIrqVectNo >= MAX_IRQ_INDEX )
        {
                return;
        }
	else
	{
		prvInterruptVectorTable[ ulIrqVectNo ] = NULL;
		pvIsrData[ ulIrqVectNo ] = NULL;
	}
}
