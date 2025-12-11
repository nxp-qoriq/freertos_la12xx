// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2023 NXP
 */

#include "rf_dev.h"

#include "yuc_init.h"
#include "yuc_rfic.h"
#include "yuc_rfic_cmd.h"
#include "patron_fem.h"
#include "gpio.h"
#include "yuc_rfic_types.h"
#include <yuc_rfic_common.h>

#define LLCP_IOCR_ADDR ( 0x1FF8000 + 0xB0 )
#define LLCP_ANALOG_CTRL_ADDR ( 0x1FF8000 + 0xAC )

/* Configuring LLCPIOCR register: */
/* | Bit|31     | 1 to override LLCPIOSR                                 | */
/* | Bit|23-20  | 0 – loopback testing disabled for all TX IO.           | */
/* | Bit| 19    | 1 – LLCP2_STOUT internal termination 100Ohm is enabled | */
/* | Bit| 18-17 | 00 - LLCP2_STOUT Pre-emphasis disabled                 | */
/* | Bit| 16    | 1 – LLCP2_DOUT internal termination 100Ohm is enabled  | */
/* | Bit| 15-14 | 00 - LLCP2_DOUT Pre-emphasis disabled                  | */
/* | Bit| 13    | 1 – LLCP1_STOUT internal termination 100Ohm is enabled | */
/* | Bit| 12-11 | 00 – LLCP1_STOUT Pre-emphasis disabled                 | */
/* | Bit| 10    | 1 – LLCP1_DOUT internal termination 100Ohm is enabled  | */
/* | Bit| 9-8   | 00 – LLCP1_DOUT Pre-emphasis disabled                  | */
/* | Bit| 7-4   | 0000 for required differential swing                   | */
/* | Bit| 3     | 1 TX_AURORA_MODE to reduce duty cycle distortion       | */
/* | Bit| 2-1   | 00 – test mode disabled                                | */
/* | Bit| 0     | 1 – Current Reference Cell is enabled                  | */
/**
 * | 30|                 20|                 10|                  0|
 * |1|0|9|8|7|6|5|4|3|2|1|0|9|8|7|6|5|4|3|2|1|0|9|8|7|6|5|4|3|2|1|0|
 *  1 0 0 0 0 0 0 0 0 0 0 0 1 0 0 1 0 0 1 0 0 1 0 0 0 0 0 0 1 0 0 1
 *        8       0       0       9       2       4       0       9
 **/

#define LLCP_IOCR_VAL 0x80092609
#define LLCP_AN_CTRL_VAL 0x08080808

#if 0
/** Use when these bits need to be changed */
#define LLCPIOCR_OVERRIDE (1 << 31)
#define LLCPIOCR_LBTEST (0x0 << 20) /* Valid values 0x0 - 0xf */
#define LLCPIOCR_STOUT2_INTTERM ( 1 << 19 )
#define LLCPIOCR_STOUT2_PREEMPH ( 0x0 << 17 ) /* Valid values 0,1,2,3 */
#define LLCPIOCR_DOUT2_INTTERM ( 1 << 16 )
#define LLCPIOCR_DOUT2_PREEMPH ( 0x0 << 14 ) /* Valid values 0,1,2,3 */
#define LLCPIOCR_STOUT1_INTTERM ( 1 << 13 )
#define LLCPIOCR_STOUT1_PREEMPH ( 0x0 << 11 ) /* Valid values 0,1,2,3 */
#define LLCPIOCR_DOUT1_INTTERM ( 1 << 10 )
#define LLCPIOCR_DOUT1_PREEMPH ( 0x0 << 8 ) /* Valid values 0,1,2,3 */
#define LLCPIOCR_DIFF_SWING ( 0x0 << 4 )
#define LLCPIOCR_TXAURORA ( 1 << 3 )
#define LLCPIOCR_TEST ( 0x0 << 1 ) /* Valid values are 0,1,2,3 */
#define LLCPIOCR_CURREF_CELL ( 0x1 )

