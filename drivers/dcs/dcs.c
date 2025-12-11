// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2023 NXP
 */

#include "FreeRTOS.h"
#include "task.h"
#include <Time.h>

#include "dcs.h"
#include "dcs_hs.h"
#include "dcs_regs_b0.h"
#include "soc.h"
static TaskHandle_t xDCSTMonTaskHandle;
static uint32_t dcs_tmon_arr[1] = { 0 };
volatile struct gul_hif *pHif;

typedef void (*L1_dcs_recalFn_t) (int32_t eventid );
void dcs_recal_cb_register(L1_dcs_recalFn_t cbk_fn);


void bsp_hsdcs_recal_start(uint32_t eventid)
{
	struct gul_msi_info * pMsiInfo;
	uint32_t msiLine;

	pMsiInfo = &pGulModPriv->msi_info[ MSI_IRQ_MUX ];
	msiLine = in_le32(&pGulModPriv->pHif->msi_dcs);

	out_le32(&pGulModPriv->pHif->dcshif.eventid, eventid );

	if (msiLine < GUL_MSI_MAX_CNT)
	{
		out_le32(pMsiInfo[ msiLine ].addr, pMsiInfo[ msiLine ].data);
	}
}

#if L1C_STUB

void L1_hsdcs_recal_request(int32_t eventid)
{
	switch(eventid) {
		case HSADC_EVENT_RECAL_REQUESTED_ANT0:
			log_dbg( "%s() EVENT:Req ANT0 \r\n",__func__);
			/* L1C should take actions as per use-case for ANT0
			 * once L1C completes above actions, call the following function
			 */
			bsp_hsdcs_recal_start(HSADC_EVENT_RECAL_REQ_ACK_ANT0);
		break;

		case HSADC_EVENT_RECAL_REQUESTED_ANT1:
			log_dbg( "%s() EVENT:Req ANT1 \r\n",__func__);
			/* L1C should take actions as per use-case for ANT1
			 * once L1C completes above actions, call the following function
			 */
			bsp_hsdcs_recal_start(HSADC_EVENT_RECAL_REQ_ACK_ANT1);
		break;

		case HSADC_EVENT_RECAL_REQUESTED_ANT0_1:
			log_dbg( "%s() EVENT:Req ANT0_1 \r\n",__func__);
			/* L1C should take actions for ANT0,1 goes down.
			 * once L1C completes above actions, call the following function
			 */
			bsp_hsdcs_recal_start(HSADC_EVENT_RECAL_REQ_ACK_ANT0_1);
		break;

		case HSADC_EVENT_RECAL_FAILED_ANT0:
			/* This is system failure, ANT0 Recalibration failed.
			 * L1 to request L2 to restart the modem  */
		break;
		case HSADC_EVENT_RECAL_FAILED_ANT1:
			/* This is system failure, ANT1 Recalibration failed.
			 * L1 to request L2 to restart the modem  */
		break;

		case HSADC_EVENT_RECAL_COMPLETED:
			log_dbg( "%s() EVENT:Recal completed \r\n",__func__);
		break;

		case HSADC_EVENT_RECAL_COMPLETED2:
			log_dbg( "%s() EVENT:Recal completed2 \r\n",__func__);
		break;

		default:
			log_err( " %s() Invalid Event ID \r\n",__func__);
		break;
	}
}

/* This function reference callback registration.
 * Need to disable L1C_STUB macro after integrating with actual L1C Init
 */
void L1_test_init(){
	/*  resgister the function with DCS module */
	dcs_recal_cb_register(L1_hsdcs_recal_request);
}
#endif /* L1C_STUB */

void dcs_recal_cb_register(L1_dcs_recalFn_t cbk_fn)
{
	out_le32(&pGulModPriv->pHif->dcshif.pL1_cbk_fn, cbk_fn );
}

void dcs_recal_cb_deregister(void)
{
	out_le32(&pGulModPriv->pHif->dcshif.pL1_cbk_fn,0);
}

