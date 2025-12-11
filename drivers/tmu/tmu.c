// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2024 NXP
 */

#include "types.h"
#include "tmu.h"
#include "immap.h"
#include "FreeRTOS.h"
#include "task.h"
#include "gul_host_if.h"
#include "ppc.h"
#include "Time.h"

#if MTD_I2C_DIODE_TEMP
#include "tmu_i2c.h"
#endif

#if MTD_I2C_PWR_INFO
#include "ina220_api.h"
#endif

TmuRegs_t * pTmuHandle = NULL;
mtdThermalHandler_t tmuCallBackHandler[ MAX_TMU_USER ];
rfTempFn_t rfTempFunc = NULL;
rfTempIrqEnableFn_t rfTempIrqEnableFunc = NULL;
gul_mod_priv_t * pGulModPriv;
uint32_t mtdThreshold_count = 0;
uint32_t mtd_hysteresis = 0;

static void sendNotification( enum mtd_eventID tmuEvent )
{
    struct mtd_thermalEvent tmuCallBackData;
    struct gul_msi_info * pMsiInfo;
    int16_t temp = mtdGetTemp();
    uint32_t msiLine = 0xFFFFFFFF;

    pMsiInfo = &pGulModPriv->msi_info[ MSI_IRQ_MUX ];
    msiLine = in_le32( &pGulModPriv->pHif->msi_mtd_tvd );
    tmuCallBackData.tmuEvent = tmuEvent;
    tmuCallBackData.curTemp.temp = temp;

    for( uint8_t i = 0; i < MAX_TMU_USER; i++ )
    {
        if( tmuCallBackHandler[ i ] != NULL )
        {
            tmuCallBackHandler[ i ]( &tmuCallBackData );
        }
    }

    out_le32( &pGulModPriv->pHif->mtdThermalEvent.tmuEvent, tmuEvent );
    out_le32( &pGulModPriv->pHif->mtdThermalEvent.curTemp.temp, temp );

    if( msiLine < GUL_MSI_MAX_CNT )
    {
        out_le32( pMsiInfo[ msiLine ].addr, pMsiInfo[ msiLine ].data );
    }
}
bool_t tmuIsrAverageTempThreshold( uint32_t ulIrqNo,
                                   void * pvDevData )
{
    uint32_t tmuStatus;
    bool_t status = false;

    ( void ) ulIrqNo;
    ( void ) pvDevData;
    tmuStatus = in_le32( &pTmuHandle->tidr );

	/* ERR052243: If raising or falling edge happens invalid temp. */
    if (tmuStatus & TMU_TIDR_FALL_RISE_MASK) {
        out_le32( &pTmuHandle->tidr, TMU_TIDR_FALL_RISE_MASK);
        return false;
    }

    if( ( tmuStatus & 0x40000000 ) || ( tmuStatus & 0x08000000 ) )
    {
        if( (tmuStatus & 0x40000000) && ( in_le32( &pTmuHandle->tier ) & TMU_TIER_AHT_ENABLE ) )
        {
            if( MIN_THRESHOLD_COUNT == mtdThreshold_count )
            {
                out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) & TMU_TIER_AHT_DISABLE ) );
                out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) | TMU_TIER_ALT_ENABLE ) );
            }

            sendNotification( TMU_HIGH_TEMP_EVENT );
            out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) & TMU_TIER_AHT_DISABLE ) );
            out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) | TMU_TIER_ALT_ENABLE ) );
            out_le32( &pTmuHandle->tidr, ( in_le32( &pTmuHandle->tidr ) | ( TMU_TIER_AHT_ENABLE ) ) );
        }
	else if( (tmuStatus & 0x08000000) && ( in_le32( &pTmuHandle->tier ) & TMU_TIER_ALT_ENABLE ) )
        {
            if( MIN_THRESHOLD_COUNT == mtdThreshold_count )
            {
                out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) | TMU_TIER_AHT_ENABLE ) );
                out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) & TMU_TIER_ALT_DISABLE ) );
            }

            sendNotification( TMU_LOW_TEMP_EVENT );
            out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) & TMU_TIER_ALT_DISABLE ) );
            out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) | TMU_TIER_AHT_ENABLE ) );
            out_le32( &pTmuHandle->tidr, ( in_le32( &pTmuHandle->tidr ) | ( TMU_TIER_ALT_ENABLE ) ) );
        }

        status = true;
    }

    return status;
}