#define LLCPIOCR_VAL ( \
          ( LLCPIOCR_OVERRIDE )   \
        | ( LLCPIOCR_LBTEST )  \
        | ( LLCPIOCR_STOUT2_INTTERM )  \
        | ( LLCPIOCR_STOUT2_PREEMPH )  \
        | ( LLCPIOCR_DOUT2_INTTERM )  \
        | ( LLCPIOCR_DOUT2_PREEMPH )  \
        | ( LLCPIOCR_STOUT1_INTTERM )  \
        | ( LLCPIOCR_STOUT1_PREEMPH )  \
        | ( LLCPIOCR_DOUT1_INTTERM )  \
        | ( LLCPIOCR_DOUT1_PREEMPH )  \
        | ( LLCPIOCR_DIFF_SWING )  \
        | ( LLCPIOCR_TXAURORA )  \
        | ( LLCPIOCR_TEST )  \
        | ( LLCPIOCR_CURREF_CELL )  \
        )
#endif


extern void lSetGPIO2Defaults( void );

int32_t iYucInitLLCP()
{
    u32 llcpiocr_val = 0;
    llcpiocr_val |= LLCP_IOCR_VAL;
    OUT_32 ( ( CCSR_BASE_ADDR + LLCP_IOCR_ADDR ) , LLCP_IOCR_VAL);
    OUT_32 ( ( CCSR_BASE_ADDR + LLCP_ANALOG_CTRL_ADDR) , LLCP_AN_CTRL_VAL);
    RF_LOGDBG("LLCP_IOCR_ADDR Val = 0x%x", IN_32((unsigned int *)(CCSR_BASE_ADDR + LLCP_IOCR_ADDR)));
    RF_LOGDBG("LLCP_ANALOG_CTRL_ADDR Val = 0x%x", IN_32((unsigned int *)(CCSR_BASE_ADDR + LLCP_ANALOG_CTRL_ADDR)));

    return pdPASS;
}

int32_t iYucInit( RficHandle_t pxRFDevice)
{
    int32_t iRet = 0;
    uint32_t ulCoreId = ulMpicCurrentCore();
    if ( iFemGpioInit() )
    {
        RF_DEV_INIT_DEATH_LOOP( ulCoreId );
    }


    iYucInitLLCP();

    pYucInfo = malloc( sizeof( YucRfInfo_t ) );

    if( !pYucInfo )
    {
        RF_LOGERRMSG( "YucRfInfo Malloc failed" );
        return -1;
    }
    
    pYucInfo->llcp_rfic_addr = YUC_LLCP1_ADDR;
    pYucInfo->llcp_rfic1_addr = YUC_LLCP1_ADDR;
    pYucInfo->llcp_rfic2_addr = YUC_LLCP2_ADDR;
    pYucInfo->eFR1Mode = eFR1Mode1t1r0;
    mod_mem_region_t *ddr_addr_m = (mod_mem_region_t *)
				bsp_get_mem_region(MOD_MEM_SCRATCH_BUF);
    pYucInfo->cal_data = (yuc_cal_data_t *)(ddr_addr_m->addr_v + get_modem_rf_data_offset());

    pYucInfo->state = YUC_SS_STANDBY;
    memset(&(pYucInfo->state_data), 0, sizeof(yucStateData_t));
    pYucInfo->state_data.path = YUC_PATH_RX1_RX2_TX1_TX2;
    pYucInfo->state_data.pathRssi = YUC_RX_1_2;
    pYucInfo->state_data.pathBand = YUC_BAND_MB;
    pYucInfo->yucData[FR1_IDX_YC1 - 1] = (yucCurData_t *)(pxRFDevice->rfic_priv);
    pYucInfo->yucData[FR1_IDX_YC2 - 1] = (yucCurData_t *)(pxRFDevice->rfic_priv + sizeof(yucCurData_t));
    pYucInfo->yucData[FR1_IDX_YC1 - 1]->txGainIdx = 0;
    pYucInfo->yucData[FR1_IDX_YC2 - 1]->txGainIdx = 0;
    pYucInfo->yucData[FR1_IDX_YC1 - 1]->rxGainIdx = 0;
    pYucInfo->yucData[FR1_IDX_YC2 - 1]->rxGainIdx = 0;
    return iRet;
}

int32_t iYucDeInit( void )
{
    int32_t iRet = 0;

    free( pYucInfo );

    return iRet;
}