bool_t hsdcs_msi_irq_handler( uint32_t ulIrqNo,
                                void * pdata )
{
	uint32_t msiOffset = 0x10;
	uint32_t msiNumber = ulIrqNo - MSI_INTR_START;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	TaskHandle_t xDCSTaskHandle = pdata;

	dcs_tmon_arr[0] = in_le32(&pGulModPriv->pHif->dcshif.eventid );

	xHigherPriorityTaskWoken = pdFALSE;
	xTaskNotifyFromISR( xDCSTaskHandle, 1, eSetBits, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	mpic_in32( MPIC_REGS_MSIR0 + msiNumber * msiOffset );

	return 0;
}

void dcsRegisterHostinterrupt( void * pData )
{
    int ret;

    ret = lRegisterIrq( ( uint32_t ) ( MSI_INTR_START + HOST_DCS_TMON_MSIA_7 ), hsdcs_msi_irq_handler, pData );
    if( ret < 0 )
    {
        log_err( "DCS HS: IRQ register error:%d\r\n", HOST_DCS_TMON_MSIA_7 );
		return;
    }
        log_err( "DCS HS: IRQ registered :%d\r\n", HOST_DCS_TMON_MSIA_7 );

    bMpicEnable( DEVICE_SHARE_MESSAGE, HOST_DCS_TMON_MSIA_7 );
}

void vDCSTMonTask( void *pvParameters )
{
	uint32_t ulNotifiedValue;
	L1_dcs_recalFn_t pL1_cbk_fn = NULL;

	(void) pvParameters;

	dcsRegisterHostinterrupt( (void *) xDCSTMonTaskHandle );

	while ( 1 )	{
		/* Wait to be notified of an interrupt. */
		xTaskNotifyWait( pdFALSE, UINT32_MAX, &ulNotifiedValue, portMAX_DELAY);

		pL1_cbk_fn = (L1_dcs_recalFn_t)in_le32(&pGulModPriv->pHif->dcshif.pL1_cbk_fn );
		if(pL1_cbk_fn)
		{
			(*pL1_cbk_fn)(dcs_tmon_arr[0]);
		}
	}
}

void init_dcs_host_if()
{
	int iRc = 0;

	iRc = xTaskCreate(vDCSTMonTask, "DCS_TMon", DCSTMON_TASK_STACKSIZE,
				   NULL, DCSTMON_TASK_PRIORITY, &xDCSTMonTaskHandle);

	if (unlikely( iRc != pdPASS ))
	{
		log_err("Failed to create DCS TMon task\n\r");
	}
	log_info("DCS_TMon task created !!\n\r");
    
}
/* Waiting for DCS PLL to be locked */
static int iDcsPllCheck( void )
{
	vuint32 * ulDcsPllStatus = (vuint32 *) (DCS_CLK_GEN_BASE +
					DCS_PLLRSTCTL_OFFSET);
	uint32_t ulRegStatus = 0, ulTryCount = WAIT_LIMIT_MAX;

	ulRegStatus = READ_REGISTER(ulDcsPllStatus);
	log_dbg("DCSPLL check\n\r");
	while (ulTryCount--) {
		if (ulRegStatus & DCS_PLL_STATUS_CHK) {
			log_info("DCS: PLL is locked\n\r");
			return 0;
		}
		vUdelay(200);
		ulRegStatus = READ_REGISTER(ulDcsPllStatus);
		log_dbg("DCS: PLL Not locked\n\r");
	}
	return -DCS_WAIT_TIMEOUT;
}

int iDcsInit(volatile struct gul_hif *pHif)
{
    int iRet;

    log_dbg( "FreeRTOS DCS Init\r\n");

    /* Software should wait for DCS PLL lock before accessing
     * DCS Macro registers/CCSR space */
    iRet = iDcsPllCheck();
    if(iRet) {
        log_err("DCS PLL lock status: Failed[%d]\n\r", iRet);
        return iRet;
    }

#ifdef DCS_LS_ENABLED
    /* Configuring LS DCS 1*/
    if(get_soc_revision() == GEUL_SVR_REVA_VAL)
    {
        vUdelay(500); /* Sleep for sometime and wait for Magic */
        iRet = vLSDcsInit_a0( pHif, DCS_LS1 );
        if( 0 != iRet ) {
            log_err( "%s:[DCS LS1]:Configuration Failed, err=%d\r\n", __func__, iRet );
            return iRet;
        }

        /* Configuring LS DCS 2*/
        iRet = vLSDcsInit_a0( pHif, DCS_LS2 );
        if( 0 != iRet ) {
            log_err( "%s:[DCS LS2]:Configuration Failed, err=%d\r\n", __func__, iRet );
            return iRet;
        }
    }
    else
    {
        iRet = vLSDcsInit_b0( pHif );
        if( 0 != iRet ) {
            log_err( "%s:[DCS LS]:Configuration Failed, err=%d\r\n", __func__, iRet );
            return iRet;
        }
    }
    log_info( "DCS_LS Success\r\n" );
#else
    UNUSED(pHif);
#endif /* ifdef DCS_LS_ENABLED */

#if L1C_STUB
	L1_test_init(); //test
#endif

    return 0;
}