bool_t tmuIsrCriticalTempThreshold( uint32_t ulIrqNo,
                                    void * pvDevData )
{
    uint32_t tmuStatus;
    bool_t status = false;

    ( void ) ulIrqNo;
    ( void ) pvDevData;
    tmuStatus = in_le32( &pTmuHandle->tidr );

	/* ERR052243: If raising or falling edge happens invalid temp. */
    if (tmuStatus & TMU_TIDR_FALL_RISE_MASK) {
        out_le32( &pTmuHandle->tidr, TMU_TIDR_FALL_RISE_MASK);
        return false;
    }

    if( (tmuStatus & 0x20000000) && (in_le32( &pTmuHandle->tier ) & TMU_TIER_HTC_ENABLE))
    {
        out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) & TMU_TIER_HTC_DISABLE ) );
        out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) | TMU_TIER_LTC_ENABLE ) );

        sendNotification( TMU_HIGH_CRITICAL_TEMP_EVENT );
        out_le32( &pTmuHandle->tidr, ( in_le32( &pTmuHandle->tidr ) | ( TMU_TIER_HTC_ENABLE ) ) );
        status = true;
    }
    else if( tmuStatus & 0x04000000 && (in_le32( &pTmuHandle->tier ) & TMU_TIER_LTC_ENABLE))
    {
        out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) & TMU_TIER_LTC_DISABLE ) );
        out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) | TMU_TIER_HTC_ENABLE ) );

	sendNotification( TMU_LOW_CRIICAL_TEMP_EVENT );
        out_le32( &pTmuHandle->tidr, ( in_le32( &pTmuHandle->tidr ) | ( TMU_TIER_LTC_ENABLE ) ) );
        status = true;
    }
    else
    {
        /* Do nothing */
    }
    return status;
}

static inline void tmuRegisterIRQ( uint8_t xIrqNo,
                                   bIsrFunc bIsr )
{
    int ulReturn = lRegisterIrq( ( uint32_t ) ( INTERNAL_IRQ_OFFSET + xIrqNo ), bIsr, NULL );

    if( ulReturn > 0 )
    {
        bMpicEnable( DEVICE_INTERNAL, xIrqNo );
    }
}

