// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#ifndef  _TEST_FRAMEWORK_H_
#define _TEST_FRAMEWORK_H_

#include <common.h>
#include <test_framework_config.h>

/* Shared Memory Addresses -
	TODO : To be replaced by proper Shared Memory APIs
*/
typedef struct TestStatus
{
	unsigned long long uiGeulTestStatus[ GEUL_E200_CORE_GLOBAL_NUM ];
	unsigned long long uiTempStatus[ GEUL_E200_CORE_GLOBAL_NUM ];
}TestStatus_t;
extern TestStatus_t xTestStatus;

#define BLOCK_FOREVER		( (unsigned int ) ( -1 ) )
#define FUNCTION_NAME_LEN	20
#define QUEUE_LENGTH		10
#define MAX_E200_CORES		GEUL_E200_CORE_GLOBAL_NUM


/* Defines go here */
extern void vSetTestStatus( u32 iCoreId, u32 iBit );
extern void vResetTestStatus( u32 iCoreId, u32 iBit );
#if defined(HOST_CLI_ENABLE) || (defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)
extern void send_host_cli_notification(void);
#endif
#define SET_TEST_STATUS( core_id, bit ) 	vSetTestStatus( core_id, bit )
#define RESET_TEST_STATUS( core_id, bit ) 	vResetTestStatus( core_id, bit )
#define GET_TEST_STATUS( core_id, bit ) 	( ( xTestStatus.uiGeulTestStatus[ core_id ] & ( ( u64 )1 << bit ) ) ? "PASS" : "FAIL" )

#define SET_PREV_STATUS( core_id, bit ) 	( xTestStatus.uiTempStatus[ core_id ] |= ( ( u64 )1 << bit ) )
#define INIT_PREV_STATUS( core_id, value )	( xTestStatus.uiTempStatus[ core_id ] = value )
#define RESET_PREV_STATUS( core_id, bit )	( xTestStatus.uiTempStatus[ core_id ] &= ~( ( u64 )1 << bit ) )
#define GET_PREV_STATUS( core_id, bit )		( ( xTestStatus.uiTempStatus[ core_id ] & ( ( u64 )1 << bit ) ) ? 1 : 0 )

