// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2017-2021 NXP
 */

#include <FreeRTOS.h>
#include <task.h>

#include "epu.h"

int iEpuEvtToScuEvt( enum epu_event_input_sel event_mask,
        struct scu_event *scu_event )
{
    /* TODO: Populate SCU EVENT to SCU conversion */
    scu_event->isel_val = event_mask / 4;
    scu_event->isel = event_mask % 4;

    return 0;
}

int iEpuScuSetInputEvent( enum epu_scu_sel scu_sel,
                          enum epu_event_input_sel event_mask,
                          enum epu_scu_input_sel * input_sel )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t epecrn = 0, epsmcrn = 0, ic = 0, new_ic = 0, isel_val = 0,
             mask = 0;
    struct scu_event scu_event;
    int ret = 0;

    epecrn = IN_32( &pEpuRegs->ulEpecrn[ scu_sel ] );
    iEpuEvtToScuEvt( event_mask, &scu_event );

    switch( scu_event.isel )
    {
        case EPU_SCU_INPUT_SEL0:
            ic = ( epecrn >> SCU_EPECR_IC0_SHIFT ) &
                 SCU_EPECR_IC_MASK;
            new_ic = ( uint32_t ) ( SCU_EPECR_IC_SUFFICIENT <<
                                    SCU_EPECR_IC0_SHIFT );
            isel_val = ( scu_event.isel_val & SCU_EPSMCR_ISEL_MASK )
                       << SCU_EPSMCR_ISEL0_SHIFT;
            mask = ~( ( uint32_t ) SCU_EPSMCR_ISEL_MASK <<
                      SCU_EPSMCR_ISEL0_SHIFT );
            break;

        case EPU_SCU_INPUT_SEL1:
            ic = ( epecrn >> SCU_EPECR_IC1_SHIFT ) &
                 SCU_EPECR_IC_MASK;
            new_ic = ( uint32_t ) ( SCU_EPECR_IC_SUFFICIENT <<
                                    SCU_EPECR_IC1_SHIFT );
            isel_val = ( scu_event.isel_val & SCU_EPSMCR_ISEL_MASK )
                       << SCU_EPSMCR_ISEL1_SHIFT;
            mask = ~( ( uint32_t ) SCU_EPSMCR_ISEL_MASK <<
                      SCU_EPSMCR_ISEL1_SHIFT );
            break;

        case EPU_SCU_INPUT_SEL2:
            ic = ( epecrn >> SCU_EPECR_IC2_SHIFT ) &
                 SCU_EPECR_IC_MASK;
            new_ic = ( uint32_t ) ( SCU_EPECR_IC_SUFFICIENT <<
                                    SCU_EPECR_IC2_SHIFT );
            isel_val = ( scu_event.isel_val & SCU_EPSMCR_ISEL_MASK )
                       << SCU_EPSMCR_ISEL2_SHIFT;
            mask = ~( ( uint32_t ) SCU_EPSMCR_ISEL_MASK <<
                      SCU_EPSMCR_ISEL2_SHIFT );
            break;

        case EPU_SCU_INPUT_SEL3:
            ic = ( epecrn >> SCU_EPECR_IC3_SHIFT ) &
                 SCU_EPECR_IC_MASK;
            new_ic = ( uint32_t ) ( SCU_EPECR_IC_SUFFICIENT <<
                                    SCU_EPECR_IC3_SHIFT );
            isel_val = ( scu_event.isel_val & SCU_EPSMCR_ISEL_MASK )
                       << SCU_EPSMCR_ISEL3_SHIFT;
            mask = ~( ( uint32_t ) SCU_EPSMCR_ISEL_MASK <<
                      SCU_EPSMCR_ISEL3_SHIFT );

            break;
    }

    if( ic != SCU_EPECR_IC_DISABLED )
    {
        ret = -1;
        goto out;
    }

    *input_sel = ( enum epu_scu_input_sel ) scu_event.isel;

    epsmcrn = IN_32( &pEpuRegs->ulEpsmCRN[ scu_sel ].ulEpsmCR );
    epsmcrn &= mask;
    epsmcrn |= isel_val;
    epecrn |= new_ic;

    OUT_32( &pEpuRegs->ulEpsmCRN[ scu_sel ].ulEpsmCR, epsmcrn );
    OUT_32( &pEpuRegs->ulEpecrn[ scu_sel ], epecrn );
out:

    return ret;
}