void tmuEnableInterrupt( void )
{
    uint32_t highThresholdVal_high = 0, highThresholdVal_low = 0;
    uint32_t lowThresholdVal_high = 0, lowThresholdVal_low = 0;
    mtdThreshold_count = in_le32( &pGulModPriv->pHif->mtd_threshold.threshold_count );
    /* Disable monitoring*/
    out_le32( &pTmuHandle->tmr, TMU_TMR_DISABLE );
    tmuRegisterIRQ( TMU_ALARM_IRQ_NO, tmuIsrAverageTempThreshold );
    tmuRegisterIRQ( TMU_CRITICAL_ALARM_IRQ_NO, tmuIsrCriticalTempThreshold );

    if( MIN_THRESHOLD_COUNT == mtdThreshold_count )
    {
        lowThresholdVal_high = TEMP_CELSIUS_TO_KELVIN( in_le32( &pGulModPriv->pHif->mtd_threshold.threshold[ 0 ] ) + mtd_hysteresis );
        lowThresholdVal_low = TEMP_CELSIUS_TO_KELVIN( in_le32( &pGulModPriv->pHif->mtd_threshold.threshold[ 0 ] ) - mtd_hysteresis );
        /* Enabling average high temperature threshold interrupt*/
	vUdelay(100);
        out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) | TMU_TIER_AHT_ENABLE ) );
        out_le32( &pTmuHandle->tiascr, TMU_TIASCR_ENABLE );
        out_le32( &pTmuHandle->tmhtatr, ( lowThresholdVal_high | TMU_TEMP_VALID ) );
        /* Enabling average low temperature threshold interrupt*/
        out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) & TMU_TIER_ALT_DISABLE ) );
        out_le32( &pTmuHandle->tiascr, TMU_TIASCR_ENABLE );
        out_le32( &pTmuHandle->tmltatr, ( lowThresholdVal_low | TMU_TEMP_VALID ) );
        out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) & TMU_TIER_LTC_DISABLE ) );
        out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) & TMU_TIER_HTC_DISABLE ) );
    }
    else if( MAX_THRESHOLD_COUNT == mtdThreshold_count )
    {
        lowThresholdVal_high = TEMP_CELSIUS_TO_KELVIN( in_le32( &pGulModPriv->pHif->mtd_threshold.threshold[ 0 ] ) + mtd_hysteresis );
        lowThresholdVal_low = TEMP_CELSIUS_TO_KELVIN( in_le32( &pGulModPriv->pHif->mtd_threshold.threshold[ 0 ] ) - mtd_hysteresis );
        highThresholdVal_high = TEMP_CELSIUS_TO_KELVIN( in_le32( &pGulModPriv->pHif->mtd_threshold.threshold[ 1 ] ) + mtd_hysteresis );
        highThresholdVal_low = TEMP_CELSIUS_TO_KELVIN( in_le32( &pGulModPriv->pHif->mtd_threshold.threshold[ 1 ] ) - mtd_hysteresis );
	/* Enabling high temperature critical threshold interrupt*/
	vUdelay(100);
        out_le32( &pTmuHandle->ticscr, TMU_TICSCR_ENABLE );
        out_le32( &pTmuHandle->tmhtactr, ( highThresholdVal_high | TMU_TEMP_VALID ) );
        /* Enabling average high temperature threshold interrupt*/
        out_le32( &pTmuHandle->tiascr, TMU_TIASCR_ENABLE );
        out_le32( &pTmuHandle->tmhtatr, ( lowThresholdVal_high | TMU_TEMP_VALID ) );

        /* Enabling low temperature critical threshold interrupt*/
        out_le32( &pTmuHandle->ticscr, TMU_TICSCR_ENABLE );
        out_le32( &pTmuHandle->tmltactr, ( highThresholdVal_low | TMU_TEMP_VALID ) );
        /* Enabling average low temperature threshold interrupt*/
        out_le32( &pTmuHandle->tiascr, TMU_TIASCR_ENABLE );
        out_le32( &pTmuHandle->tmltatr, ( lowThresholdVal_low | TMU_TEMP_VALID ) );

	out_le32( &pTmuHandle->tidr, ( in_le32( &pTmuHandle->tidr )));
	out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) | TMU_TIER_HTC_ENABLE ) );
	out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) | TMU_TIER_AHT_ENABLE ) );
	out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) | TMU_TIER_LTC_ENABLE ) );
	out_le32( &pTmuHandle->tier, ( in_le32( &pTmuHandle->tier ) | TMU_TIER_ALT_ENABLE ) );
    }

    if (rfTempIrqEnableFunc) {
        int thr1 = TEMP_CELSIUS_TO_KELVIN( in_le32( &pGulModPriv->pHif->rtd_threshold.threshold[ 0 ] ));

        rfTempIrqEnableFunc(thr1);
    }

    /* Enable monitoring*/
    out_le32( &pTmuHandle->tmr, TMU_TMR_ENABLE );
}
static void tmu_sendPciMSI( int msiInterruptFlag )
{
    uint32_t msiLine = 0xFFFFFFFF;
    struct gul_msi_info * pMsiInfo;
    union mtdpowerInfo mtd_power_info;

    pMsiInfo = &pGulModPriv->msi_info[ MSI_IRQ_MUX ];
    if( TVD_MTD_CURENT_TEMP_REQUESTED == msiInterruptFlag) {
    	msiLine = in_le32( &pGulModPriv->pHif->msi_mtd_tvd_curentTemp );
    	out_le32( &pGulModPriv->pHif->tvd_mtdEvent, TVD_MTD_CURENT_TEMP_REQUEST_ACKNOWLEDGED );
	out_le32( &pGulModPriv->pHif->mtd_curentTemp.temp, mtdGetTemp() );
    }
    else if ( TVD_RTD_CURENT_TEMP_REQUESTED == msiInterruptFlag)
    {
       msiLine = in_le32( &pGulModPriv->pHif->msi_rtd_tvd_curentTemp );
       out_le32( &pGulModPriv->pHif->tvd_rtdEvent, TVD_RTD_CURENT_TEMP_REQUEST_ACKNOWLEDGED );
       out_le32( &pGulModPriv->pHif->rtd_curentTemp, rtdGetTemp() );
    }
    else if (TVD_MTD_POWER_INFO_REQUESTED == msiInterruptFlag)
    {
	    msiLine = in_le32( &pGulModPriv->pHif->msi_mtd_power_info );
	    out_le32( &pGulModPriv->pHif->tvd_mtdPowerEvent,
		    TVD_MTD_POWER_INFO_REQUEST_ACKNOWLEDGED );

#if MTD_I2C_PWR_INFO
	    get_sensor_info(&mtd_power_info);
#else
	    mtd_power_info.power_info_fh = NO_PWR;
	    mtd_power_info.power_info_sh = NO_PWR;
	    log_info("Info: I2C power sensor is not available / disabled.\n\r ");
#endif
	    out_le32( &pGulModPriv->pHif->mtd_power_info.
		    power_info_fh, mtd_power_info.power_info_fh);
	    out_le32( &pGulModPriv->pHif->mtd_power_info.
		    power_info_sh, mtd_power_info.power_info_sh);
    }
    else {
	    log_err("\n%s:%s Invalid Request\n",__func__,__FILE__);
    }
    if( msiLine < GUL_MSI_MAX_CNT )
    {
        out_le32( pMsiInfo[ msiLine ].addr, pMsiInfo[ msiLine ].data );
    }
}