/* Test Case Status Bits */
enum eGeulDemoTestStatus
{
#if GEUL_DEMO_MSI_TEST
	GEUL_DEMO_MSI_TEST_STATUS,
#endif
#if GEUL_DEMO_TIMER_TEST
	GEUL_DEMO_TIMER_TEST_STATUS,
#endif
#if GEUL_DEMO_QDMA_TEST
	GEUL_DEMO_QDMA_TEST_STATUS,
#endif
#if GEUL_DEMO_QDMA_LEGACY_TEST
	GEUL_DEMO_QDMA_LEGACY_TEST_STATUS,
#endif
#if GEUL_DEMO_IPI_QUEUE_TEST
	GEUL_DEMO_IPI_QUEUE_TEST_STATUS,
#endif
#if GEUL_DEMO_SPINLOCK_TEST
	GEUL_DEMO_SPINLOCK_TEST_STATUS,
#endif
#if GEUL_DEMO_AVI_TEST
	GEUL_DEMO_VSPA_AVI_TEST_STATUS,
#endif
	GEUL_DEMO_TBGEN2_TDD_TEST_STATUS,
	GEUL_DEMO_TBGEN2_TDD_MANUAL_TEST_STATUS,
#if GEUL_DEMO_OVERLAY_TEST
	GEUL_DEMO_VSPA_OVERLAY_TEST_STATUS,
#endif
#if GEUL_DEMO_WDOG_TEST
	GEUL_DEMO_WDOG_TEST_STATUS,
#endif
#if GEUL_DEMO_MEMCHECK_TEST
	GEUL_DEMO_MEMCHECK_TEST_STATUS,
#endif
#if GEUL_DEMO_GPIO_TEST
	GEUL_DEMO_GPIO_TEST_STATUS,
#endif
#if GEUL_DEMO_PCIMSI_TEST
	GEUL_DEMO_PCIMSI_TEST_STATUS,
#endif
	GEUL_DEMO_QDMA_MUL_TEST_STATUS,
	GEUL_DEMO_QDMA_SG_TEST_STATUS,
#if GEUL_DEMO_IPC_TEST
	GEUL_DEMO_IPC_TEST1_STATUS,
	GEUL_DEMO_IPC_TEST2_STATUS,
#endif
#if GEUL_DEMO_DCS_TEST
	GEUL_DEMO_DCS_TEST_STATUS,
#endif
#if GEUL_BOOT_MODE_PCI
	GEUL_DEMO_PEBM_LOGGER_STATUS,
#endif
#if GEUL_DEMO_I2C_TEST
	GEUL_DEMO_I2C_TEST_STATUS,
#endif
#if GEUL_HEAP_USAGE_TEST
	GEUL_HEAP_USAGE_TEST_STATUS,
#endif
	GEUL_DEMO_TBGEN1_RX_ALIGN_INT_TEST_STATUS,
	GEUL_DEMO_TBGEN2_RX_ALIGN_INT_TEST_STATUS,
	GEUL_DEMO_TBGEN1_HOST_TTI_TEST_STATUS,
	GEUL_DEMO_TBGEN2_HOST_TTI_TEST_STATUS,
#if GEUL_DEMO_DSPI_TEST
	GEUL_DEMO_DSPI_TEST_STATUS,
#endif
#if GEUL_DEMO_TMU_TEST
	GEUL_DEMO_TMU_TEST_STATUS,
#endif
#if GEUL_DEMO_IPI_ISR_TEST
	GEUL_DEMO_IPI_ISR_TEST_STATUS,
#endif
#if GEUL_DEMO_LATENCY_TEST
	GEUL_DEMO_LATENCY_TEST_STATUS,
#endif
#if GEUL_DEMO_SMEM_TEXT_TEST
	GEUL_DEMO_SMEM_TEXT_TEST_STATUS,
#endif
	GEUL_DEMO_FECA_BBDEV_TEST_STATUS,
#if (GEUL_DEMO_OVERLAY_REUSE_TEST) &&  !defined(VSPA_OV_PEBM_ENABLED)
	GEUL_DEMO_OVERLAY_REUSE_TEST_STATUS,
#endif
	GEUL_DEMO_TBGEN1_NON_TDD_TEST_STATUS,
	GEUL_DEMO_TBGEN2_NON_TDD_TEST_STATUS,
	GEUL_DEMO_TBGEN1_RFG_TEST_STATUS,
	GEUL_DEMO_TBGEN2_RFG_TEST_STATUS,
#if GEUL_DEMO_CORE_SMEM_TEXT_TEST
	GEUL_DEMO_CORE_SMEM_TEXT_TEST_STATUS,
#endif
#if GEUL_DEMO_PEBM_PORT3_TEST
	GEUL_DEMO_PEB_PORT3_TEST_STATUS,
#endif
#if GEUL_DEMO_FLOAT_TEST
	GEUL_DEMO_FLOAT_TEST_STATUS,
#endif
	GEUL_DEMO_MAX_TEST_STATUS
};

/* Hashtable structure to store Test functions and Description */
typedef struct FunctionHashTable 
{
  const char *pcName;
  void (*pvFunc)(void);
  const char *pcTestCaseDesc;
} FunctionHashTable_t;

/* Data structure to exchange data between IPI src and dest */
typedef struct TfIpiData
{
	char pcFunctionName[ FUNCTION_NAME_LEN ];
	uint32 uiTestIteration;
} TfIpiData_t;

extern FunctionHashTable_t xFunctionMap[];

/* Interface APIs to Shell Commands Framework */
bool bIsRestrictedCommand( const char * pcFunctionName );
void vPrintValidTestCases( void );
int iCallFunction( const char *pcName, uint32 uiIterations );
void vInitTestFramework( void );

#endif	/* _TEST_FRAMEWORK_H_ */
