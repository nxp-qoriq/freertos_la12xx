// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#ifndef _TEST_CASES_AVI_H_
#define _TEST_CASES_AVI_H_

#include "test_framework.h"
/******************************************************************
*			Define's
******************************************************************/
#define OVERLAY_SECTION_OFFSET 		( 0x00 )
#define NUMBER_OF_VSPA_MAILBOXES 	( 2 )

#define MAX_MBOX_SEND 			( 2 )
#define VSPA_MAX_LOOP 			( 1 )

/******************************************************************
*		Function Prototype's
******************************************************************/
bool_t bVSPA0GroupAInterrupt( uint32_t ulirq, void *dev_data );
bool_t bVSPA0GroupBInterrupt( uint32_t ulirq, void *dev_data );

#if GEUL_DEMO_AVI_TEST
void vGeulDemoVspaAviTest( void );
void vLaunchAviTestTask( void );
#endif	/* GEUL_DEMO_AVI_TEST */

#if GEUL_DEMO_OVERLAY_TEST
extern int32_t ulOverlay_resp;
int iTriggerHostOverlay( void );
int iTriggerPEBMOverlay( void );
void vGeulOverlayTest( void );
#endif	/* GEUL_DEMO_OVERLAY_TEST */

#if GEUL_VSPA_LOG
bool_t vspa_irq_handler(uint32_t ulIrq, void *pvDevData);
void vLaunchVspaLogs(void);
#endif

#endif	/* _TEST_CASES_AVI_H_ */