bool_t tmu_msiInterruptHandler( uint32_t ulIrqNo,
                                void * pvDevData )
{
    uint32_t msiMtdInterruptFlag;
    uint32_t msiOffset = 0x10;
    uint32_t msiNumber = ulIrqNo - MSI_INTR_START;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    TaskHandle_t xTMUTaskHandle = pvDevData;

    mpic_in32( MPIC_REGS_MSIR0 + msiNumber * msiOffset );
    msiMtdInterruptFlag = in_le32( &pGulModPriv->pHif->tvd_mtdEvent );
    if( (TVD_MTD_CURENT_TEMP_REQUESTED == msiMtdInterruptFlag) )
    {
        tmu_sendPciMSI( msiMtdInterruptFlag );
        return 0;
    }
    if( TVD_MTD_HYSTERESIS_UPDATE_REQUESTED == msiMtdInterruptFlag )
    {
        mtd_hysteresis = in_le32( &pGulModPriv->pHif->mtd_hysteresisVal );
        out_le32( &pGulModPriv->pHif->tvd_mtdEvent,
                  TVD_MTD_HYSTERESIS_UPDATE_ACKNOWLEDGED );
    }

    xTaskNotifyFromISR( xTMUTaskHandle, 1, eSetBits, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    return 0;
}

bool_t mtd_powerIrqHandler( uint32_t ulIrqNo )
{
    uint32_t msiMtdPowerInterruptFlag;
    uint32_t msiOffset = 0x10;
    uint32_t msiNumber = ulIrqNo - MSI_INTR_START;

    mpic_in32( MPIC_REGS_MSIR0 + msiNumber * msiOffset );
    msiMtdPowerInterruptFlag = in_le32(&pGulModPriv->pHif->tvd_mtdPowerEvent);

    if ((TVD_MTD_POWER_INFO_REQUESTED == msiMtdPowerInterruptFlag)) {
	    tmu_sendPciMSI( msiMtdPowerInterruptFlag);
    }
    return 0;
}

bool_t rtd_tempIrqHandler( uint32_t ulIrqNo )
{
    uint32_t msiRtdInterruptFlag;
    uint32_t msiOffset = 0x10;
    uint32_t msiNumber = ulIrqNo - MSI_INTR_START;

    mpic_in32( MPIC_REGS_MSIR0 + msiNumber * msiOffset );

    msiRtdInterruptFlag = in_le32( &pGulModPriv->pHif->tvd_rtdEvent );

    if( (TVD_RTD_CURENT_TEMP_REQUESTED == msiRtdInterruptFlag) )
    {
        tmu_sendPciMSI( msiRtdInterruptFlag);
    }

    return 0;
}

uint32_t rtdTemp_cb ( void ){
        /*TODO: Need to Implement this function
         * Returning a dummy value as 5
         */
        return 40;
}

uint32_t rtdTemp_irq_cb ( int32_t threshold ){
        /*TODO: rtd irq handler yet to implement
         * Returning  success always
         */
        threshold = 0; //MK: Added to avoid compilation warning
        return threshold;
}

void tmuRegisterHostinterrupt( void * pvDevData )
{
    int ret;

    ret = lRegisterIrq( ( uint32_t ) ( MSI_INTR_START + HOST_MSI_TMU ), tmu_msiInterruptHandler, pvDevData );

    if( 1 != ret )
    {
        log_err( "TMU: IRQ register error:%d\r\n", HOST_MSI_TMU );
    }
    /*
     * MSI registration for RTD
     */
    ret = lRegisterIrq( ( uint32_t ) ( MSI_INTR_START + HOST_MSI_RTD ), (bIsrFunc) rtd_tempIrqHandler, pvDevData );

    if( 1 != ret )
    {
        log_err( "TMU: RF IRQ register error:%d\r\n", HOST_MSI_RTD );
    }

    ret = lRegisterIrq( ( uint32_t ) ( MSI_INTR_START + HOST_MSI_MTD_POWER ),
	    (bIsrFunc) mtd_powerIrqHandler, pvDevData );

    if( 1 != ret )
    {
	    log_err( "TMU: MTD power IRQ register error:%d\r\n",
		    HOST_MSI_MTD_POWER );
    }

    bMpicEnable( DEVICE_SHARE_MESSAGE, HOST_MSI_TMU );
    bMpicEnable( DEVICE_SHARE_MESSAGE, HOST_MSI_RTD );
    bMpicEnable( DEVICE_SHARE_MESSAGE, HOST_MSI_MTD_POWER );
}

void tmuInit( void )
{
    pTmuHandle = ( TmuRegs_t * ) TMU_BASE_ADDR;

    out_le32( &pTmuHandle->ttrcr[ 0 ], TMU_TTRCR0_INIT );
    out_le32( &pTmuHandle->ttrcr[ 1 ], TMU_TTRCR1_INIT );

    out_le32( &pTmuHandle->ttcfgr, TMU_TTCFGR_INIT0 );
    out_le32( &pTmuHandle->tscfgr, TMU_TSCFGR_INIT0 );
    out_le32( &pTmuHandle->ttcfgr, TMU_TTCFGR_INIT1 );
    out_le32( &pTmuHandle->tscfgr, TMU_TSCFGR_INIT1 );
    out_le32( &pTmuHandle->teumr[ 0 ], TMU_TEUMR0_ENABLE );
    out_le32( &pTmuHandle->tdemar, TMU_TDEMAR_ENABLE );
    out_le32( &pTmuHandle->tmtmir, TMU_TMTMIR_ENABLE );

    out_le32( &pTmuHandle->tmr, TMU_TMR_DISABLE );
    out_le32( &pTmuHandle->tsr, TMU_TSR_INIT );
    out_le32( &pTmuHandle->tmsr, TMU_TMSR_ENABLE );

    /* ERR052243: Set the raising & falling edge monitor. */
    out_le32( &pTmuHandle->tmrtrctr, TMU_TMRTRCTR_ENABLE | TMRTRCTR_TEMP(0x7));
    out_le32( &pTmuHandle->tmftrctr, TMU_TMFTRCTR_ENABLE | TMFTRCTR_TEMP(0x7));

    vUdelay( 100 );

    out_le32( &pTmuHandle->tmr, TMU_TMR_ENABLE );

    /*
     * RTD temperature get callbacks registeration
     */
    if(rtd_temp_fn_register(rtdTemp_cb, rtdTemp_irq_cb))
        log_err("Failed to register RTD temp callback\n");
}

int32_t mtdGetTemp( void )
{
#ifdef WARMUP_ENABLE
	int32_t temp = 0, retry = 0;
	uint32_t  valid = 0;
	volatile int delay = 0;

	for (retry = 0; retry < MAX_ATTEMPT; retry++)
	{
	    valid = in_le32( &pTmuHandle->site[ 0 ].tritsr ) & 0x80000000;
	    if (!valid) {
		    /* Busy waiting here to let the site come out of busy state & retry. */
		    for(delay=0; delay<MAX_DELAY_ITR; delay++);
		    continue;
	    }
		/* ERR052243: If raising or falling edge happens invalid temp. */
        if ( (in_le32(&pTmuHandle->tidr)) & TMU_TIDR_FALL_RISE_MASK ) {
            out_le32( &pTmuHandle->tidr, TMU_TIDR_FALL_RISE_MASK);
            continue;
        }
	    break;
	}
	if ( valid )
		temp = TEMP_KELVIN_TO_CELSIUS(in_le32( &pTmuHandle->site[ 0 ].tritsr ));

	if ( !valid || temp < MTD_MIN_TEMP || temp > MTD_MAX_TEMP ) {
		log_err("\nmtdGetTemp: Invalid temperature, valid=%u, temp=%d\n\r",
				valid, temp);
		return MTD_TEMP_INVALID;
	}
	log_info("Current Temperature : %d°C \r\n", temp);
	return temp;
#else
    int32_t temp = 0, i;
    uint32_t  valid = 0, retry = 0;
    union mtdcurentTemp curentTemp;
    volatile int delay;

    for( i = 0; i < MAX_TEMP_MONITORING_SITE_ENABLED; i++)
    {
        /* Retry by MAX_ATTEMPT times if site is busy. */
	    for( retry =0; retry < MAX_ATTEMPT; retry++)
	    {
	        valid = in_le32( &pTmuHandle->site[ i ].tritsr ) & 0x80000000;
	        if (!valid) {
			/* Busy waiting here to let the site come out of busy state & retry. */
		        for(delay=0; delay<MAX_DELAY_ITR; delay++);
		        continue;
	        }
		    /* ERR052243: If raising or falling edge happens invalid temp. */
            if ((in_le32(&pTmuHandle->tidr)) & TMU_TIDR_FALL_RISE_MASK) {
                out_le32( &pTmuHandle->tidr, TMU_TIDR_FALL_RISE_MASK);
                continue;
            }
	        break;
	    }

	    if (!valid) {
		    log_err("\nsite[%d] - Temp out of range / Site Busy : %u\n\r",
					i, valid);
		    temp = MTD_TEMP_INVALID;
	    }

        temp = TEMP_KELVIN_TO_CELSIUS(in_le32( &pTmuHandle->site[ i ].tritsr ));

	    if ( temp < MTD_MIN_TEMP || temp > MTD_MAX_TEMP ) {
            log_err("\nmtdGetTemp: site[%d] Invalid temperature, temp=%d\n\r",
                    i, temp);
            temp = MTD_TEMP_INVALID;
        }

        switch (i) {
            case VSPA_TEMP:
                curentTemp.vspa_temp = temp - TEMP_ADJUST;
                break;
            case FECA_TEMP:
                curentTemp.feca_temp = temp - TEMP_ADJUST;
                break;
            case PCI_TEMP:
                curentTemp.pci_temp = temp - TEMP_ADJUST;
                break;
        };
    }
#if MTD_I2C_DIODE_TEMP
    temp = sa56xxxx_get_temp_c();
    if (temp == TMU_I2C_FAILED) {
        log_err("ERR: Unable to get I2C diode temperature.\n\r");
    }
    curentTemp.diode_temp = temp - TEMP_ADJUST;
#else
    log_dbg("Info: Get I2C sensor info is not available / disabled.\n\r ");
    curentTemp.diode_temp = MTD_TEMP_INVALID - TEMP_ADJUST;
#endif

    return curentTemp.temp;
#endif
}

int32_t rtdGetTemp( void )
{
	int rTemp = RTD_TEMP_INVALID;

	if (rfTempFunc != NULL) {
		rTemp = rfTempFunc();
	}
	return rTemp;
}

int8_t rtd_temp_fn_register( rfTempFn_t rtd_temp_cbk,
			rfTempIrqEnableFn_t rtd_irq_cbk)
{
	rfTempFunc = rtd_temp_cbk;
	rfTempIrqEnableFunc = rtd_irq_cbk;
	return 0;
}

void rtd_temp_fun_deregister( void )
{
	rfTempFunc = NULL;
	rfTempIrqEnableFunc = NULL;
}

int8_t mtd_register( mtdThermalHandler_t mtd_cbk )
{
    for( uint8_t i = 0; i < MAX_TMU_USER; i++ )
    {
        if( tmuCallBackHandler[ i ] == NULL )
        {
            tmuCallBackHandler[ i ] = mtd_cbk;
            return i;
        }
    }

    return MTD_REGISTER_FAILED;
}

void mtd_deregister( int8_t id )
{
    if( ( id >= 0 ) && ( id < MAX_TMU_USER ) )
    {
        tmuCallBackHandler[ id ] = NULL;
    }
}
