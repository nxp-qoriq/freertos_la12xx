// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2017-2021 NXP
 */

#include <FreeRTOS.h>
#include <task.h>

#include "epu.h"

/* To be called once.
 * Enables Counters of EPU
 * */
void vEpuGlobalInit()
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t cur_val;

    cur_val = IN_32( &pEpuRegs->ulEpgcr );

    cur_val |= EPU_COUNTER_ENABLE;

    OUT_32( &pEpuRegs->ulEpgcr, cur_val );
}

/* Enables nth EPU Counter
 * */
void vEpuCounterEnable( uint8_t epu_counter )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t cur_val;

    epu_counter = epu_counter & EPU_COUNTER_NUM_COUNTERS_MASK;

    cur_val = IN_32( &pEpuRegs->ulEpccrn[ epu_counter ] );

    cur_val |= EPU_COUNTER_ENABLE;

    OUT_32( &pEpuRegs->ulEpccrn[ epu_counter ], cur_val );
}

/* Configures the counter based on the given inputs in cfg
 * and sets the comparator value as well
 * */
int iEpuCounterConfig( uint8_t epu_counter,
                       struct epu_counter_cfg * cfg )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t cur_val;

    epu_counter = epu_counter & EPU_COUNTER_NUM_COUNTERS_MASK;

    cur_val = IN_32( &pEpuRegs->ulEpccrn[ epu_counter ] );

    if( cur_val & EPU_COUNTER_ENABLE )
    {
        return -1;
    }

    if( ( cfg->isel == EPU_COUNTER_INPUT_PLATFORM_CLK ) && cfg->edge_detect )
    {
        return -1;
    }

    cur_val = ( cfg->edge_detect << EPU_COUNTER_EDE_OFFSET ) &
              EPU_COUNTER_EDE_MASK;
    cur_val |= cfg->isel << EPU_COUNTER_ISEL_OFFSET;
    cur_val |= cfg->lt << EPU_COUNTER_LEV_OFFSET;
    cur_val |= cfg->action << EPU_COUNTER_AC_OFFSET;
    cur_val |= cfg->gt << EPU_COUNTER_GEV_OFFSET;

    OUT_32( &pEpuRegs->ulEpcmPRn[ epu_counter ],
            cfg->comparator_val );
    OUT_32( &pEpuRegs->ulEpccrn[ epu_counter ], cur_val );

    return 0;
}

/* Disables nth EPU Counter
 * */
void vEpuCounterDisable( uint8_t epu_counter )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t cur_val;

    epu_counter = epu_counter & EPU_COUNTER_NUM_COUNTERS_MASK;

    cur_val = IN_32( &pEpuRegs->ulEpccrn[ epu_counter ] );

    cur_val &= ~EPU_COUNTER_ENABLE;

    OUT_32( &pEpuRegs->ulEpccrn[ epu_counter ], cur_val );
}

/* Configures the EDFE detect for the nth counter
 * */
int iEpuCounterCfgEdgeDetect( uint8_t epu_counter,
                              uint32_t edge_detect )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t cur_val, isel;

    epu_counter = epu_counter & EPU_COUNTER_NUM_COUNTERS_MASK;

    cur_val = IN_32( &pEpuRegs->ulEpccrn[ epu_counter ] );

    if( cur_val & EPU_COUNTER_ENABLE )
    {
        return -1;
    }

    isel = ( cur_val & EPU_COUNTER_ISEL_MASK ) >> EPU_COUNTER_ISEL_OFFSET;

    if( ( isel == EPU_COUNTER_INPUT_PLATFORM_CLK ) && edge_detect )
    {
        return -1;
    }

    cur_val &= ~( uint32_t ) ( EPU_COUNTER_EDE_MASK );
    cur_val |= ( edge_detect << EPU_COUNTER_EDE_OFFSET ) & EPU_COUNTER_EDE_MASK;

    OUT_32( &pEpuRegs->ulEpccrn[ epu_counter ], cur_val );

    return 0;
}

int iEpuCounterInputMuxSelect( uint8_t epu_counter,
                               enum epu_counter_input_select isel )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t cur_val, edge_detect;

    epu_counter = epu_counter & EPU_COUNTER_NUM_COUNTERS_MASK;

    cur_val = IN_32( &pEpuRegs->ulEpccrn[ epu_counter ] );

    if( cur_val & EPU_COUNTER_ENABLE )
    {
        return -1;
    }

    edge_detect = ( cur_val & EPU_COUNTER_EDE_MASK ) >>
                  EPU_COUNTER_EDE_OFFSET;

    if( ( isel == EPU_COUNTER_INPUT_PLATFORM_CLK ) && edge_detect )
    {
        return -1;
    }

    cur_val &= ~( uint32_t ) ( EPU_COUNTER_ISEL_MASK );
    cur_val |= isel << EPU_COUNTER_ISEL_OFFSET;

    OUT_32( &pEpuRegs->ulEpccrn[ epu_counter ], cur_val );

    return 0;
}

int iEpuCounterCfgCompare( uint8_t epu_counter,
                           uint32_t val )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t cur_val;

    epu_counter = epu_counter & EPU_COUNTER_NUM_COUNTERS_MASK;

    cur_val = IN_32( &pEpuRegs->ulEpccrn[ epu_counter ] );

    if( cur_val & EPU_COUNTER_ENABLE )
    {
        return -1;
    }

    OUT_32( pEpuRegs->ulEpcmPRn[ epu_counter ], val );

    return 0;
}

int iEpuCounterCfgLocalTrigger( uint8_t epu_counter,
                                enum epu_counter_event_trigger lt )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t cur_val;

    epu_counter = epu_counter & EPU_COUNTER_NUM_COUNTERS_MASK;

    cur_val = IN_32( &pEpuRegs->ulEpccrn[ epu_counter ] );

    if( cur_val & EPU_COUNTER_ENABLE )
    {
        return -1;
    }

    cur_val &= ~( uint32_t ) ( EPU_COUNTER_LEV_MASK );
    cur_val |= lt << EPU_COUNTER_LEV_OFFSET;

    OUT_32( pEpuRegs->ulEpccrn[ epu_counter ], cur_val );

    return 0;
}

int iEpuCounterCfgLocalAction( uint8_t epu_counter,
                               enum epu_counter_local_action action )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t cur_val;

    epu_counter = epu_counter & EPU_COUNTER_NUM_COUNTERS_MASK;

    cur_val = IN_32( &pEpuRegs->ulEpccrn[ epu_counter ] );

    if( cur_val & EPU_COUNTER_ENABLE )
    {
        return -1;
    }

    cur_val &= ~( uint32_t ) ( EPU_COUNTER_AC_MASK );
    cur_val |= action << EPU_COUNTER_AC_OFFSET;

    OUT_32( pEpuRegs->ulEpccrn[ epu_counter ], cur_val );

    return 0;
}

int iEpuCounterCfgGlobalTrigger( uint8_t epu_counter,
                                 enum epu_counter_event_trigger gt )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t cur_val;

    epu_counter = epu_counter & EPU_COUNTER_NUM_COUNTERS_MASK;

    cur_val = IN_32( &pEpuRegs->ulEpccrn[ epu_counter ] );

    if( cur_val & EPU_COUNTER_ENABLE )
    {
        return -1;
    }

    cur_val &= ~( uint32_t ) ( EPU_COUNTER_GEV_MASK );
    cur_val |= gt << EPU_COUNTER_GEV_OFFSET;

    OUT_32( pEpuRegs->ulEpccrn[ epu_counter ], cur_val );

    return 0;
}
