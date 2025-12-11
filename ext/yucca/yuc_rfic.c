// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2022 NXP
 */

#include <stdint.h>
#include <types.h>

#include "common.h"
#include "immap.h"
#include <FreeRTOS.h>
#include "task.h"

#include "yuc_rfic_common.h"
#include "yuc_rfic.h"
#include "yuc_rfic_cmd.h"
#include "patron_fem.h"

#ifdef YUCCA_LLCP_STUB
    u16 llcp_cmdif_space[ 20 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xf97f, 0x0, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff };
#endif

void vYucLlcpRficRegRW( u16 * val,
                        u16 addr,
                        u8 rw )
{
    #ifndef YUCCA_LLCP_STUB
        u32 uiLlcpRficAddr = pYucInfo->llcp_rfic_addr;
    #else
        u16 * uiLlcpRficAddr = ( llcp_cmdif_space );
    #endif

    /* During the LLCP standalone verification (Rattler LLCP + Venom
     * LLCP), it was noticed that the Rattler addressing scheme is AMBA APB
     * compliant while the Venom is not. For this reason, in case the
     * Rattler LLCP is used to communicate with the Venom LLCP, a special
     * care should be taken when addressing the venom memory map. The
     * address should be shifted one bit to the left.
     */
    addr = addr << 1;
    if( rw == LLCP_REG_W )
    {
        llcpRFIC_WRITE( ( uint32_t * ) ( uiLlcpRficAddr + addr ), *val );
        RF_LOGDBG( "Write 0x%x: 0x%x", ( addr >> 1 ), *val );
    }
    else
    {
        *val = llcpRFIC_READ( ( u32 * ) ( uiLlcpRficAddr + addr ) );
        RF_LOGDBG( "Read 0x%x: 0x%x", ( addr >> 1 ), *val );
    }

    return;
}

char * cYucRtcToStr( YucRtc_t uErrWd )
{
    switch( uErrWd )
    {
        RET_CASE( YUC_RTC_OK );
        RET_CASE( YUC_RTC_CMD_UNKNOWN );
        RET_CASE( YUC_RTC_CMD_WRONG_FLAGS );
        RET_CASE( YUC_RTC_TOO_FEW_DATAWORDS );
        RET_CASE( YUC_RTC_TOO_MANY_DATAWORDS );
        RET_CASE( YUC_RTC_INVALID_PARAMETER );
        RET_CASE( YUC_RTC_WRONG_SYS_STATE );
        RET_CASE( YUC_RTC_PLL_UNLOCKED );
        RET_CASE( YUC_RTC_PLL_WARNING );
        RET_CASE( YUC_RTC_PAENV_ERROR );
        RET_CASE( YUC_RTC_CAL_FAILED );
        RET_CASE( YUC_RTC_INTERNAL_ERROR );
        RET_CASE( YUC_RTC_BOOT_FAILED );
        RET_CASE( YUC_RTC_SET_FREQ );
        RET_CASE( YUC_RTC_PLL_NOT_LOCKED );
        RET_CASE( YUC_RTC_PLL_NOT_CALIBRATED );
        RET_CASE( YUC_RTC_TRANSITION_NOT_ALLOWED );
        RET_CASE( YUC_RTC_UNKNOWN_STATE );
        RET_CASE( YUC_RTC_OUT_OF_RANGE );
        RET_CASE( YUC_RTC_NOT_IMPLEMENTED );
        RET_CASE( YUC_RTC_NOT_MOUNTED );
        RET_CASE( YUC_RTC_REG_ACCESS_VIOLATION );
        RET_CASE( YUC_RTC_LOOKUP_FAILED );
        RET_CASE( YUC_RTC_WAIT_FAIL );
        RET_CASE( YUC_RTC_TIMEOUT );
        RET_CASE( YUC_RTC_CAL_RESISTOR_FAILED );
    }

    return YUC_RTC_UNKNOWN;
}

static BaseType_t xprvYucRficCmdCheck()
{
    YucCmd_t cmd;
    YucCmdResp_t resp = { 0 };
    u32 ulNumRead;
    __attribute__((unused)) u16 * cmd16 = ( u16 * ) &cmd;
    u16 uErrWd = 0;
    u32 ulNumRetry = YUC_RFIC_TF_RETRY;

    while( ulNumRetry )
    {
        vYucLlcpRficRegRW( ( ( u16 * ) &resp ), YUC_RFIC_CMD_RESP_ADDR, LLCP_REG_R );

        if( resp.tf && resp.id )
        {
            RF_LOGDBG( "cmd 0x%x - done", resp.id );

            /* Response has arrived, now check the error bit*/
            if( resp.ew )
            {
                /* Get responce rtc code */
                vYucLlcpRficRegRW( &uErrWd, YUC_RFIC_CMD_RESP_ADDR + 1,
                        LLCP_REG_R );
                RF_LOGERR( "cmd [0x%x] -RTC[code:%d,len:%d] %s", resp.id, uErrWd, resp.len, cYucRtcToStr( uErrWd ) );

                if( uErrWd != YUC_RTC_OK )
                {
                    /* Get error word [dw1] from response which contains further err details */
                    vYucLlcpRficRegRW( &uErrWd, YUC_RFIC_CMD_RESP_ADDR + 2,
                            LLCP_REG_R );
                    RF_LOGERR( "cmd [0x%x] -Error %s", resp.id, cYucRtcToStr( uErrWd ) );

                    for( ulNumRead = 2; ulNumRead <= resp.len; ulNumRead++ )
                    {
                        vYucLlcpRficRegRW( &uErrWd, YUC_RFIC_CMD_RESP_ADDR + ulNumRead,
                                LLCP_REG_R );
                        PRINTF(" resp[%x]=0x%x, ",ulNumRead, uErrWd);
                    }
                    return RF_SW_CMD_RESULT_RFIC_ERR;
                }
            }

            return RF_SW_CMD_RESULT_OK;
        }

        ulNumRetry--;

        /* here for high performance requirement
         * we may comment task yielding but effect will
         * none will be schedule till response from RFIC */
        taskYIELD();
    }

    if( !ulNumRetry && ( resp.ew || !resp.id ) )
    {
        RF_LOGERR( "Yuc rfic cmd [%x] failed", resp.id );
        return RF_SW_CMD_RESULT_RFIC_TIMEOUT;
    }
    return RF_SW_CMD_RESULT_OK;
}