int iEpuScuClearInputEvent( enum epu_scu_sel scu_sel,
                            enum epu_scu_input_sel input_sel )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t epecrn;
    int ret = 0;

    epecrn = IN_32( &pEpuRegs->ulEpecrn[ scu_sel ] );

    switch( input_sel )
    {
        case EPU_SCU_INPUT_SEL0:
            epecrn &= ~( uint32_t ) ( SCU_EPECR_IC_MASK << SCU_EPECR_IC0_SHIFT );
            break;

        case EPU_SCU_INPUT_SEL1:
            epecrn &= ~( uint32_t ) ( SCU_EPECR_IC_MASK << SCU_EPECR_IC1_SHIFT );
            break;

        case EPU_SCU_INPUT_SEL2:
            epecrn &= ~( uint32_t ) ( SCU_EPECR_IC_MASK << SCU_EPECR_IC2_SHIFT );
            break;

        case EPU_SCU_INPUT_SEL3:
            epecrn &= ~( uint32_t ) ( SCU_EPECR_IC_MASK << SCU_EPECR_IC3_SHIFT );
            break;

        default:
            ret = -1;
            goto out;
    }

    OUT_32( &pEpuRegs->ulEpecrn[ scu_sel ], epecrn );
out:
    return ret;
}

int iEpuScuDisableAllEvents( enum epu_scu_sel scu_sel )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t epecrn;
    int ret = 0;

    epecrn = IN_32( &pEpuRegs->ulEpecrn[ scu_sel ] );
    epecrn &= ~( SCU_EPECR_ALL_IC_MASK << SCU_EPECR_IC3_SHIFT );
    OUT_32( &pEpuRegs->ulEpecrn[ scu_sel ], epecrn );

    return ret;
}

int iEpuScuCfgInputControl( enum epu_scu_sel scu_sel,
                            enum epu_scu_input_sel input_sel,
                            enum epu_scu_input_control_sel ctrl_sel )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t epecrn;
    int ret = 0;

    epecrn = IN_32( &pEpuRegs->ulEpecrn[ scu_sel ] );

    switch( input_sel )
    {
        case EPU_SCU_INPUT_SEL0:
            epecrn &= ~( ctrl_sel << SCU_EPECR_IC0_SHIFT );
            break;

        case EPU_SCU_INPUT_SEL1:
            epecrn &= ~( ctrl_sel << SCU_EPECR_IC1_SHIFT );
            break;

        case EPU_SCU_INPUT_SEL2:
            epecrn &= ~( ctrl_sel << SCU_EPECR_IC2_SHIFT );
            break;

        case EPU_SCU_INPUT_SEL3:
            epecrn &= ~( ctrl_sel << SCU_EPECR_IC3_SHIFT );
            break;

        default:
            ret = -1;
            goto out;
    }

    OUT_32( &pEpuRegs->ulEpecrn[ scu_sel ], epecrn );
out:

    return ret;
}

int iEpuScuSetEdgeDetect( enum epu_scu_sel scu_sel )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t epecrn;

    epecrn = IN_32( &pEpuRegs->ulEpecrn[ scu_sel ] );
    epecrn |= ( SCU_EPECR_EDE_EN << SCU_EPECR_EDE_SHIFT );
    OUT_32( &pEpuRegs->ulEpecrn[ scu_sel ], epecrn );

    return 0;
}

int iEpuScuClrEdgeDetect( enum epu_scu_sel scu_sel )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t epecrn;

    epecrn = IN_32( &pEpuRegs->ulEpecrn[ scu_sel ] );
    epecrn &= ( SCU_EPECR_EDE_EN << SCU_EPECR_EDE_SHIFT );
    OUT_32( &pEpuRegs->ulEpecrn[ scu_sel ], epecrn );

    return 0;
}

int iEpuScuSetInputInversion( enum epu_scu_sel scu_sel,
                              enum epu_scu_input_sel input_sel )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t epecrn;
    int ret = 0;

    epecrn = IN_32( &pEpuRegs->ulEpecrn[ scu_sel ] );

    switch( input_sel )
    {
        case EPU_SCU_INPUT_SEL0:
            epecrn |= ( uint32_t ) ( SCU_EPECR_IIE_EN << SCU_EPECR_IIE0_SHIFT );
            break;

        case EPU_SCU_INPUT_SEL1:
            epecrn |= ( uint32_t ) ( SCU_EPECR_IIE_EN << SCU_EPECR_IIE1_SHIFT );
            break;

        case EPU_SCU_INPUT_SEL2:
            epecrn |= ( uint32_t ) ( SCU_EPECR_IIE_EN << SCU_EPECR_IIE2_SHIFT );
            break;

        case EPU_SCU_INPUT_SEL3:
            epecrn |= ( uint32_t ) ( SCU_EPECR_IIE_EN << SCU_EPECR_IIE3_SHIFT );
            break;

        default:
            ret = -1;
            goto out;
    }

    OUT_32( &pEpuRegs->ulEpecrn[ scu_sel ], epecrn );
out:
    return ret;
}