static BaseType_t xprvYucRficCmdSend( u32 ulCmdData,
                            u32 ulRwWds,
                            u16 * uWords,
                            u8 ucNumWds )
{
    YucCmd_t cmd;
    u32 ulNumRead;
    __attribute__((unused)) u16 * cmd16 = ( u16 * ) &cmd;

    if( ulRwWds == YUC_RFIC_CMD_ADDR )
    {
        cmd.len = ucNumWds;
        cmd.id = ulCmdData & YUC_CMD_ID_MASK;
        cmd.sd = 0;
        cmd.rrq = 1;
        sync_dmb();
        vYucLlcpRficRegRW( ( ( u16 * ) &cmd ), ulRwWds, LLCP_REG_W );

#if RF_DEBUG
        RF_LOGDBG( "cmd 0x%x, numwrd %d", *cmd16, ucNumWds );
        for( ulNumRead = 0; ulNumRead < ucNumWds; ulNumRead++ )
        {
            PRINTF( " input words : [%d:0x%x] ", ulNumRead, uWords[ulNumRead] );
        }
#endif

        for( ulNumRead = 1; ulNumRead <= ucNumWds; ulNumRead++ )
        {
            vYucLlcpRficRegRW( uWords, ( ulRwWds + ulNumRead ), LLCP_REG_W );
            sync_dmb();
            uWords += 1;
        }

#ifdef YUCCA_LLCP_STUB
        RF_LOGDBG( "dw: [0:0x%x][1:0x%x][2:0x%x][3:0x%x][4:0x%x][5:0x%x][6:0x%x][7:0x%x][8:0x%x][9:0x%x]",
                llcp_cmdif_space[ 0 ],
                llcp_cmdif_space[ 1 ],
                llcp_cmdif_space[ 2 ],
                llcp_cmdif_space[ 3 ],
                llcp_cmdif_space[ 4 ],
                llcp_cmdif_space[ 5 ],
                llcp_cmdif_space[ 6 ],
                llcp_cmdif_space[ 7 ],
                llcp_cmdif_space[ 8 ],
                llcp_cmdif_space[ 9 ]
                );
#endif /* ifdef YUCCA_LLCP_STUB */

        sync_dmb();
    }
    else /* read response command */
    {
        for( ulNumRead = 1; ulNumRead <= ucNumWds; ulNumRead++ )
        {
            vYucLlcpRficRegRW( uWords, ( ulRwWds + ulNumRead ),
                               LLCP_REG_R );
            uWords += 1;
        }
    }

    return RF_SW_CMD_RESULT_OK;
}

BaseType_t xYucRficCmdProc( u32 ulCmdData,
                            u32 ulRwWds,
                            u16 * uWords,
                            u8 ucNumWds )
{
    uint32_t ret = RF_SW_CMD_RESULT_OK;
    if ( pYucInfo->eFR1Mode & eFR1Mode2t2r0 )
    {
        pYucInfo->llcp_rfic_addr = pYucInfo->llcp_rfic1_addr;
        ret = xprvYucRficCmdSend(ulCmdData, ulRwWds, uWords, ucNumWds);
        if (ret)
        {
            RF_LOGERRMSG("CMD failed to execute on RFIC1");
            return ret;
        }
    }
    if ( pYucInfo->eFR1Mode & eFR1Mode2t2r1 )
    {
        pYucInfo->llcp_rfic_addr = pYucInfo->llcp_rfic2_addr;
        ret = xprvYucRficCmdSend(ulCmdData, ulRwWds, uWords, ucNumWds);
        if (ret)
        {
            RF_LOGERRMSG("CMD failed to execute on RFIC2");
            return ret;
        }
    }

    if( ulRwWds == YUC_RFIC_CMD_ADDR )
    {
        if ( pYucInfo->eFR1Mode & eFR1Mode2t2r0 )
        {
            pYucInfo->llcp_rfic_addr = pYucInfo->llcp_rfic1_addr;
            ret = xprvYucRficCmdCheck();
            if (ret)
            {
                RF_LOGERRMSG("CMD check failed to on RFIC1");
                return ret;
            }
        }
        if ( pYucInfo->eFR1Mode & eFR1Mode2t2r1 )
        {
            pYucInfo->llcp_rfic_addr = pYucInfo->llcp_rfic2_addr;
            ret = xprvYucRficCmdCheck();
            if (ret)
            {
                RF_LOGERRMSG("CMD check failed to execute on RFIC2");
                return ret;
            }
        }
    }
    return ret;
}