int iEpuScuClrInputInversion( enum epu_scu_sel scu_sel,
                              enum epu_scu_input_sel input_sel )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t epecrn;
    int ret = 0;

    epecrn = IN_32( &pEpuRegs->ulEpecrn[ scu_sel ] );

    switch( input_sel )
    {
        case EPU_SCU_INPUT_SEL0:
            epecrn &= ~( uint32_t ) ( SCU_EPECR_IIE_EN << SCU_EPECR_IIE0_SHIFT );
            break;

        case EPU_SCU_INPUT_SEL1:
            epecrn &= ~( uint32_t ) ( SCU_EPECR_IIE_EN << SCU_EPECR_IIE1_SHIFT );
            break;

        case EPU_SCU_INPUT_SEL2:
            epecrn &= ~( uint32_t ) ( SCU_EPECR_IIE_EN << SCU_EPECR_IIE2_SHIFT );
            break;

        case EPU_SCU_INPUT_SEL3:
            epecrn &= ~( uint32_t ) ( SCU_EPECR_IIE_EN << SCU_EPECR_IIE3_SHIFT );
            break;

        default:
            ret = -1;
            goto out;
    }

    OUT_32( &pEpuRegs->ulEpecrn[ scu_sel ], epecrn );
out:
    return ret;
}

int iEpuScuSetInversion( enum epu_scu_sel scu_sel )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t epecrn;

    epecrn = IN_32( &pEpuRegs->ulEpecrn[ scu_sel ] );
    epecrn |= ( uint32_t ) ( SCU_EPECR_ICE_EN << SCU_EPECR_ICE_SHIFT );
    OUT_32( &pEpuRegs->ulEpecrn[ scu_sel ], epecrn );

    return 0;
}

int iEpuScuClrInversion( enum epu_scu_sel scu_sel )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t epecrn;

    epecrn = IN_32( &pEpuRegs->ulEpecrn[ scu_sel ] );
    epecrn &= ~( uint32_t ) ( SCU_EPECR_ICE_EN << SCU_EPECR_ICE_SHIFT );
    OUT_32( &pEpuRegs->ulEpecrn[ scu_sel ], epecrn );

    return 0;
}

int iEpuScuSetSticky( enum epu_scu_sel scu_sel )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t epecrn;

    epecrn = IN_32( &pEpuRegs->ulEpecrn[ scu_sel ] );
    epecrn |= ( uint32_t ) ( SCU_EPECR_SSE_EN << SCU_EPECR_SSE_SHIFT );
    OUT_32( &pEpuRegs->ulEpecrn[ scu_sel ], epecrn );

    return 0;
}

int iEpuScuClrSticky( enum epu_scu_sel scu_sel )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t epecrn;

    epecrn = IN_32( &pEpuRegs->ulEpecrn[ scu_sel ] );
    epecrn &= ~( uint32_t ) ( SCU_EPECR_SSE_EN << SCU_EPECR_SSE_SHIFT );
    OUT_32( &pEpuRegs->ulEpecrn[ scu_sel ], epecrn );

    return 0;
}

int iEpuScuSetEventStatus( enum epu_scu_sel scu_sel )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t epecrn;

    epecrn = IN_32( &pEpuRegs->ulEpecrn[ scu_sel ] );
    epecrn |= ( uint32_t ) ( SCU_EPECR_STS_EN << SCU_EPECR_STS_SHIFT );
    OUT_32( &pEpuRegs->ulEpecrn[ scu_sel ], epecrn );

    return 0;
}

int iEpuScuGetEventStatus( enum epu_scu_sel scu_sel )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t epecrn;
    int ret;

    epecrn = IN_32( &pEpuRegs->ulEpecrn[ scu_sel ] );

    if( epecrn & ( SCU_EPECR_STS_EN << SCU_EPECR_STS_SHIFT ) )
    {
        ret = 1;
    }
    else
    {
        ret = 0;
    }

    return ret;
}

int iEpuScuClrEventStatusNoLock( enum epu_scu_sel scu_sel )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t epecrn;

    epecrn = IN_32( &pEpuRegs->ulEpecrn[ scu_sel ] );
    epecrn &= ~( uint32_t ) ( SCU_EPECR_STS_EN << SCU_EPECR_STS_SHIFT );
    OUT_32( &pEpuRegs->ulEpecrn[ scu_sel ], epecrn );

    return 0;
}

int iEpuScuEventOutputToGpin( enum epu_scu_sel scu_sel,
                              enum epu_scu_core_cts bit )
{
    EpuRegs_t * pEpuRegs = ( EpuRegs_t * ) ( EPU_BASE );
    uint32_t epevctrn;

    epevctrn = ( uint32_t ) ( scu_sel << SCU_EPEVTCR_SCU_SEL_SHIFT );
    epevctrn |= SCU_EPEVTCR_DIR_EN;

    OUT_32( &pEpuRegs->ulEpevtCRn[ bit ], epevctrn );

    return 0;
}
