// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2025 NXP
 *
 * FreeRTOS Kernel V10.0.1
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

 /******************************************************************************
 *
 * See the following URL for information on the commands defined in this file:
 * http://www.FreeRTOS.org/FreeRTOS-Plus/FreeRTOS_Plus_UDP/Embedded_Ethernet_Examples/Ethernet_Related_CLI_Commands.shtml
 *
 ******************************************************************************/


/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "Time.h"

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"
#include "debug_console.h"
#include "common.h"
#include "i2cAPI.h"
#include "soc.h"
#include "queue.h"

#ifdef TESTFRAMEWORK_ENABLE
#include "ipiQueue.h"
#include "test_framework.h"
#ifdef CONFIG_TTI
#include "tc_la12xx_tbgen.h"
#endif
#endif
#ifdef HAWK_ENABLED
#include "hawk.h"
#endif
#ifdef TDD_DEMOAPP_ENABLE
#include "tdd_app.h"
#endif
#if (defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)
#include "l1c_cli.h"
#endif

#ifdef CONFIG_FLEXSPI
#include <fspi_api.h>
#endif

#if defined(LA12XX_DRIVER_PCI) || defined(LA12XX_DRIVER_PCI_LAT_FP)
#include "nxp-pcie.h"
#ifdef LA12XX_DRIVER_PCI_LAT_FP
#include "lattice-pcie-ep-fpga.h"
#include "pcie_ep_lat_fpga.h"
#endif
#endif

#ifndef  configINCLUDE_TRACE_RELATED_CLI_COMMANDS
#define configINCLUDE_TRACE_RELATED_CLI_COMMANDS 0
#endif

static portBASE_TYPE prvEchoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvMDCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvMWCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvI2cReadCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvI2cWriteCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
#ifdef TESTFRAMEWORK_ENABLE
static portBASE_TYPE prvTestCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
#endif
#ifdef CONFIG_TTI
static portBASE_TYPE prvTbgenttiCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
#endif
#ifdef TDD_DEMOAPP_ENABLE
extern void vVspaAddNewBuffer(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
extern void vVspaAddNewPattern(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
extern void vTbgenTddStart(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
extern void vTbgenTddStop();
static portBASE_TYPE prvVspaCfgBufferCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvVspaCfgPatternCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvTddStartCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvTddStopCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
#endif

#ifdef GEUL_LA1224
static portBASE_TYPE prvVidGetVddCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
int iGetCurrentVdd();
#endif

#ifdef FSPI_CMD_ENABLE 
static portBASE_TYPE prvFspiUtil(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
#endif

#ifdef LA12XX_DRIVER_PCI
static portBASE_TYPE prvPCIeReadEpMem(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static portBASE_TYPE prvPCIeWriteEpMem(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static portBASE_TYPE prvPCIeReadRcMem(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static portBASE_TYPE prvPCIeWriteRcMem(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
#endif

#if defined(LA12XX_DRIVER_PCI) || defined(LA12XX_DRIVER_PCI_LAT_FP)
static portBASE_TYPE prvlsPCIe(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
#endif

#ifdef LA12XX_DRIVER_PCI_LAT_FP
static portBASE_TYPE prvPCIeTxData(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static portBASE_TYPE prvPCIeRxData(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static portBASE_TYPE prvPCIeThroughPut(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);

#endif

static const CLI_Command_Definition_t xEchoCommand =
{
	"echo", /* The command string to type. */
	"\r\necho \r\n Usage : Takes variable number of parameters, echos each in turn\r\n",
	prvEchoCommand, /* The function to run. */
	-1 /* The user can enter any number of commands. */
};

static const CLI_Command_Definition_t xMDCommand =
{
	"md", /* The command string to type. */
	"\r\nmd <32 bit address> - memory display takes 32 bit address to be read\r\n Usage : md <address>\r\n",
	prvMDCommand, /* The function to run. */
	1 /* User can enter only one argument. */
};

static const CLI_Command_Definition_t xMWCommand =
{
	"mw", /* The command string to type. */
	"\r\nmw <32 bit address> <value> - memory write takes 32 bit address and value\r\n Usage : mw <address> <value>\r\n",
	prvMWCommand, /* The function to run. */
	2 /* User can enter two argumensts. */
};

#ifdef TESTFRAMEWORK_ENABLE
static const CLI_Command_Definition_t xTestCommand =
{
	"test", /* The command string to type. */
	"\r\ntest <function_name> <options> - \r\n Usage : test <function_name> <core_id> <iterations>" \
	"\r\n To get list of supported test cases," \
	"\n\r Type:" \
	"\n\r\t test help 0x1 0 - Execute help function on Core 0 (Min Iteration is 1)" \
	"\n\r\t test list 0x2 2 - Execute list function on Core 1 (Iteration - 2)" \
	"\n\r\t test list 0x3 1 - Execute list function on Core 0 & 1 (Iteration - 1)",
	prvTestCommand, /* The function to run. */
	3 /* User can enter three argumensts. */
};
#endif
#ifdef CONFIG_TTI
static const CLI_Command_Definition_t xTbgenttiCommand =
{
	"tbgen_tti", /* The command string to type. */
	"\r\n Usage : tbgen_tti  <tbgen_num> <tti_interval(uS)>" \
	"\r\n Possible tbgen_num: 1, 2"\
	"\r\n possible tti_internval val:125uS, 250uS, 500uS, 1000uS" \
	"\n\r Ex:for tti interval 500uS at tbgen1: tbgen2_tti 1 500",
	prvTbgenttiCommand, /* The function to run. */
	2 /* User can enter one argumensts. */
};
#endif

#ifdef TDD_DEMOAPP_ENABLE
static const CLI_Command_Definition_t xVspaCfgBufferCommand =
{
	"vspa_cfg_buff", /* The command string to type */
	"\r\n\r\nvspa_cfg_buff <buf-no> <buf-addr> <buf-size> <ls0/ls1> <antenna-id> <tx/rx>\r\n",
	prvVspaCfgBufferCommand, /* The function to run. */
	6 /* Number of arguments */
};

static const CLI_Command_Definition_t xVspaCfgPatternCommand =
{
	"vspa_cfg_patt", /* The command string to type */
	"\r\n\r\nvspa_cfg_patt <pattern-no> <buf-no> <is-last> <total-syms> <active-syms> <ls0/ls1> <antenna-id> <tx/rx>\r\n",
	prvVspaCfgPatternCommand, /* The function to run. */
	8 /* Number of arguments */
};
static const CLI_Command_Definition_t xTddStartCommand =
{
	"start_tdd", /* The command string to type */
	"\r\n\r\nstart_tdd <dl-slots> <dl-syms> <ul-slots> <ul-syms> <ls0/ls1> <antenna-id> \r\n Starts the TDD reference application. Only 5ms pattern duration currently supported.\r\n",
	prvTddStartCommand, /* The function to run. */
	6 /* Number of arguments */
};

static const CLI_Command_Definition_t xTddStopCommand =
{
	"stop_tdd", /* The command string to type */
	"\r\nstop_tdd\r\n Stops the TDD reference application\r\n",
	prvTddStopCommand, /* The function to run. */
	0 /* Number of arguments */
};
#endif

#if (defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)
static const CLI_Command_Definition_t xL1CDemoCommand =
{
	"l1c", /* The command string to type */
	L1C_CLI_COMMAND_DESCRIPTION,
	prvL1CDemoCommand, /* The function to run. */
	-1 /* Number of arguments */
};
#endif

static const CLI_Command_Definition_t xI2cReadCommand =
{
	"i2cread", /* The command string to type. */
	"\r\ni2cread bus_no slave_address offset noOffBytes max_offset_length\r\n",
	prvI2cReadCommand, /* The function to run. */
	5/* User have to enter five argumensts. */
};
          

static const CLI_Command_Definition_t xI2cWriteCommand =
{
	"i2cwrite", /* The command string to type. */
	"\r\ni2cwrite bus_no slave_address offset value(for write, only 4 bytes) noOffBytes max_offset_length\r\n",
	prvI2cWriteCommand, /* The function to run. */
	6/* User have to enter five argumensts. */
};

#ifdef GEUL_LA1224
static const CLI_Command_Definition_t xVidGetVddCommand =
{
	"get_vdd", /* The command string to type. */
	"\r\nget_vdd\r\n",
	prvVidGetVddCommand, /* The function to run. */
	0/* User have to enter five argumensts. */
};
#endif

#ifdef FSPI_CMD_ENABLE 
static const CLI_Command_Definition_t xFlexSpiProbeTest =
{
	"sf_probe", /* The command string to type. */
	"\r\nsf_probe\r\n",
	prvFspiUtil, /* The function to run. */
	0/* User have to enter five argumensts. */
};

static const CLI_Command_Definition_t xFlexSpiEraseTest =
{
	"sf_erase", /* The command string to type. */
	"\r\nsf_erase flash_offset length \r\n",
	prvFspiUtil, /* The function to run. */
	2/* User have to enter five argumensts. */
};

static const CLI_Command_Definition_t xFlexSpiReadTest =
{
	"sf_rd", /* The command string to type. */
	"\r\nsf_rd flash_offset memory_address length \r\n",
	prvFspiUtil, /* The function to run. */
	3/* User have to enter five argumensts. */
};
static const CLI_Command_Definition_t xFlexSpiWriteTest =
{
	"sf_wr", /* The command string to type. */
	"\r\nsf_wr memory_address flash_offset length \r\n",
	prvFspiUtil, /* The function to run. */
	3/* User have to enter five argumensts. */
};
#endif

#ifdef LA12XX_DRIVER_PCI
static const CLI_Command_Definition_t xPCIeReadEpMemTest =
{
	"PCIeReadEpMem", /* The command string to type. */
	"\r\nPCIeReadEpMem:\r\n" /* The command string to type. */
	"\tBrief:\r\n"
	"\t\tPCIeReadEpMem will read data from EP memory from RC side.\r\n"
	"\tUsage:\r\n"
	"\t\tPCIeReadEpMem <ID> <BarNumber> <Offset> <Size>\r\n"
	"\t\tID - PCIe controller ID\r\n"
	"\t\tBarNumber - EP's BAR Number\r\n"
	"\t\tOffset - Offset from BAR address\r\n"
	"\t\tSize - Number of dwords(4 byte words) to read\r\n"
	"\t\tNote: All arguments should be in hexadecimal.\r\n",
	prvPCIeReadEpMem, /* The Function to run. */
	4 /* User have to enter four arguments. */
};

static const CLI_Command_Definition_t xPCIeWriteEpMemTest =
{
	"PCIeWriteEpMem", /* The command string to type. */
	"\r\nPCIeWriteEpMem:\r\n" /* The command string to type. */
	"\tBrief:\r\n"
	"\t\tPCIeWriteEpMem will write to EP memory from RC side.\r\n"
	"\tUsage:\r\n"
	"\t\tPCIeWriteEpMem <ID> <BarNumber> <Offset> <Value>\r\n"
	"\t\tID - PCIe controller ID\r\n"
	"\t\tBarNumber - EP's BAR Number\r\n"
	"\t\tOffset - Offset from BAR address\r\n"
	"\t\tValue - Value(4Byte) to be written\r\n"
	"\t\tNote: All arguments should be in hexadecimal.\r\n",
	prvPCIeWriteEpMem, /* The Function to run. */
	4 /* User have to enter four arguments. */
};

static const CLI_Command_Definition_t xPCIeReadRcMemTest =
{
	"PCIeReadRcMem", /* The command string to type. */
	"\r\nPCIeReadRcMem:\r\n" /* The command string to type. */
	"\tBrief:\r\n"
	"\t\tPCIeReadRcMem will read from RC memory from EP side.\r\n"
	"\tUsage:\r\n"
	"\t\tPCIeReadRcMem <ID> <Address> <Size>\r\n"
	"\t\tID - PCIe controller ID\r\n"
	"\t\tAddress - PCIe bus address exposed by RC\r\n"
	"\t\tSize - Number of dwords(4 byte words) to read\r\n"
	"\t\tNote: All arguments should be in hexadecimal.\r\n",
	prvPCIeReadRcMem, /* The Function to run. */
	3 /* User have to enter three arguments. */
};

static const CLI_Command_Definition_t xPCIeWriteRcMemTest =
{
	"PCIeWriteRcMem", /* The command string to type. */
	"\r\nPCIeWriteRcMem:\r\n" /* The command string to type. */
	"\tBrief:\r\n"
	"\t\tPCIeWriteRcMem will write to RC memory from EP side.\r\n"
	"\tUsage:\r\n"
	"\t\tPCIeWriteRcMem <ID> <Address> <Value>\r\n"
	"\t\tID - PCIe controller ID\r\n"
	"\t\tAddress - PCIe bus address exposed by RC\r\n"
	"\t\tValue - Value(4Byte) to be written\r\n"
	"\t\tNote: All arguments should be in hexadecimal.\r\n",
	prvPCIeWriteRcMem, /* The Function to run. */
	3 /* User have to enter three arguments. */
};
#endif /* LA12XX_DRIVER_PCI ends */

#if defined(LA12XX_DRIVER_PCI) || defined(LA12XX_DRIVER_PCI_LAT_FP)
static const CLI_Command_Definition_t xlsPCIeTest =
{
	"lspci", /* The command string to type. */
	"\r\nlspci:\r\n" /* The command string to type. */
	"\tBrief:\r\n"
	"\t\tDump PCIe config space.\r\n"
	"\tUsage:\r\n"
	"\t\tlspci <ID> <Mode>\r\n"
	"\t\tID - PCIe controller ID\r\n"
	"\t\tMode - i (info) / v (verbose)\r\n"
	"\t\te.g.: lspci 2 i\r\n",
	prvlsPCIe, /* The Function to run. */
	2 /* User have to enter two arguments. */
};
#endif /* LA12XX_DRIVER_PCI || LA12XX_DRIVER_PCI_LAT_FP ends */
/*-----------------------------------------------------------*/

#ifdef LA12XX_DRIVER_PCI_LAT_FP
static const CLI_Command_Definition_t xPCIeTxData =
{
	"PCIeTxData", /* The command string to type. */
	"\r\nPCIeTxData\r\n"
	"\tBrief:\r\n"
	"\t\tPCIeTxData will config TX RX buffer and start start DMA\r\n"
	"\tUsage:\r\n"
	"\t\tPCIeTxData <TxAddr> <RxAddr> <Size> <TotTxSzAddr> <TotRxSzAddr>\r\n"
	"\t\t TxAddr : Tx Buf Addr\r\n"
	"\t\t RxAddr : Rx Buf Addr\r\n"
	"\t\t Size   : Tx/Rx Buf Size, 4K aligned\r\n"
	"\t\t TotTxSzAddr : Tx data size/Optional 0\r\n"
	"\t\t TotRxSzAddr : Rx data size/Optional 0\r\n"
	"\t\t e.g.: PCIeTxData 0xe5000000 0xe5004000 0x1000 0xe6000000 0xe6000100\r\n"
	"\t\t OR    PCIeTxData 0xe5000000 0xe5004000 0x1000 0 0\r\n"
	"\t\tNote: All arg should be in hex\r\n",
	prvPCIeTxData, /* The function to run. */
	5/* User have to enter five argumensts. */
};

static const CLI_Command_Definition_t xPCIeRxData =
{
	"PCIeRxData", /* The command string to type. */
	"\r\nPCIeRxData\r\n"
	"\tBrief:\r\n"
	"\t\tPCIeRxData will stop DMA and read back RX buffer.\r\n"
	"\tUsage:\r\n"
	"\t\tPCIeRxData <RxAddr> <Size>\r\n"
	"\t\t RxAddr : Rx Buffer Address\r\n"
	"\t\t Size   : Rx Buffer Size, Should be 4K aligned\r\n"
	"\t\t e.g.: PCIeRxData 0xe5004000 0x1000\r\n"
	"\t\tNote: - RxAddr & Size, same as PCIeTxData RxAddr & Size\r\n"
	"\t\t      - All arg should be in hex.\r\n",
	prvPCIeRxData, /* The function to run. */
	2/* User have to enter two argumensts. */
};

static const CLI_Command_Definition_t xPCIeThroughPut =
{
	"PCIeThroughPut", /* The command string to type. */
	"\r\nPCIeThroughPut\r\n"
	"\tBrief:\r\n"
	"\t\tPCIeThroughPut will find the throughput.\r\n"
	"\tUsage:\r\n"
	"\t\tPCIeThroughPut <TxAddr> <RxAddr> <Size> <TotTxSzAddr> <TotRxSzAddr>\r\n"
	"\t\t TxAddr : Tx Buffer Address\r\n"
	"\t\t RxAddr : Rx Buffer Address\r\n"
	"\t\t Size   : Tx/Rx Buffer Size, Should be 4K aligned\r\n"
	"\t\t TotTxSzAddr : Buffer to hold number of Bytes transmitted\r\n"
	"\t\t TotRxSzAddr : Buffer to hold number of Bytes recieved\r\n"
	"\t\t e.g.: PCIeThroughPut 0xe5000000 0xe5004000 0x1000 0xe6000000 0xe6000100\r\n"
	"\t\tNote: All arg should be in hex\r\n",
	prvPCIeThroughPut, /* The function to run. */
	5/* User have to enter five argumensts. */
};
#endif /* LA12XX_DRIVER_PCI_LAT_FP */

void vRegisterSampleCLICommands( void )
{
	FreeRTOS_CLIRegisterCommand( &xEchoCommand );
	FreeRTOS_CLIRegisterCommand( &xMDCommand );
	FreeRTOS_CLIRegisterCommand( &xMWCommand );
	FreeRTOS_CLIRegisterCommand( &xI2cReadCommand );
	FreeRTOS_CLIRegisterCommand( &xI2cWriteCommand );
#ifdef GEUL_LA1224
	FreeRTOS_CLIRegisterCommand( &xVidGetVddCommand );
#endif
#ifdef TESTFRAMEWORK_ENABLE
	FreeRTOS_CLIRegisterCommand( &xTestCommand );
#endif
#ifdef CONFIG_TTI
	FreeRTOS_CLIRegisterCommand( &xTbgenttiCommand );
#endif
#ifdef TDD_DEMOAPP_ENABLE
	FreeRTOS_CLIRegisterCommand( &xVspaCfgBufferCommand );
	FreeRTOS_CLIRegisterCommand( &xVspaCfgPatternCommand );
	FreeRTOS_CLIRegisterCommand( &xTddStartCommand );
	FreeRTOS_CLIRegisterCommand( &xTddStopCommand );
#endif
#if (defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)
	FreeRTOS_CLIRegisterCommand( &xL1CDemoCommand );
#endif
#ifdef HAWK_ENABLED
	vRegisterHAWKCommands();
#endif
#if FSPI_CMD_ENABLE
	FreeRTOS_CLIRegisterCommand( &xFlexSpiProbeTest );
	FreeRTOS_CLIRegisterCommand( &xFlexSpiEraseTest );
	FreeRTOS_CLIRegisterCommand( &xFlexSpiReadTest );
	FreeRTOS_CLIRegisterCommand( &xFlexSpiWriteTest );
#endif
#ifdef LA12XX_DRIVER_PCI
	FreeRTOS_CLIRegisterCommand( &xPCIeReadEpMemTest );
	FreeRTOS_CLIRegisterCommand( &xPCIeWriteEpMemTest );
	FreeRTOS_CLIRegisterCommand( &xPCIeReadRcMemTest );
	FreeRTOS_CLIRegisterCommand( &xPCIeWriteRcMemTest );
#endif /* LA12XX_DRIVER_PCI ends */	
#if defined(LA12XX_DRIVER_PCI) || defined(LA12XX_DRIVER_PCI_LAT_FP)
	FreeRTOS_CLIRegisterCommand( &xlsPCIeTest );
#endif /* LA12XX_DRIVER_PCI ends */
#ifdef LA12XX_DRIVER_PCI_LAT_FP
	FreeRTOS_CLIRegisterCommand( &xPCIeRxData );
	FreeRTOS_CLIRegisterCommand( &xPCIeTxData );
	FreeRTOS_CLIRegisterCommand( &xPCIeThroughPut );
#endif
}
/*-----------------------------------------------------------*/

static portBASE_TYPE prvEchoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	const char *pcParameter;
	BaseType_t xParameterStringLength, xReturn;
	static UBaseType_t uxParameterNumber = 0;

	/* Remove compile time warnings about unused parameters, and check the
	   write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	   write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	if( uxParameterNumber == 0 )
	{
		/* The first time the function is called after the command has been
		   entered just a header string is returned. */
		sprintf( pcWriteBuffer, "The parameters were:\r\n" );

		/* Next time the function is called the first parameter will be echoed
		   back. */
		uxParameterNumber = 1U;

		/* There is more data to be returned as no parameters have been echoed
		   back yet. */
		xReturn = pdPASS;
	}
	else
	{
		/* Obtain the parameter string. */
		pcParameter = FreeRTOS_CLIGetParameter
			(
			 pcCommandString,		/* The command string itself. */
			 uxParameterNumber,		/* Return the next parameter. */
			 &xParameterStringLength	/* Store the parameter string length. */
			);

		if( pcParameter != NULL )
		{
			/* Return the parameter string. */
			memset( pcWriteBuffer, 0x00, xWriteBufferLen );
			sprintf( pcWriteBuffer, "%d: ", ( int ) uxParameterNumber );
			strncat( pcWriteBuffer, ( char * ) pcParameter, ( size_t ) xParameterStringLength );
			strncat( pcWriteBuffer, "\r\n", strlen( "\r\n" ) );

			/* There might be more parameters to return after this one. */
			xReturn = pdTRUE;
			uxParameterNumber++;
		}
		else
		{
			/* No more parameters were found.  Make sure the write buffer does
			   not contain a valid string. */
			pcWriteBuffer[ 0 ] = 0x00;

			/* No more data to return. */
			xReturn = pdFALSE;

			/* Start over the next time this command is executed. */
			uxParameterNumber = 0;
		}
	}

	return xReturn;
}
#ifdef LA12XX_DRIVER_PCI
/* \fn static portBASE_TYPE prvPCIeReadEpMem(char *pcWriteBuffer,
 *					size_t xWriteBufferLen, const char *pcCommandString)
 *
 * @brief : Function to read EP memory via PCIe. If PCIe controller
 * 	    	is working in RC mode it will read the data from EP memory
 * 	    	else will return error.
 * */
static portBASE_TYPE prvPCIeReadEpMem(char *pcWriteBuffer,
		size_t xWriteBufferLen, const char *pcCommandString)
{
	BaseType_t xParameterIdStrLen = 0,
		   xParameterBarNumStrLen = 0,
		   xParameterOffsetStrLen = 0,
		   xParameterSizeStrLen = 0;
	char *pcId = NULL,
	     *pcBarNum = NULL,
	     *pcOffset = NULL,
	     *pcSize = NULL;
	uint32_t uiId = 0,
			 uiBarNum = 0,
			 uiOffset = 0,
			 uiSize = 0;
	uint32_t data = 0,
			 ret = 0,
			 offset = 0;

	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* Fill all parameters and there length from command strings */
	pcId = ( char * ) FreeRTOS_CLIGetParameter
        (
            pcCommandString,	/* The command string itself. */
            1U,           		/* Return the 1st parameter. */
            &xParameterIdStrLen	/* Store the parameter string length. */
        );

	pcBarNum = ( char * ) FreeRTOS_CLIGetParameter
        (
            pcCommandString,	/* The command string itself. */
            2U,           		/* Return the 2nd parameter. */
            &xParameterBarNumStrLen	/* Store the parameter string length. */
        );

	pcOffset = ( char * ) FreeRTOS_CLIGetParameter
        (
            pcCommandString,	/* The command string itself. */
            3U,           		/* Return the 3rd parameter. */
            &xParameterOffsetStrLen	/* Store the parameter string length. */
        );

	pcSize = ( char * ) FreeRTOS_CLIGetParameter
        (
            pcCommandString,	/* The command string itself. */
            4U,           		/* Return the 4th parameter. */
            &xParameterSizeStrLen	/* Store the parameter string length. */
        );

	/* NULL Terminate all strings */
	pcId[ xParameterIdStrLen ] = 0x00;
	pcBarNum[ xParameterBarNumStrLen ] = 0x00;
	pcOffset[ xParameterOffsetStrLen ] = 0x00;
	pcSize[ xParameterSizeStrLen ] = 0x00;

	uiId = strtoul( pcId,  (char **)NULL, BASE_HEXA );
	uiBarNum = strtoul( pcBarNum,  (char **)NULL, BASE_HEXA );
	uiOffset = strtoul( pcOffset,  (char **)NULL, BASE_HEXA );
	uiSize = strtoul( pcSize,  (char **)NULL, BASE_HEXA );

	uiId = uiId - 1;
	log_dbg("\r\n%s: ID:%x, BarNum:%x, offset:%x, size:%x\r\n",__func__,
			uiId, uiBarNum, uiOffset, uiSize);

	switch (uiId) {
		case PCIE_1:
			log_err("%s: PCIE%d : Only EP mode supported !\r\n",
						__func__, GEUL_PCIE_ID(uiId));
			break;

		case PCIE_2:
			ret = PcieGetMode(uiId);
			if (ret == 1) {
				/* IN RC Mode */
				log_info("PCIE%d : Data -\r\n", GEUL_PCIE_ID(uiId));
				for(uint32_t i = 0; i < uiSize; i++)
				{
					offset = uiOffset+(i*4);
					ret = ulPcieReadMem(uiId, uiBarNum, offset, sizeof(uint32_t), &data);
					log_info("%s0x%08x ",(i%4)?"":"\r\n", data);
				}
				log_info("\r\n");
			} else {
				log_err("%s: PCIE%d : In EP mode (Invalid Command) !\r\n",
								__func__,GEUL_PCIE_ID(uiId));
			}
			break;

		default:
			log_err("%s: PCIE%d : Invalid PCIe ID, use ID as 1 or 2 !\r\n",
								__func__, GEUL_PCIE_ID(uiId));
			break;
	};

	pcWriteBuffer[ 0 ] = 0x00;
	return pdFALSE;
}

/* \fn static portBASE_TYPE prvPCIeWriteEpMem(char *pcWriteBuffer,
 * 					size_t xWriteBufferLen, const char *pcCommandString)
 *
 * @brief : Function to write EP memory via PCIe. If PCIe controller
 * 	    	is working in RC mode it will write data to the EP memory
 * 	    	else will return error.
 * */
static portBASE_TYPE prvPCIeWriteEpMem(char *pcWriteBuffer,
		size_t xWriteBufferLen, const char *pcCommandString)
{
	BaseType_t xParameterIdStrLen = 0,
			xParameterBarNumStrLen = 0,
			xParameterOffsetStrLen = 0,
			xParameterValueStrLen = 0;
	char *pcId = NULL,
		*pcBarNum = NULL,
		*pcOffset = NULL,
		*pcValue = NULL;
	uint32_t uiId = 0,
			uiBarNum = 0,
			uiOffset = 0,
			uiValue = 0,
			ret = 0;

	( void ) pcCommandString;
    ( void ) xWriteBufferLen;
    configASSERT( pcWriteBuffer );

	/* Fill all parameters and there length from command strings */
	pcId = ( char * ) FreeRTOS_CLIGetParameter
        (
            pcCommandString,	/* The command string itself. */
            1U,           		/* Return the 1st parameter. */
            &xParameterIdStrLen	/* Store the parameter string length. */
        );

	pcBarNum = ( char * ) FreeRTOS_CLIGetParameter
        (
            pcCommandString,	/* The command string itself. */
            2U,           		/* Return the 2nd parameter. */
            &xParameterBarNumStrLen	/* Store the parameter string length. */
        );

	pcOffset = ( char * ) FreeRTOS_CLIGetParameter
        (
            pcCommandString,	/* The command string itself. */
            3U,           		/* Return the 3rd parameter. */
            &xParameterOffsetStrLen	/* Store the parameter string length. */
        );

	pcValue = ( char * ) FreeRTOS_CLIGetParameter
        (
            pcCommandString,	/* The command string itself. */
            4U,           		/* Return the 4th parameter. */
            &xParameterValueStrLen	/* Store the parameter string length. */
        );

	/* NULL Terminate all strings */
	pcId[ xParameterIdStrLen ] = 0x00;
	pcBarNum[ xParameterBarNumStrLen ] = 0x00;
	pcOffset[ xParameterOffsetStrLen ] = 0x00;
	pcValue[ xParameterValueStrLen ] = 0x00;

	uiId = strtoul( pcId,  (char **)NULL, BASE_HEXA );
	uiBarNum = strtoul( pcBarNum,  (char **)NULL, BASE_HEXA );
	uiOffset = strtoul( pcOffset,  (char **)NULL, BASE_HEXA );
	uiValue = strtoul( pcValue,  (char **)NULL, BASE_HEXA );

	uiId = uiId - 1;
	log_dbg("\r\n %s: ID:%x, BarNum:%x, offset:%x, Value:%x\r\n",__func__,
			uiId, uiBarNum, uiOffset, uiValue);

	switch (uiId) {
		case PCIE_1:
			log_err("%s: PCIE%d : Only EP mode supported !\r\n",
								__func__,GEUL_PCIE_ID(uiId));
			break;

		case PCIE_2:
			ret = PcieGetMode(uiId);
			if (ret == 1) {
				/* In RC Mode */
				ret = ulPcieWriteMem(uiId, uiBarNum, uiOffset, sizeof(uiValue),
											uiValue);
				if (ret == sizeof(uiValue)) {
					log_info("PCIE%d : Data written successfully\r\n",
								GEUL_PCIE_ID(uiId));
				} else {
					log_info("%s: PCIE%d : Data write failed !\r\n",
								__func__, GEUL_PCIE_ID(uiId));
				}
			} else {
				log_err("%s: PCIE%d : In EP mode (Invalid Command) !\r\n",
							__func__, GEUL_PCIE_ID(uiId));
			}
			break;

		default:
			log_err("%s: PCIE%d : Invalid PCIe ID, use ID as 1 or 2 !\r\n",
								__func__, GEUL_PCIE_ID(uiId));
			break;
	};

	pcWriteBuffer[ 0 ] = 0x00;
	return pdFALSE;
}

/* \fn static portBASE_TYPE prvPCIeReadRcMem(char *pcWriteBuffer,
 * 				size_t xWriteBufferLen, const char *pcCommandString)
 *
 * @brief : Function to read RC memory via PCIe. If PCIe controller
 * 	        is working in EP mode it will read the data from EP memory
 * 	        else will return error.
 * */
static portBASE_TYPE prvPCIeReadRcMem(char *pcWriteBuffer,
		size_t xWriteBufferLen, const char *pcCommandString)
{
	BaseType_t xParameterIdStrLen = 0,
			xParameterAddressStrLen = 0,
			xParameterSizeStrLen = 0;
	char *pcId = NULL,
		*pcAddress = NULL,
		*pcSize = NULL;
	uint32_t uiId = 0,
			uiAddress = 0,
			uiSize = 0;
	uint32_t data = 0,
			 ret = 0,
			 offset = 0;

	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* Fill all parameters and there length from command strings */
	pcId = ( char * ) FreeRTOS_CLIGetParameter
        (
            pcCommandString,	/* The command string itself. */
            1U,           		/* Return the 1st parameter. */
            &xParameterIdStrLen	/* Store the parameter string length. */
        );

	pcAddress = ( char * ) FreeRTOS_CLIGetParameter
        (
            pcCommandString,	/* The command string itself. */
            2U,           		/* Return the 2nd parameter. */
            &xParameterAddressStrLen	/* Store the parameter string length. */
        );

	pcSize = ( char * ) FreeRTOS_CLIGetParameter
        (
            pcCommandString,	/* The command string itself. */
            3U,           		/* Return the 3rd parameter. */
            &xParameterSizeStrLen	/* Store the parameter string length. */
        );

	/* NULL Terminate all strings */
	pcId[ xParameterIdStrLen ] = 0x00;
	pcAddress[ xParameterAddressStrLen ] = 0x00;
	pcSize[ xParameterSizeStrLen ] = 0x00;

	uiId = strtoul( pcId,  (char **)NULL, BASE_HEXA );
	uiAddress = strtoul( pcAddress,  (char **)NULL, BASE_HEXA );
	uiSize = strtoul( pcSize,  (char **)NULL, BASE_HEXA );
	uiId = uiId - 1;

	log_dbg("\r\n %s: ID:%x, Address:%x, size:%x\r\n",__func__,
			uiId, uiAddress, uiSize);

	switch (uiId) {
		case PCIE_1:
			ret = PcieGetMode(uiId);
			if (ret == 1) {
				/* In RC Mode */
				log_err("%s: PCIE%d : Only EP mode supported !\r\n",
								__func__,GEUL_PCIE_ID(uiId));
			} else {
				/* In EP Mode */
				/* NOTE:
				 * Considered <uiAddress> given by the user is a valid PCIe Address.
				 * As there is no mechanism to validate the address. Incase of
				 * invalid address system may crash.
				 */
				log_info("PCIE%d : Data - \r\n",
								GEUL_PCIE_ID(uiId));
				for (uint32_t i=0; i < uiSize; i++)
				{
					offset = i*4;
					data = in_le32((uint32_t *)(uiAddress + offset));
					log_info("%s0x%08x ",(i%4)?"":"\r\n", data);
				}
				log_info("\r\n");
			}
			break;
		case PCIE_2:
			ret = PcieGetMode(uiId);
			if (ret == 1) {
				/* In RC MODE */
				log_err("%s: PCIE%d : In RC mode (Invalid Command) !!\r\n",
								__func__, GEUL_PCIE_ID(uiId));
			} else {
				/* In EP Mode */
				/* NOTE:
				 * Considered <uiAddress> given by the user is a valid PCIe Address.
				 * As there is no mechanism to validate the address. Incase of
				 * invalid address system may crash.
				 */
				log_info("PCIE%d : Data - \r\n",
								GEUL_PCIE_ID(uiId));
				for (uint32_t i=0; i < uiSize; i++)
				{
					offset = i*4;
					data = in_le32((uint32_t *)(uiAddress + offset));
					log_info("%s0x%08x ",(i%4)?"":"\r\n", data);
				}
				log_info("\r\n");
			}
			break;
		default:
			log_err("%s: PCIE%d : Invalid PCIe ID, use ID as 1 or 2 !\r\n",
								__func__, GEUL_PCIE_ID(uiId));
			break;
	};

	pcWriteBuffer[ 0 ] = 0x00;
	return pdFALSE;
}

/* \fn static portBASE_TYPE prvPCIeWriteRcMem(char *pcWriteBuffer,
 * 					size_t xWriteBufferLen, const char *pcCommandString)
 *
 * @brief : Function to write RC memory via PCIe. If PCIe controller
 * 	    	is working in EP mode it will write the data to EP memory
 * 	    	else will return error.
 * */
static portBASE_TYPE prvPCIeWriteRcMem(char *pcWriteBuffer,
		size_t xWriteBufferLen, const char *pcCommandString)
{
	BaseType_t xParameterIdStrLen = 0,
			xParameterAddressStrLen = 0,
			xParameterValueStrLen = 0;
	char *pcId = NULL,
		*pcAddress = NULL,
		*pcValue = NULL;
	uint32_t uiId = 0,
			uiAddress = 0,
			uiValue = 0,
			ret = 0;

	( void ) pcCommandString;
    ( void ) xWriteBufferLen;
    configASSERT( pcWriteBuffer );

	/* Fill all parameters and there length from command strings */
	pcId = ( char * ) FreeRTOS_CLIGetParameter
        (
            pcCommandString,	/* The command string itself. */
            1U,           		/* Return the First parameter. */
            &xParameterIdStrLen/* Store the parameter string length. */
        );

	pcAddress = ( char * ) FreeRTOS_CLIGetParameter
        (
            pcCommandString,		/* The command string itself. */
            2U,           			/* Return the second parameter. */
            &xParameterAddressStrLen/* Store the parameter string length. */
        );

	pcValue = ( char * ) FreeRTOS_CLIGetParameter
        (
            pcCommandString,		/* The command string itself. */
            3U,           			/* Return the third parameter. */
            &xParameterValueStrLen	/* Store the parameter string length. */
        );

	/* NULL Terminate all strings */
	pcId[ xParameterIdStrLen ] = 0x00;
	pcAddress[ xParameterAddressStrLen ] = 0x00;
	pcValue[ xParameterValueStrLen ] = 0x00;

	uiId = strtoul( pcId,  (char **)NULL, BASE_HEXA );
	uiAddress = strtoul( pcAddress,  (char **)NULL, BASE_HEXA );
	uiValue = strtoul( pcValue,  (char **)NULL, BASE_HEXA );
	uiId = uiId - 1;
	log_dbg("\r\n %s: ID:%x, Address:%x, Value:%x\r\n",
					__func__, uiId, uiAddress, uiValue);

	switch (uiId) {
		case PCIE_1:
			ret = PcieGetMode(uiId);
			if (ret == 1) {
				/* In RC Mode */
				log_err("%s: PCIE%d : Only EP mode supported !\r\n",
								__func__, GEUL_PCIE_ID(uiId));
			} else {
				/* In EP Mode */
				/* NOTE:
				 * Considered <uiAddress> given by user is valid. As there is no
				 * mechanism to validate the address. Incase of invalid address
				 * system may crash.
				 */
				out_le32((uint32_t *)uiAddress, uiValue);
				log_info("PCIE%d : Data written successfully.\r\n",
								GEUL_PCIE_ID(uiId));
			}
			break;

		case PCIE_2:
			ret = PcieGetMode(uiId);
			if (ret == 1) {
				/* In RC Mode */
				log_err("%s: PCIE%d : In RC mode (Invalid Command) !\r\n",
								__func__, GEUL_PCIE_ID(uiId));
			} else {
				/* In EP Mode */
				/* NOTE:
				 * Considered <uiAddress> given by user is valid. As there is no
				 * mechanism to validate the address. Incase of invalid address
				 * system may crash.
				 */
				out_le32((uint32_t *)uiAddress, uiValue);
				log_info("PCIE%d : Data written successfully.\r\n",
								GEUL_PCIE_ID(uiId));
			}
			break;
		default:
			log_err("%s: PCIE%d : Invalid PCIe ID, use ID as 1 or 2 !\r\n",
								__func__, GEUL_PCIE_ID(uiId));
			break;
	};

	pcWriteBuffer[ 0 ] = 0x00;
	return pdFALSE;
}
#endif /* LA12XX_DRIVER_PCI ends */

#if defined(LA12XX_DRIVER_PCI) || defined(LA12XX_DRIVER_PCI_LAT_FP)
/* \fn static portBASE_TYPE prvlsPCIe(char *pcWriteBuffer,
 * 				size_t xWriteBufferLen, const char *pcCommandString)
 *
 * @brief : Function to list PCIe controller with their config data.
 * 	    	It will print the config space data of the controller as
 * 	    	per the selected mode <i/v : info/verbose>. With info mode
 * 	    	,it will print standard 256B config space & with verbose
 * 	    	mode, it will print extended config space of 4K.
 * */
static portBASE_TYPE prvlsPCIe(char *pcWriteBuffer,
		size_t xWriteBufferLen, const char *pcCommandString)
{
	BaseType_t xParameterIdStrLen = 0;
	BaseType_t xParameterLsModeStrLen = 0;
	char *pcId = NULL,
	     *pcLsMode = NULL;
	uint32_t uiId = 0,
			 ret = 0,
			 ulAddress = 0,
			 data = 0,
			 offset = 0,
			 ulBusDev = 0,
			 configLen = 0,
			 numDword = 0;

	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* Fill all parameters and there length from command strings */
	pcId = ( char * ) FreeRTOS_CLIGetParameter
        (
            pcCommandString,	/* The command string itself. */
            1U,           		/* Return the 1st parameter. */
            &xParameterIdStrLen	/* Store the parameter string length. */
        );

	pcLsMode = ( char * ) FreeRTOS_CLIGetParameter
        (
            pcCommandString,	/* The command string itself. */
            2U,           		/* Return the 2nd parameter. */
            &xParameterLsModeStrLen /* Store the parameter string length. */
        );

	/* NULL Terminate all strings */
	pcId[ xParameterIdStrLen ] = 0x00;
	pcLsMode[ xParameterLsModeStrLen ] = 0x00;

	uiId = strtoul( pcId,  (char **)NULL, BASE_HEXA );
	uiId = uiId - 1;
	ulBusDev = GEUL_PCIE_ATU_BUS(1) | GEUL_PCIE_ATU_DEV(0) |
				GEUL_PCIE_ATU_FUNC(0);

	if (xParameterLsModeStrLen) {
		switch (pcLsMode[0]) {
			case 'v':
			case 'V':
				configLen = 1024*4; /* 4KB : Config Space with extended capabilities.*/
				break;
			case 'i':
			case 'I':
				configLen = 256; /* 256Bytes : Standard Config Space*/
				break;
			default:
				log_err("%s : PCIe%d : Invalid mode !\r\n",
							 __func__, GEUL_PCIE_ID(uiId));
				pcWriteBuffer[ 0 ] = 0x00;
				return pdFALSE;
		};
	}

	numDword = configLen/4;

	switch (uiId) {
		case PCIE_1:
			ret = PcieGetMode(uiId);
			if (ret == 1) {
				/* In RC Mode */
				log_err("%s : PCIe%d : Only EP mode supported !\r\n",
							 __func__, GEUL_PCIE_ID(uiId));
			} else {
				/* In EP MODE */
				log_info("PCIe%d (EP):\r\n", GEUL_PCIE_ID(uiId));
				ulAddress = PCIE_BASE_ADDR(PCIE_1);
				for (uint32_t i=0; i < numDword ; i++)
				{
					offset = i * 4;
					data = in_le32((uint32_t *)(ulAddress + offset));
					log_info("%s0x%08x ",(i%4)?"":"\r\n", data);
				}
				log_info("\r\n");
			}
			break;
		case PCIE_2:
			ret = PcieGetMode(uiId);
			if (ret == 1) {
				/* In RC MODE */
				log_info("PCIe%d: (RC)\r\n", GEUL_PCIE_ID(uiId));
				ulAddress = PCIE_BASE_ADDR(PCIE_2);
				for (uint32_t i=0; i < numDword ; i++)
				{
					offset = i * 4;
					data = in_le32((uint32_t *)(ulAddress + offset));
					log_info("%s0x%08x ",(i%4)?"":"\r\n", data);
				}
				log_info("\r\n*******************************************\r\n");
				log_info("PCIe%d: (RC:EP)", GEUL_PCIE_ID(uiId));
				for (uint32_t i=0; i < numDword; i++)
				{
					offset = i * 4;
					data = ulPcieReadConfig(uiId, ulBusDev, offset);
					log_info("%s0x%08x ",(i%4)?"":"\r\n", data);
				}
				log_info("\r\n*******************************************\r\n");
			} else {
				log_info("PCIe%d: (EP)\r\n", GEUL_PCIE_ID(uiId));
				ulAddress = PCIE_BASE_ADDR(PCIE_2);
				for (uint32_t i=0; i < numDword; i++)
				{
					offset = i * 4;
					data = in_le32((uint32_t *)(ulAddress + offset));
					log_info("%s0x%08x ",(i%4)?"":"\r\n", data);
				}
				log_info("\r\n*******************************************\r\n");
			}
			break;

		default:
			log_err("%s: PCIE%d : Invalid PCIe ID, use ID as 1 or 2 !\r\n",
								__func__, GEUL_PCIE_ID(uiId));
			break;
	};

	pcWriteBuffer[ 0 ] = 0x00;
	return pdFALSE;
}
#endif /* LA12XX_DRIVER_PCI||LA12XX_DRIVER_PCI_LAT_FP ends */

#ifdef LA12XX_DRIVER_PCI_LAT_FP
void print_dbgl(xConfigChannels_t *channel_config)
{
	log_info(" CH0_TXD_ADDR  0x%08x TBCNT 0x%08X\r\n", 
			channel_config->channelCfg[CHANNEL_0].tx_config.channelAddr,
			channel_config->channelCfg[CHANNEL_0].tx_config.channelSize);
	log_info(" CH1 TXD_ADDR  0x%08x TBCNT 0x%08X\r\n", 
			channel_config->channelCfg[CHANNEL_1].tx_config.channelAddr ,
			channel_config->channelCfg[CHANNEL_1].tx_config.channelSize);
	log_info(" CH2 TXD_ADDR  0x%08x TBCNT 0x%08X\r\n", 
			channel_config->channelCfg[CHANNEL_2].tx_config.channelAddr ,
			channel_config->channelCfg[CHANNEL_2].tx_config.channelSize);	
	log_info(" CH3 TXD_ADDR  0x%08x TBCNT 0x%08X\r\n\n", 
			channel_config->channelCfg[CHANNEL_3].tx_config.channelAddr ,
			channel_config->channelCfg[CHANNEL_3].tx_config.channelSize);

	log_info(" CH0_RXD_ADDR  0x%08x RBCNT 0x%08X\r\n", 
			channel_config->channelCfg[CHANNEL_0].rx_config.channelAddr ,
			channel_config->channelCfg[CHANNEL_0].rx_config.channelSize);
	log_info(" CH1 RXD_ADDR  0x%08x RBCNT 0x%08X\r\n", 
			channel_config->channelCfg[CHANNEL_1].rx_config.channelAddr ,
			channel_config->channelCfg[CHANNEL_1].rx_config.channelSize);
	log_info(" CH2 RXD_ADDR  0x%08x RBCNT 0x%08X\r\n",
			channel_config->channelCfg[CHANNEL_2].rx_config.channelAddr ,
			channel_config->channelCfg[CHANNEL_2].rx_config.channelSize);
	log_info(" CH3 RXD_ADDR  0x%08x RBCNT 0x%08X\r\n", 
			channel_config->channelCfg[CHANNEL_3].rx_config.channelAddr,
			channel_config->channelCfg[CHANNEL_3].rx_config.channelSize);

}

/* \fn static portBASE_TYPE prvPCIeThroughPut(char *pcWriteBuffer,
 * 				size_t xWriteBufferLen, const char *pcCommandString)
 *
 * @brief : Function to configure TX RX Buffers to Lattice FPGA and
 *			start DMA.
 * */

static portBASE_TYPE prvPCIeThroughPut(char *pcWriteBuffer,
		size_t xWriteBufferLen, const char *pcCommandString)
{
	BaseType_t xParameterTxAddrStrLen = 0,
		   xParameterRxAddrStrLen = 0,
		   xParameterBufSizeStrLen = 0,
		   xParameterTotRxSzAddrStrLen = 0,
		   xParameterTotTxSzAddrStrLen = 0;

	char *pcTxAddr = NULL,
	     *pcRxAddr = NULL,
	     *pcBufSize = NULL,
	     *pcTotRxSzAddr = NULL,
	     *pcTotTxSzAddr = NULL;

	uint32_t uiTxAddr = 0,
		 uiRxAddr = 0,
		 uiBufSize = 0,
		 uiTotRxSzAddr = 0,
		 uiTotTxSzAddr = 0;

	uint32_t *buf, *rxbuf;
	xConfigChannels_t *channel_config;
	xSetDma_t *enable_dma;
	uint32_t txDmaRate[2] = {0};
	uint32_t rxDmaRate[2] = {0};

	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* Fill all parameters and there length from command strings */
	pcTxAddr = ( char * ) FreeRTOS_CLIGetParameter
		(
		 pcCommandString,	/* The command string itself. */
		 1U,           		/* Return the First parameter. */
		 &xParameterTxAddrStrLen/* Store the parameter string length. */
		);

	pcRxAddr = ( char * ) FreeRTOS_CLIGetParameter
		(
		 pcCommandString,		/* The command string itself. */
		 2U,           			/* Return the second parameter. */
		 &xParameterRxAddrStrLen/* Store the parameter string length. */
		);

	pcBufSize = ( char * ) FreeRTOS_CLIGetParameter
		(
		 pcCommandString,		/* The command string itself. */
		 3U,           			/* Return the third parameter. */
		 &xParameterBufSizeStrLen	/* Store the parameter string length. */
		);

	pcTotRxSzAddr = ( char * ) FreeRTOS_CLIGetParameter
		(
		 pcCommandString,		/* The command string itself. */
		 4U,           			/* Return the third parameter. */
		 &xParameterTotRxSzAddrStrLen	/* Store the parameter string length. */
		);

	pcTotTxSzAddr = ( char * ) FreeRTOS_CLIGetParameter
		(
		 pcCommandString,		/* The command string itself. */
		 5U,           			/* Return the third parameter. */
		 &xParameterTotTxSzAddrStrLen	/* Store the parameter string length. */
		);

	/* NULL Terminate all strings */
	pcTxAddr[ xParameterTxAddrStrLen ] = 0x00;
	pcRxAddr[ xParameterRxAddrStrLen ] = 0x00;
	pcBufSize[ xParameterBufSizeStrLen ] = 0x00;
	pcTotRxSzAddr[ xParameterTotRxSzAddrStrLen ] = 0x00;
	pcTotTxSzAddr[ xParameterTotTxSzAddrStrLen ] = 0x00;


	uiTxAddr = strtoul( pcTxAddr,  (char **)NULL, BASE_HEXA );
	uiRxAddr = strtoul( pcRxAddr,  (char **)NULL, BASE_HEXA );
	uiBufSize = strtoul( pcBufSize,  (char **)NULL, BASE_HEXA );
	uiTotRxSzAddr = strtoul( pcTotRxSzAddr,  (char **)NULL, BASE_HEXA );
	uiTotTxSzAddr = strtoul( pcTotTxSzAddr,  (char **)NULL, BASE_HEXA );

	log_dbg("\r\n %s: TxAddr:%x, RxAddr:%x, BufSize:%x, TotRxSzAddr: %x, TotTxSzAddr: %x\r\n",
		__func__, uiTxAddr, uiRxAddr, uiBufSize, uiTotRxSzAddr, uiTotTxSzAddr );

	if(uiRxAddr < (uiTxAddr + uiBufSize))
		log_err(" RxAddr should be Greater than TxAddr * 4 * BufSize !\r\n");

	if (!uiBufSize)
		uiBufSize = 0x1000;


	if ( !(uiTotRxSzAddr) || !(uiTotTxSzAddr) ) {
		uiTotRxSzAddr = 0xe6000000;
		uiTotTxSzAddr = 0xe6001000;
		log_info(" TotRxSzAddr/TotTxSzAddr not defined using Tx:%x Rx:%x \r\n",
				uiTotRxSzAddr, uiTotTxSzAddr);
	}

	if ( !(uiTxAddr) || !(uiRxAddr) ) {
		buf = (uint32_t *)0xe5000000;
		rxbuf = (uint32_t *)(buf + uiBufSize);
		log_info(" TxAddr/RxAddr not defined using Tx:%x Rx:%x, Size:%x\r\n",
				buf, rxbuf, uiBufSize);
	} else {
		buf = (uint32_t *)uiTxAddr;
		rxbuf = (uint32_t *)uiRxAddr;
	}

	channel_config = (xConfigChannels_t *)pvPortMalloc(sizeof(xConfigChannels_t));
	configASSERT( channel_config ); 

	enable_dma = (xSetDma_t *)pvPortMalloc(sizeof(xSetDma_t));
	configASSERT( enable_dma ); 

	/* Fill TX Buffer with incremental Data */
	for(uint32_t i = 0; i < uiBufSize; i++) {
		out_le32(((uint32_t *)buf + i), i*4);
	}

	/* Fill RX buffer with deadbeef */
	for(uint32_t i = 0; i < uiBufSize; i++) {
		out_le32(((uint32_t *)rxbuf + i), 0xdeadbeef);
	}

	channel_config->channels = (CNFG_RX_CHANNEL_0 | CNFG_RX_CHANNEL_1 |
			CNFG_RX_CHANNEL_2 | CNFG_RX_CHANNEL_3 |
			CNFG_TX_CHANNEL_0 | CNFG_TX_CHANNEL_1 |
			CNFG_TX_CHANNEL_2 | CNFG_TX_CHANNEL_3 );

	//---------------------CH 0 ------------------------------------------------
	channel_config->channelCfg[CHANNEL_0].tx_config.channelTotalSizeAddr = 
		uiTotTxSzAddr;
	channel_config->channelCfg[CHANNEL_0].rx_config.channelTotalSizeAddr = 
		uiTotRxSzAddr;

	channel_config->channelCfg[CHANNEL_0].tx_config.channelAddr = (uint32_t)(buf);
	channel_config->channelCfg[CHANNEL_0].tx_config.channelSize = uiBufSize;

	channel_config->channelCfg[CHANNEL_0].rx_config.channelAddr = (uint32_t)(rxbuf);
	channel_config->channelCfg[CHANNEL_0].rx_config.channelSize = uiBufSize;

	//-------------------CH1 ----------------------------------------------------
	channel_config->channelCfg[CHANNEL_1].tx_config.channelTotalSizeAddr = 
		(uint32_t)uiTotTxSzAddr + CHANNEL_1*4;
	channel_config->channelCfg[CHANNEL_1].rx_config.channelTotalSizeAddr = 
		(uint32_t)uiTotRxSzAddr + CHANNEL_1*4;

	channel_config->channelCfg[CHANNEL_1].tx_config.channelAddr =
		(uint32_t)(buf + (CHANNEL_1 * (uiBufSize/4)));
	channel_config->channelCfg[CHANNEL_1].tx_config.channelSize = uiBufSize;

	channel_config->channelCfg[CHANNEL_1].rx_config.channelAddr =
		(uint32_t)(rxbuf + (CHANNEL_1 * (uiBufSize/4)));
	channel_config->channelCfg[CHANNEL_1].rx_config.channelSize = uiBufSize;

	//-------------------CH2-------------------------------------------------------
	channel_config->channelCfg[CHANNEL_2].tx_config.channelTotalSizeAddr = 
		(uint32_t)uiTotTxSzAddr + CHANNEL_2*4;
	channel_config->channelCfg[CHANNEL_2].rx_config.channelTotalSizeAddr = 
		(uint32_t)uiTotRxSzAddr + CHANNEL_2*4;

	channel_config->channelCfg[CHANNEL_2].tx_config.channelAddr  =
		(uint32_t)(buf + (CHANNEL_2 * (uiBufSize/4)));
	channel_config->channelCfg[CHANNEL_2].tx_config.channelSize = uiBufSize;


	channel_config->channelCfg[CHANNEL_2].rx_config.channelAddr =
		(uint32_t)(rxbuf + (CHANNEL_2 * (uiBufSize/4)));
	channel_config->channelCfg[CHANNEL_2].rx_config.channelSize = uiBufSize;

	//-------------------CH3-------------------------------------------------------
	channel_config->channelCfg[CHANNEL_3].tx_config.channelTotalSizeAddr = 
		(uint32_t)uiTotTxSzAddr + CHANNEL_3*4;
	channel_config->channelCfg[CHANNEL_3].rx_config.channelTotalSizeAddr = 
		(uint32_t)uiTotRxSzAddr + CHANNEL_3*4;
	channel_config->channelCfg[CHANNEL_3].tx_config.channelAddr =
		(uint32_t)(buf + (CHANNEL_3 * (uiBufSize/4)));
	channel_config->channelCfg[CHANNEL_3].tx_config.channelSize = uiBufSize;

	channel_config->channelCfg[CHANNEL_3].rx_config.channelAddr =
		(uint32_t)(rxbuf + (CHANNEL_3 * (uiBufSize/4)));
	channel_config->channelCfg[CHANNEL_3].rx_config.channelSize = uiBufSize;

	PcieEpConfigDmaBuf(channel_config);
	log_info("*** Checking Throughput ***\n\r");
	log_info(" *** Enabled Only CH0 *** \r\n");

	/* Enable DMA */
	enable_dma->selectChannel =  (SEL_RX_DMA_CH0 | SEL_TX_DMA_CH0);
	enable_dma->setChannel =  (EN_RX_DMA_CH0 | EN_TX_DMA_CH0);
	PcieEpSetDma (enable_dma);
	for(uint32_t j =0; j<5; j++) {
		txDmaRate[0] = in_le32((uint32_t *)uiTotTxSzAddr);
		rxDmaRate[0] = in_le32((uint32_t *)uiTotRxSzAddr);
		vUdelay(1000000);
		txDmaRate[1] = in_le32((uint32_t *)uiTotTxSzAddr);
		rxDmaRate[1] = in_le32((uint32_t *)uiTotRxSzAddr);
		log_info("CH0 : Throughput : TX: %d Bytes/sec RX: %d Bytes/sec \r\n",
				(txDmaRate[1]-txDmaRate[0]),  (rxDmaRate[1]- rxDmaRate[0]) );
	}

	/* Disable DMA */
	enable_dma->selectChannel = (SEL_RX_DMA_CH0 | SEL_TX_DMA_CH0);
	enable_dma->setChannel =  0x0;
	PcieEpSetDma (enable_dma);
	//---------------------------------------------------------------------------
	enable_dma->selectChannel = (SEL_RX_DMA_CH0 | SEL_RX_DMA_CH1 | SEL_RX_DMA_CH2 |
			SEL_RX_DMA_CH3 | SEL_TX_DMA_CH0 | SEL_TX_DMA_CH1 |
			SEL_TX_DMA_CH2 | SEL_TX_DMA_CH3);

	enable_dma->setChannel = (EN_RX_DMA_CH0 | EN_RX_DMA_CH1 | EN_RX_DMA_CH2 |
			EN_RX_DMA_CH3 | EN_TX_DMA_CH0 | EN_TX_DMA_CH1 |
			EN_TX_DMA_CH2 | EN_TX_DMA_CH3 );

	PcieEpSetDma (enable_dma);
	//---------------------------------------------------------------------------
	log_info(" *** Enabled All channels *** \r\n");
	log_info("Channel 0: \r\n");
	for(uint32_t j =0; j<5; j++) {
		txDmaRate[0] = in_le32((uint32_t *)uiTotTxSzAddr);
		rxDmaRate[0] = in_le32((uint32_t *)uiTotRxSzAddr);
		vUdelay(1000000);
		txDmaRate[1] = in_le32((uint32_t *)uiTotTxSzAddr);
		rxDmaRate[1] = in_le32((uint32_t *)uiTotRxSzAddr);
		log_info("CH0 : Throughput : TX: %d Bytes/sec RX: %d Bytes/sec \r\n",
				(txDmaRate[1]-txDmaRate[0]),  (rxDmaRate[1]- rxDmaRate[0]) );
	}
	log_info("Channel 1: \r\n");
	for(uint32_t j =0; j<5; j++) {
		txDmaRate[0] = in_le32((uint32_t *)(uiTotTxSzAddr + CHANNEL_1*4));
		rxDmaRate[0] = in_le32((uint32_t *)(uiTotRxSzAddr + CHANNEL_1*4));
		vUdelay(1000000);
		txDmaRate[1] = in_le32((uint32_t *)(uiTotTxSzAddr + CHANNEL_1*4));
		rxDmaRate[1] = in_le32((uint32_t *)(uiTotRxSzAddr + CHANNEL_1*4));
		log_info("CH0 : Throughput : TX: %d Bytes/sec RX: %d Bytes/sec \r\n",
				(txDmaRate[1]-txDmaRate[0]),  (rxDmaRate[1]- rxDmaRate[0]) );
	}
	log_info("Channel 2: \r\n");
	for(uint32_t j =0; j<5; j++) {
		txDmaRate[0] = in_le32((uint32_t *)(uiTotTxSzAddr + CHANNEL_2*4));
		rxDmaRate[0] = in_le32((uint32_t *)(uiTotRxSzAddr + CHANNEL_2*4));
		vUdelay(1000000);
		txDmaRate[1] = in_le32((uint32_t *)(uiTotTxSzAddr + CHANNEL_2*4));
		rxDmaRate[1] = in_le32((uint32_t *)(uiTotRxSzAddr + CHANNEL_2*4));
		log_info("CH0 : Throughput : TX: %d Bytes/sec RX: %d Bytes/sec \r\n",
				(txDmaRate[1]-txDmaRate[0]),  (rxDmaRate[1]- rxDmaRate[0]) );
	}
	log_info("Channel 3: \r\n");
	for(uint32_t j =0; j<5; j++) {
		txDmaRate[0] = in_le32((uint32_t *)(uiTotTxSzAddr + CHANNEL_3*4));
		rxDmaRate[0] = in_le32((uint32_t *)(uiTotRxSzAddr + CHANNEL_3*4));
		vUdelay(1000000);
		txDmaRate[1] = in_le32((uint32_t *)(uiTotTxSzAddr + CHANNEL_3*4));
		rxDmaRate[1] = in_le32((uint32_t *)(uiTotRxSzAddr + CHANNEL_3*4));
		log_info("CH0 : Throughput : TX: %d Bytes/sec RX: %d Bytes/sec \r\n",
				(txDmaRate[1]-txDmaRate[0]),  (rxDmaRate[1]- rxDmaRate[0]) );
	}

	/* Disable DMA */
	enable_dma->selectChannel = (SEL_RX_DMA_CH0 | SEL_RX_DMA_CH1 | SEL_RX_DMA_CH2 |
			SEL_RX_DMA_CH3 | SEL_TX_DMA_CH0 | SEL_TX_DMA_CH1 |
			SEL_TX_DMA_CH2 | SEL_TX_DMA_CH3);
	enable_dma->setChannel =  0x0;
	PcieEpSetDma (enable_dma);

	vPortFree(channel_config);
	vPortFree(enable_dma);

	pcWriteBuffer[ 0 ] = 0x00;
	return pdFALSE;
} 


/* \fn static portBASE_TYPE prvPCIeRxData(char *pcWriteBuffer,
 * 				size_t xWriteBufferLen, const char *pcCommandString)
 *
 * @brief : Function to configure TX RX Buffers to Lattice FPGA and
 *			start DMA.
 * */
static portBASE_TYPE prvPCIeRxData(char *pcWriteBuffer,
		size_t xWriteBufferLen, const char *pcCommandString)
{
	BaseType_t xParameterRxAddrStrLen = 0,
		   xParameterBufSizeStrLen = 0;
	char *pcRxAddr = NULL,
	     *pcBufSize = NULL;
	uint32_t uiRxAddr = 0,
		 uiBufSize = 0;
	uint32_t *rxbuf;
	xSetDma_t *enable_dma;

	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	pcRxAddr = ( char * ) FreeRTOS_CLIGetParameter
		(
		 pcCommandString,		/* The command string itself. */
		 1U,           			/* Return the second parameter. */
		 &xParameterRxAddrStrLen/* Store the parameter string length. */
		);

	pcBufSize = ( char * ) FreeRTOS_CLIGetParameter
		(
		 pcCommandString,		/* The command string itself. */
		 2U,           			/* Return the third parameter. */
		 &xParameterBufSizeStrLen	/* Store the parameter string length. */
		);

	/* NULL Terminate all strings */
	pcRxAddr[ xParameterRxAddrStrLen ] = 0x00;
	pcBufSize[ xParameterBufSizeStrLen ] = 0x00;

	uiRxAddr = strtoul( pcRxAddr,  (char **)NULL, BASE_HEXA );
	uiBufSize = strtoul( pcBufSize,  (char **)NULL, BASE_HEXA );

	log_dbg("\r\n %s: RxAddr:%x, BufSize:%x\r\n",
			__func__, uiRxAddr, uiBufSize);

	if (!uiBufSize)
		uiBufSize = 0x1000;

	if (!(uiRxAddr)) {
		rxbuf = (uint32_t *)(0xe5000000 + uiBufSize/4);
		log_info(" RxAddr not defined using Rx:%x, Size:%x", rxbuf, uiBufSize);
	} else {
		rxbuf = (uint32_t *)uiRxAddr;
	}

	enable_dma = (xSetDma_t *)pvPortMalloc(sizeof(xSetDma_t));
	configASSERT( enable_dma ); 

	enable_dma->selectChannel = (SEL_RX_DMA_CH0 | SEL_RX_DMA_CH1 | SEL_RX_DMA_CH2 |
			SEL_RX_DMA_CH3 | SEL_TX_DMA_CH0 | SEL_TX_DMA_CH1 |
			SEL_TX_DMA_CH2 | SEL_TX_DMA_CH3);

	enable_dma->setChannel =  0x0;
	PcieEpSetDma (enable_dma);

	log_info("---------------------------------------------------------------\r\n");
	log_info("----------------------------- RX 0 ----------------------------\r\n");
	for(int j = 0; j < 0x10; j++) {
		log_info("Test::: %p : 0x%lx \r\n", ((uint32_t *)rxbuf + j),
				in_le32(((uint32_t *)rxbuf + j)));
	}
	log_info("----------------------------- RX 1 ----------------------------\r\n");

	for(int j = 0; j < 0x10; j++) {
		log_info("Test::: %p : 0x%lx \r\n", (uint32_t *)(rxbuf + (CHANNEL_1 * uiBufSize)/4 + j),
				in_le32((uint32_t *)(rxbuf + (CHANNEL_1 * uiBufSize)/4 + j)));
	}
	log_info("----------------------------- RX 2 ----------------------------\r\n");

	for(int j = 0; j < 0x10; j++) {
		log_info("Test::: %p : 0x%lx \r\n", (uint32_t *)(rxbuf + (CHANNEL_2 * uiBufSize)/4 + j),
				in_le32((uint32_t *)(rxbuf + (CHANNEL_2 * uiBufSize)/4 + j)));
	}
	log_info("----------------------------- RX 3 ----------------------------\r\n");

	for(int j = 0; j < 0x10; j++) {
		log_info("Test::: %p : 0x%lx \r\n", (uint32_t *)(rxbuf + (CHANNEL_3 * uiBufSize)/4 + j),
				in_le32((uint32_t *)(rxbuf + (CHANNEL_3 * uiBufSize)/4 + j)));
	}
	log_info("---------------------------------------------------------------\r\n");

	vPortFree(enable_dma);
	pcWriteBuffer[ 0 ] = 0x00;

	return pdFALSE;
} 

/* \fn static portBASE_TYPE prvPCIeTxData(char *pcWriteBuffer,
 * 				size_t xWriteBufferLen, const char *pcCommandString)
 *
 * @brief : Function to configure TX RX Buffers to Lattice FPGA and
 *			start DMA.
 * */
static portBASE_TYPE prvPCIeTxData(char *pcWriteBuffer,
		size_t xWriteBufferLen, const char *pcCommandString)
{
	BaseType_t xParameterTxAddrStrLen = 0,
		   xParameterRxAddrStrLen = 0,
		   xParameterBufSizeStrLen = 0,
		   xParameterTotRxSzAddrStrLen = 0,
		   xParameterTotTxSzAddrStrLen = 0;

	char *pcTxAddr = NULL,
	     *pcRxAddr = NULL,
	     *pcBufSize = NULL,
	     *pcTotRxSzAddr = NULL,
	     *pcTotTxSzAddr = NULL;

	uint32_t uiTxAddr = 0,
		 uiRxAddr = 0,
		 uiBufSize = 0,
		 uiTotRxSzAddr = 0,
		 uiTotTxSzAddr = 0;
	uint32_t *buf, *rxbuf;
	xConfigChannels_t *channel_config;
	xSetDma_t *enable_dma;

	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* Fill all parameters and there length from command strings */
	pcTxAddr = ( char * ) FreeRTOS_CLIGetParameter
		(
		 pcCommandString,	/* The command string itself. */
		 1U,           		/* Return the First parameter. */
		 &xParameterTxAddrStrLen/* Store the parameter string length. */
		);

	pcRxAddr = ( char * ) FreeRTOS_CLIGetParameter
		(
		 pcCommandString,		/* The command string itself. */
		 2U,           			/* Return the second parameter. */
		 &xParameterRxAddrStrLen/* Store the parameter string length. */
		);

	pcBufSize = ( char * ) FreeRTOS_CLIGetParameter
		(
		 pcCommandString,		/* The command string itself. */
		 3U,           			/* Return the third parameter. */
		 &xParameterBufSizeStrLen	/* Store the parameter string length. */
		);

	pcTotRxSzAddr = ( char * ) FreeRTOS_CLIGetParameter
		(
		 pcCommandString,		/* The command string itself. */
		 4U,           			/* Return the third parameter. */
		 &xParameterTotRxSzAddrStrLen	/* Store the parameter string length. */
		);

	pcTotTxSzAddr = ( char * ) FreeRTOS_CLIGetParameter
		(
		 pcCommandString,		/* The command string itself. */
		 5U,           			/* Return the third parameter. */
		 &xParameterTotTxSzAddrStrLen	/* Store the parameter string length. */
		);

	/* NULL Terminate all strings */
	pcTxAddr[ xParameterTxAddrStrLen ] = 0x00;
	pcRxAddr[ xParameterRxAddrStrLen ] = 0x00;
	pcBufSize[ xParameterBufSizeStrLen ] = 0x00;
	pcTotRxSzAddr[ xParameterTotRxSzAddrStrLen ] = 0x00;
	pcTotTxSzAddr[ xParameterTotTxSzAddrStrLen ] = 0x00;


	uiTxAddr = strtoul( pcTxAddr,  (char **)NULL, BASE_HEXA );
	uiRxAddr = strtoul( pcRxAddr,  (char **)NULL, BASE_HEXA );
	uiBufSize = strtoul( pcBufSize,  (char **)NULL, BASE_HEXA );
	uiTotRxSzAddr = strtoul( pcTotRxSzAddr,  (char **)NULL, BASE_HEXA );
	uiTotTxSzAddr = strtoul( pcTotTxSzAddr,  (char **)NULL, BASE_HEXA );

	log_dbg("\r\n %s: TxAddr:%x, RxAddr:%x, BufSize:%x, TotRxSzAddr: %x, TotTxSzAddr: %x\r\n",
			__func__, uiTxAddr, uiRxAddr, uiBufSize, uiTotRxSzAddr, uiTotTxSzAddr );

	if(uiRxAddr < (uiTxAddr + uiBufSize))
		log_err(" RxAddr should be Greater than TxAddr * 4 * BufSize !\r\n");

	if (!uiBufSize)
		uiBufSize = 0x1000;

	if ( !(uiTxAddr) || !(uiRxAddr) ) {
		buf = (uint32_t *)0xe5000000;
		rxbuf = (uint32_t *)(buf + uiBufSize);
		log_info(" TxAddr/RxAddr not defined using Tx:%x Rx:%x, Size:%x\r\n", buf,
				rxbuf, uiBufSize);
	} else {
		buf = (uint32_t *)uiTxAddr;
		rxbuf = (uint32_t *)uiRxAddr;
	}

	channel_config = (xConfigChannels_t *)pvPortMalloc(sizeof(xConfigChannels_t));
	configASSERT( channel_config ); 

	enable_dma = (xSetDma_t *)pvPortMalloc(sizeof(xSetDma_t));
	configASSERT( enable_dma ); 

	/* Fill TX Buffer with incremental Data */
	for(uint32_t i = 0; i < uiBufSize; i++) {
		out_le32(((uint32_t *)buf + i), i*4);
	}

	/* Fill RX buffer with deadbeef */
	for(uint32_t i = 0; i < uiBufSize; i++) {
		out_le32(((uint32_t *)rxbuf + i), 0xdeadbeef);
	}

	channel_config->channels = (CNFG_RX_CHANNEL_0 | CNFG_RX_CHANNEL_1 |
			CNFG_RX_CHANNEL_2 | CNFG_RX_CHANNEL_3 |
			CNFG_TX_CHANNEL_0 | CNFG_TX_CHANNEL_1 |
			CNFG_TX_CHANNEL_2 | CNFG_TX_CHANNEL_3 );

	//---------------------CH 0 ------------------------------------------------
	channel_config->channelCfg[CHANNEL_0].tx_config.channelTotalSizeAddr = 
		uiTotRxSzAddr;
	channel_config->channelCfg[CHANNEL_0].rx_config.channelTotalSizeAddr = 
		uiTotTxSzAddr;

	channel_config->channelCfg[CHANNEL_0].tx_config.channelAddr = (uint32_t)(buf);
	channel_config->channelCfg[CHANNEL_0].tx_config.channelSize = uiBufSize;

	channel_config->channelCfg[CHANNEL_0].rx_config.channelAddr = (uint32_t)(rxbuf);
	channel_config->channelCfg[CHANNEL_0].rx_config.channelSize = uiBufSize;

	//-------------------CH1 ----------------------------------------------------
	channel_config->channelCfg[CHANNEL_1].tx_config.channelTotalSizeAddr = 
		(uint32_t)uiTotRxSzAddr + CHANNEL_1*4;
	channel_config->channelCfg[CHANNEL_1].rx_config.channelTotalSizeAddr = 
		(uint32_t)uiTotTxSzAddr + CHANNEL_1*4;

	channel_config->channelCfg[CHANNEL_1].tx_config.channelAddr =
		(uint32_t)(buf + (CHANNEL_1 * (uiBufSize/4)));
	channel_config->channelCfg[CHANNEL_1].tx_config.channelSize = uiBufSize;

	channel_config->channelCfg[CHANNEL_1].rx_config.channelAddr =
		(uint32_t)(rxbuf + (CHANNEL_1 * (uiBufSize/4)));
	channel_config->channelCfg[CHANNEL_1].rx_config.channelSize = uiBufSize;

	//-------------------CH2-------------------------------------------------------
	channel_config->channelCfg[CHANNEL_2].tx_config.channelTotalSizeAddr = 
		(uint32_t)uiTotRxSzAddr + CHANNEL_2*4;
	channel_config->channelCfg[CHANNEL_2].rx_config.channelTotalSizeAddr = 
		(uint32_t)uiTotTxSzAddr + CHANNEL_2*4;

	channel_config->channelCfg[CHANNEL_2].tx_config.channelAddr  =
		(uint32_t)(buf + (CHANNEL_2 * (uiBufSize/4)));
	channel_config->channelCfg[CHANNEL_2].tx_config.channelSize = uiBufSize;


	channel_config->channelCfg[CHANNEL_2].rx_config.channelAddr =
		(uint32_t)(rxbuf + (CHANNEL_2 * (uiBufSize/4)));
	channel_config->channelCfg[CHANNEL_2].rx_config.channelSize = uiBufSize;

	//-------------------CH3-------------------------------------------------------
	channel_config->channelCfg[CHANNEL_3].tx_config.channelTotalSizeAddr = 
		(uint32_t)uiTotRxSzAddr + CHANNEL_3*4;
	channel_config->channelCfg[CHANNEL_3].rx_config.channelTotalSizeAddr = 
		(uint32_t)uiTotTxSzAddr + CHANNEL_3*4;
	channel_config->channelCfg[CHANNEL_3].tx_config.channelAddr =
		(uint32_t)(buf + (CHANNEL_3 * (uiBufSize/4)));
	channel_config->channelCfg[CHANNEL_3].tx_config.channelSize = uiBufSize;

	channel_config->channelCfg[CHANNEL_3].rx_config.channelAddr =
		(uint32_t)(rxbuf + (CHANNEL_3 * (uiBufSize/4)));
	channel_config->channelCfg[CHANNEL_3].rx_config.channelSize = uiBufSize;

	PcieEpConfigDmaBuf(channel_config);
	log_info("-------------------------------------------------------------\r\n");
	log_info("          		DMA CONFIG \r\n");
	log_info("-------------------------------------------------------------\r\n");
	print_dbgl(channel_config);
	log_info("-------------------------------------------------------------\r\n");
	log_info("--------------------------TX 0-------------------------------\r\n");
	for(int j = 0; j < 0x10; j++) {
		log_info("Test::: %p : 0x%lx \r\n", ((uint32_t *)(buf + j)),
				in_le32(((uint32_t *)(buf + j))));
	}
	log_info("--------------------------TX 1-------------------------------\r\n");

	for(int j = 0; j < 0x10; j++) {
		log_info("Test::: %p : 0x%lx \r\n", (uint32_t *)(buf + (CHANNEL_1 * uiBufSize)/4 + j),
				in_le32((uint32_t *)(buf + (CHANNEL_1 * uiBufSize)/4 + j)));
	}
	log_info("--------------------------TX 2-------------------------------\r\n");

	for(int j = 0; j < 0x10; j++) {
		log_info("Test::: %p : 0x%lx \r\n", (uint32_t *)(buf + (CHANNEL_2 * uiBufSize)/4 + j),
				in_le32((uint32_t *)(buf + (CHANNEL_2 * uiBufSize)/4 + j)));
	}
	log_info("--------------------------TX 3-------------------------------\r\n");

	for(int j = 0; j < 0x10; j++) {
		log_info("Test::: %p : 0x%lx \r\n", (uint32_t *)(buf + (CHANNEL_3 * uiBufSize)/4 + j),
				in_le32((uint32_t *)(buf + (CHANNEL_3 * uiBufSize)/4 + j)));
	}
	log_info("--------------------------------------------------------------\r\n");

	log_info("---------------------------------------------------------------\r\n");
	log_info("----------------------------- RX 0 ----------------------------\r\n");
	for(int j = 0; j < 0x10; j++) {
		log_info("Test::: %p : 0x%lx \r\n", ((uint32_t *)rxbuf + j),
				in_le32(((uint32_t *)rxbuf + j)));
	}
	log_info("----------------------------- RX 1 ----------------------------\r\n");

	for(int j = 0; j < 0x10; j++) {
		log_info("Test::: %p : 0x%lx \r\n", (uint32_t *)(rxbuf + (CHANNEL_1 * uiBufSize)/4 + j),
				in_le32((uint32_t *)(rxbuf + (CHANNEL_1 * uiBufSize)/4 + j)));
	}
	log_info("----------------------------- RX 2 ----------------------------\r\n");

	for(int j = 0; j < 0x10; j++) {
		log_info("Test::: %p : 0x%lx \r\n", (uint32_t *)(rxbuf + (CHANNEL_2 * uiBufSize)/4 + j),
				in_le32((uint32_t *)(rxbuf + (CHANNEL_2 * uiBufSize)/4 + j)));
	}
	log_info("----------------------------- RX 3 ----------------------------\r\n");

	for(int j = 0; j < 0x10; j++) {
		log_info("Test::: %p : 0x%lx \r\n", (uint32_t *)(rxbuf + (CHANNEL_3 * uiBufSize)/4 + j),
				in_le32((uint32_t *)(rxbuf + (CHANNEL_3 * uiBufSize)/4 + j)));
	}
	log_info("---------------------------------------------------------------\r\n");

	/* Enable DMA for All Channels */
#if 1
	enable_dma->selectChannel =  (SEL_RX_DMA_CH0 | SEL_RX_DMA_CH1 | SEL_RX_DMA_CH2 |
			SEL_RX_DMA_CH3 | SEL_TX_DMA_CH0 | SEL_TX_DMA_CH1 |
			SEL_TX_DMA_CH2 | SEL_TX_DMA_CH3);

	enable_dma->setChannel =  (EN_RX_DMA_CH0 | EN_RX_DMA_CH1 | EN_RX_DMA_CH2 |
			EN_RX_DMA_CH3 | EN_TX_DMA_CH0 | EN_TX_DMA_CH1 |
			EN_TX_DMA_CH2 | EN_TX_DMA_CH3 );

#else /* Enable DMA only CHANNEL 0 */
	enable_dma->selectChannel =  (SEL_RX_DMA_CH0 | SEL_TX_DMA_CH0);
	enable_dma->setChannel =  (EN_RX_DMA_CH0 | EN_TX_DMA_CH0);
#endif 
	PcieEpSetDma (enable_dma);
	vPortFree(channel_config);
	vPortFree(enable_dma);
	pcWriteBuffer[ 0 ] = 0x00;
	return pdFALSE;
}
#endif /* LA12XX_DRIVER_PCI_LAT_FP ends */

static portBASE_TYPE prvI2cWriteCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{

        BaseType_t xParameter1StringLength = 0;
        BaseType_t xParameter2StringLength  = 0;
        BaseType_t xParameter3StringLength  = 0;
        BaseType_t xParameter4StringLength  = 0;
        BaseType_t xParameter5StringLength  = 0;
        BaseType_t xParameter6StringLength  = 0;
        char * pcBusNo = NULL, *pcBaseAddress = NULL, * pcOffSet = NULL, *pcLen = NULL, *pcOffLen=NULL;
        char *pcData=NULL;
        unsigned  uiBusNo, uiDeviceBaseAddress, uiOffSet;
        uint32_t uiData, uiLen;
        uint32_t uiBusBaseAddress, uiOffLen, uiOffLen2;
        int ret;
       /* Remove compile time warnings about unused parameters, and check the
           write buffer is not NULL.  NOTE - for simplicity, this example assumes the
           write buffer length is adequate, so does not check for buffer overflows. */
        ( void ) pcCommandString;
        ( void ) xWriteBufferLen;
        configASSERT( pcWriteBuffer );

        pcBusNo = ( char * ) FreeRTOS_CLIGetParameter
                (
                 pcCommandString,               /* The command string itself. */
                 1U,           /* Return the next parameter. */
                 &xParameter1StringLength       /* Store the parameter string length. */
                );


        pcBaseAddress = ( char * ) FreeRTOS_CLIGetParameter
                (
                 pcCommandString,               /* The command string itself. */
                 2U,           /* Return the next parameter. */
                 &xParameter2StringLength       /* Store the parameter string length. */
                );
        pcOffSet = ( char * ) FreeRTOS_CLIGetParameter
                (
                 pcCommandString,               /* The command string itself. */
                 3U,           /* Return the next parameter. */
                 &xParameter3StringLength       /* Store the parameter string length. */
                );
        pcData = ( char * ) FreeRTOS_CLIGetParameter
                (
                 pcCommandString,               /* The command string itself. */
                 4U,           /* Return the next parameter. */
                 &xParameter4StringLength       /* Store the parameter string length. */
                );
        pcLen = ( char * ) FreeRTOS_CLIGetParameter
                (
                 pcCommandString,               /* The command string itself. */
                 5U,           /* Return the next parameter. */
                 &xParameter5StringLength       /* Store the parameter string length. */
                );
	pcOffLen = (char *)  FreeRTOS_CLIGetParameter
		(
		pcCommandString,
		6U,
		&xParameter6StringLength
		);	

        /* Terminate pcFunctionName, pcCore & pcIterations */
        pcBusNo[ xParameter1StringLength ] = 0x00;
        pcBaseAddress[ xParameter2StringLength ] = 0x00;
        pcOffSet[ xParameter3StringLength ] = 0x00;
        pcData  [ xParameter4StringLength ] = 0x00;
        pcLen[ xParameter5StringLength ] = 0x00;
        pcOffLen  [ xParameter6StringLength ] = 0x00;

        uiBusNo                 = strtoul( pcBusNo      , (char **)NULL, BASE_DEC );
        uiDeviceBaseAddress     = strtoul( pcBaseAddress, (char **)NULL, BASE_HEXA );
        uiOffSet                = strtoul( pcOffSet, (char **)NULL, BASE_HEXA );
        uiData                  = strtoul( pcData, (char **)NULL, BASE_HEXA );
		uiLen  					= strtoul( pcLen, (char **)NULL, BASE_DEC );
	uiOffLen		= strtoul( pcOffLen, (char **)NULL, BASE_DEC );

		uint8_t *puiData = (uint8_t *)&uiData;
		uint8_t uiVal[uiLen];

	log_dbg("\n\ricwrite %d 0x%x 0x%x 0x%x %d",uiBusNo, uiDeviceBaseAddress, uiOffSet, uiData, uiOffLen );
	if(uiBusNo==1)
		uiBusBaseAddress=I2C1_BASE_ADDR;
	else if (uiBusNo==2)
		uiBusBaseAddress=I2C2_BASE_ADDR;
	else if (uiBusNo==3)
		uiBusBaseAddress=I2C3_BASE_ADDR;
	else
	{
		log_info("\n\rInvalid Bus no, only Bus no 1,2 and 3 are initialized");
		return pdFALSE;
	}
	switch(uiOffLen)
	{
		case 1:uiOffLen2=I2C_DEV_OFFSET_LEN_1_BYTE;break;
		case 2:uiOffLen2=I2C_DEV_OFFSET_LEN_2_BYTE;break;
		case 3:uiOffLen2=I2C_DEV_OFFSET_LEN_3_BYTE;break;
		case 4:uiOffLen2=I2C_DEV_OFFSET_LEN_4_BYTE;break;
		default:log_err("\n\r Invalid Offset Length"); return pdFALSE;
	}

		if (uiData > 0xFFFFFFFF)
		{
			log_err("\n\r Invalid Write Data size\n\r"); 
			return pdFALSE;
		}

	   for (uint32_t i = 0, j = ( 4 - uiLen ); i < uiLen; i++, j++)
	   {
			   uiVal[i] = puiData[j];
	   }

        ret = iI2C_Write(uiBusBaseAddress, uiDeviceBaseAddress, uiOffSet, uiOffLen2, uiVal, uiLen);

        if(ret < 0)
        {
                log_err("\n\ri2c_write failed %d", ret);
        }
	else
		log_info("\n\r Write Successful ");
        /* No more parameters were found.  Make sure the write buffer does
           not contain a valid string. */
        pcWriteBuffer[ 0 ] = 0x00;
	return pdFALSE;
}

static portBASE_TYPE prvI2cReadCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{

        BaseType_t xParameter1StringLength  = 0;
        BaseType_t xParameter2StringLength  = 0;
        BaseType_t xParameter3StringLength  = 0;
        BaseType_t xParameter4StringLength  = 0;
        BaseType_t xParameter5StringLength  = 0;
        char * pcBusNo = NULL, *pcBaseAddress = NULL, * pcOffSet = NULL, *pcLen=NULL, *pcOffLen=NULL;
        unsigned  uiBusNo, uiDeviceBaseAddress, uiOffSet, uiLen, uiOffLen, uiOffLen2,i;
	uint32_t uiBusBaseAddress;
	int ret;

       /* Remove compile time warnings about unused parameters, and check the
           write buffer is not NULL.  NOTE - for simplicity, this example assumes the
           write buffer length is adequate, so does not check for buffer overflows. */
        ( void ) pcCommandString;
        ( void ) xWriteBufferLen;
        configASSERT( pcWriteBuffer );

        pcBusNo = ( char * ) FreeRTOS_CLIGetParameter
                (
                 pcCommandString,               /* The command string itself. */
                 1U,           /* Return the next parameter. */
                 &xParameter1StringLength       /* Store the parameter string length. */
                );


        pcBaseAddress = ( char * ) FreeRTOS_CLIGetParameter
                (
                 pcCommandString,               /* The command string itself. */
                 2U,           /* Return the next parameter. */
                 &xParameter2StringLength       /* Store the parameter string length. */
                );
        pcOffSet = ( char * ) FreeRTOS_CLIGetParameter
                (
                 pcCommandString,               /* The command string itself. */
                 3U,           /* Return the next parameter. */
                 &xParameter3StringLength       /* Store the parameter string length. */
                );
        pcLen = ( char * ) FreeRTOS_CLIGetParameter
                (
                 pcCommandString,               /* The command string itself. */
                 4U,           /* Return the next parameter. */
                 &xParameter4StringLength       /* Store the parameter string length. */
                );

        pcOffLen = (char *)  FreeRTOS_CLIGetParameter
                (
                pcCommandString,
                5U,
                &xParameter5StringLength
                );

        /* Terminate pcFunctionName, pcCore & pcIterations */
        pcBusNo[ xParameter1StringLength ] = 0x00;
        pcBaseAddress[ xParameter2StringLength ] = 0x00;
        pcOffSet[ xParameter3StringLength ] = 0x00;
	pcLen[ xParameter4StringLength ] = 0x00;
	pcOffLen[ xParameter5StringLength ] = 0x00;


	uiBusNo         = strtoul( pcBusNo      , (char **)NULL, BASE_DEC );	
	uiDeviceBaseAddress   = strtoul( pcBaseAddress, (char **)NULL, BASE_HEXA );	
	uiOffSet        = strtoul( pcOffSet, (char **)NULL, BASE_HEXA );	
	uiLen  		= strtoul( pcLen, (char **)NULL, BASE_DEC );	
	uiOffLen	= strtoul( pcOffLen, (char **)NULL, BASE_DEC );

	uint8_t ucval[uiLen];

	PRINTF("\n\ri2cread %d 0x%x 0x%x %d %d",uiBusNo, uiDeviceBaseAddress, uiOffSet, uiLen, uiOffLen );
	if(uiBusNo==1)
		uiBusBaseAddress=I2C1_BASE_ADDR;
	else if (uiBusNo==2)
		uiBusBaseAddress=I2C2_BASE_ADDR;
	else if (uiBusNo==3)
		uiBusBaseAddress=I2C3_BASE_ADDR;
	else
		{
			log_info("\n\rInvalid Bus no, only Bus no 1,2 and 3 are initialized");
			return pdFALSE;
		}

        switch(uiOffLen)
        {
                case 1:	uiOffLen2=I2C_DEV_OFFSET_LEN_1_BYTE;break;
                case 2:	uiOffLen2=I2C_DEV_OFFSET_LEN_2_BYTE;break;
                case 3: uiOffLen2=I2C_DEV_OFFSET_LEN_3_BYTE;break;
                case 4: uiOffLen2=I2C_DEV_OFFSET_LEN_4_BYTE;break;
		default:log_err("\n\r Invalid Offset Length"); return pdFALSE;
        }

	ret = iI2C_Read(uiBusBaseAddress, uiDeviceBaseAddress, uiOffSet , uiOffLen2, ucval, uiLen);
	if (ret < 0) {
		PRINTF("iI2C_read failed ret: %d\n\r", ret);
		return pdFALSE;
	}
	else {
		for( i=0;  i<uiLen; i++)
		{
			if(i%4==0)
				log_info("\n\r");

			log_info("0x%x ",ucval[i]);
		}
	}
	/* No more parameters were found.  Make sure the write buffer does
	   not contain a valid string. */
        pcWriteBuffer[ 0 ] = 0x00;
	return pdFALSE;
}

#ifdef GEUL_LA1224
static portBASE_TYPE prvVidGetVddCommand(char *pcWriteBuffer, __attribute__((unused))size_t xWriteBufferLen, __attribute__((unused))const char *pcCommandString )
{
	int32_t iVdd = iGetCurrentVdd();

	if (iVdd < 0)
		PRINTF("VID: Failed to read current VDD\n\r");
	else if (iVdd != 0)
		PRINTF("VID: Current VDD value read is : %d mV\r\n", iVdd);

        /* No more parameters were found.  Make sure the write buffer does
           not contain a valid string. */
        pcWriteBuffer[ 0 ] = 0x00;

	return pdFALSE;
}
#endif

static portBASE_TYPE prvMDCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	const char *pcAddr;
	static UBaseType_t uxParameterNumber = 1U;
	BaseType_t xParameterStringLength;
	volatile uint32_t xVal[4], xCount, xAddrHex;
	volatile uint32_t *xAddrList;

	/* Remove compile time warnings about unused parameters, and check the
	   write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	   write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* Obtain the first parameter string i.e. Address to be read */
	pcAddr = FreeRTOS_CLIGetParameter
		(
		 pcCommandString,		/* The command string itself. */
		 uxParameterNumber,		/* Return the first parameter. */
		 &xParameterStringLength	/* Store the parameter string length. */
		);

	if( pcAddr != NULL )
	{
		/* Convert string to Hex Address */
		xAddrHex = strtoul( pcAddr, ( char ** )NULL, 16 );
		xAddrList = ( volatile uint32_t * )xAddrHex;
		for ( xCount = 0; xCount < 4; xCount++ ) {
			xVal[ xCount ] = ( *( volatile uint32_t * )xAddrList );
			xAddrList += 1;
		}
		PRINTF( "%p : %08x %08x %08x %08x\r\n", xAddrHex, xVal[0], xVal[1], xVal[2], xVal[3] );
	}

	/* Even if No parameters were found.  Make sure the write buffer does
	   not contain a valid string. */
	pcWriteBuffer[ 0 ] = 0x00;

	return pdFALSE;
}

static portBASE_TYPE prvMWCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	BaseType_t xParameterStringLength;
	static UBaseType_t uxParameterNumber = 1U;
	const char *pcAddr, *endPt;
	volatile uint32_t xAddrHex, xValHex;

	/* Remove compile time warnings about unused parameters, and check the
	   write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	   write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* Obtain first parameter string which is address to be written. */
	pcAddr = FreeRTOS_CLIGetParameter
		(
		 pcCommandString,		/* The command string itself. */
		 uxParameterNumber,		/* Return the next parameter. */
		 &xParameterStringLength	/* Store the parameter string length. */
		);

	/* Convert string to Hex Address and Value */
	xAddrHex = strtoul( pcAddr, (char **)&endPt, 16 );
	xValHex = strtoul( endPt, (char **)NULL, 16 );

	/* Assign the value to the Address */
	( *( volatile unsigned int * )( xAddrHex ) = ( xValHex ));
	PRINTF( "Value written at %p is %08x \r\n", xAddrHex, ( *( volatile uint32_t * )xAddrHex ));

	/* No more parameters were found.  Make sure the write buffer does
	   not contain a valid string. */
	pcWriteBuffer[ 0 ] = 0x00;

	return pdFALSE;
}

#ifdef CONFIG_TTI
static portBASE_TYPE prvTbgenttiCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	uint8_t uiTbgenNum = 0;
	uint32_t uiInterval = 0;
	BaseType_t xParameterIntvStringLength = 0, xParameterTbgenNumStringLength = 0;
	char *pcInterval = NULL, *pcTbgenNum = NULL;
	static UBaseType_t uxTbgenNumParam = 1U, uxIntervalParam = 2U;

	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	pcTbgenNum = ( char * ) FreeRTOS_CLIGetParameter(
			pcCommandString,		/* The command string itself. */
			uxTbgenNumParam,		/* Return the next parameter. */
			&xParameterTbgenNumStringLength	/* Store the parameter string length. */
			);

	pcInterval = ( char * ) FreeRTOS_CLIGetParameter(
			pcCommandString,		/* The command string itself. */
			uxIntervalParam,		/* Return the next parameter. */
			&xParameterIntvStringLength	/* Store the parameter string length. */
			);

	if (pcTbgenNum != NULL) {
		pcTbgenNum[ xParameterTbgenNumStringLength ] = 0x00;
		uiTbgenNum = strtoul( pcTbgenNum, (char **)NULL, BASE_DEC );
		if (uiTbgenNum == 0 && uiTbgenNum > 2) {
			PRINTF("Invalid tbgen num:%d\n\r", uiTbgenNum);
			return pdFALSE;
		}
	}

	if (pcInterval != NULL) {
		pcInterval[ xParameterIntvStringLength ] = 0x00;
		uiInterval = strtoul( pcInterval, (char **)NULL, BASE_DEC );
		if(uiInterval == TBGEN_TTI_INTERVAL_125 ||
		 uiInterval == TBGEN_TTI_INTERVAL_250 ||
		 uiInterval == TBGEN_TTI_INTERVAL_500 ||
		 uiInterval == TBGEN_TTI_INTERVAL_1000) {
			switch (uiInterval){
				case TBGEN_TTI_INTERVAL_125:
					vConfigurable_host_tbgentti_interval_test(TBGEN_TTI_INTERVAL_125_DIV, uiTbgenNum);
					break;
				case TBGEN_TTI_INTERVAL_250:
					vConfigurable_host_tbgentti_interval_test(TBGEN_TTI_INTERVAL_250_DIV, uiTbgenNum);
					break;
				case TBGEN_TTI_INTERVAL_500:
					vConfigurable_host_tbgentti_interval_test(TBGEN_TTI_INTERVAL_500_DIV, uiTbgenNum);
					break;
				case TBGEN_TTI_INTERVAL_1000:
					vConfigurable_host_tbgentti_interval_test(TBGEN_TTI_INTERVAL_1000_DIV, uiTbgenNum);
					break;
			}
		} else {
			PRINTF("invalid interval value: %d\n\r", uiInterval);
		}
	}
	pcWriteBuffer[ 0 ] = 0x00;
	return pdFALSE;
}
#endif
#ifdef TESTFRAMEWORK_ENABLE
static portBASE_TYPE prvTestCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	uint32_t uiIterations = 0, uiCore = 0xF, uiCoreIndex = 0;
	BaseType_t xParameter1StringLength = 0;
	BaseType_t xParameter2StringLength  = 0;
	BaseType_t xParameter3StringLength  = 0;
	char * pcFunctionName = NULL, *pcCore = NULL, * pcIterations = NULL;
	static UBaseType_t uxFunctionNameParam = 1U;
	static UBaseType_t uxCoreParam = 2U;
	static UBaseType_t uxIterationParam = 3U;

	extern TfIpiData_t xTfXchangeMem;
	TfIpiData_t *xTfXchangeData = &xTfXchangeMem;

	/* Remove compile time warnings about unused parameters, and check the
	   write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	   write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* Obtain Function Name to be executed ( Param 1) */
	pcFunctionName = ( char * ) FreeRTOS_CLIGetParameter
		(
		 pcCommandString,		/* The command string itself. */
		 uxFunctionNameParam,		/* Return the next parameter. */
		 &xParameter1StringLength	/* Store the parameter string length. */
		);

	/* Obtain core where function to be executed ( Param 2) */
	pcCore = ( char * ) FreeRTOS_CLIGetParameter
		(
		 pcCommandString,		/* The command string itself. */
		 uxCoreParam,				/* Return the next parameter. */
		 &xParameter2StringLength	/* Store the parameter string length. */
		);

	/* Obtain Number of iterations to be executed ( Param 3) */
	pcIterations = ( char * ) FreeRTOS_CLIGetParameter
		(
		 pcCommandString,		/* The command string itself. */
		 uxIterationParam,				/* Return the next parameter. */
		 &xParameter3StringLength	/* Store the parameter string length. */
		);

	if(xParameter1StringLength >= FUNCTION_NAME_LEN)
	{
		log_err("\n\r Input testcase string length is more than %d character \n\r", FUNCTION_NAME_LEN -1 );
		return pdFALSE;
	}

	/* Terminate pcFunctionName */
	pcFunctionName[ xParameter1StringLength ] = 0x00;

	/* Copy function name to IPI structure */
	strncpy( xTfXchangeData->pcFunctionName, pcFunctionName, FUNCTION_NAME_LEN  - 1);

	if( pcIterations != NULL )
	{
		/* Teriminate pcIterations */
		pcIterations[ xParameter3StringLength ] = 0x00;
		uiIterations = strtoul( pcIterations, (char **)NULL, BASE_DEC );

		/* Limit iterations to ONE, for following command parameters */
		if( uiIterations == 0 || bIsRestrictedCommand( pcFunctionName ) )
		{
			/* Minimum iteration(s) of test case is ONE */
			uiIterations = 1;
		}
	}

	/* Send number of iterations over IPI packet */
	xTfXchangeData->uiTestIteration = uiIterations;

	if( pcCore != NULL )
	{
		/* Teriminate pcCore */
		pcCore[ xParameter2StringLength ] = 0x00;
		uiCore = strtoul( pcCore, (char **)NULL, BASE_HEXA );
		if( uiCore == 0 )
		{
			/* Execute atleast on same core
				if no valid core is specified */
			uiCore = 1;
		}
	}
	if ( (strstr( pcFunctionName, "i2c")!=NULL) && (uiCore !=1))
	{
		log_err("\n\r All i2c tests are valid for core 0 only\n\r");
		return pdFALSE;
	}

	if ( (strstr( pcFunctionName, "tmu")==pcFunctionName) && (uiCore !=8))
	{
		log_err("\n\r Only TMU test is valid for core 3 only\n\r");
		return pdFALSE;
	}
	
	if ( (strstr( pcFunctionName, "tbgen_csg")!=NULL) && (uiCore !=1))
	{
		log_err("\n\r tbgen_csg tests are valid for core 0 only\n\r");
		return pdFALSE;
	}

	if ( (strstr( pcFunctionName, "ipilatency")!=NULL) && (uiCore != (uint32_t)((1 << get_soc_numcores()) - 1)))
	{
		log_err("\n\r ipilatency test should run on all cores\n\r");
		return pdFALSE;
	}

	if ( (strstr( pcFunctionName, "qdma")!=NULL) && (((uiCore & 0x14) == 0x14) ||
					((uiCore & 0x28) ==0x28)))
	{
		log_err("\n\r Incorrect qdma core mask, use either core2 or core4 and core3 or core5\n\r");
		return pdFALSE;
	}

	if ( (strstr( pcFunctionName, "qdma_sg")!=NULL) && (((uiCore & 0x14) == 0x14) ||
					((uiCore & 0x28) ==0x28)))
	{
		log_err("\n\r Incorrect qdma core mask, use either core2 or core4 and core3 or core5\n\r");
		return pdFALSE;
	}

	/* iterate thru all cores and check if ipi needs to be serviced on that core */
	for( uiCoreIndex = 0; uiCoreIndex < MAX_E200_CORES; uiCoreIndex++ )
	{
		if( uiCore & ( ( ( uint32_t ) 0x1 ) << ( MAX_E200_CORES - uiCoreIndex - 1 ) ) )
		{
			#if defined(TBGEN_ENABLED) && GEUL_IPI_LATENCY_TEST
			vGetCurrentTimeMulticore();
			#endif
			if( pdFALSE == vIPISendData( ( MAX_E200_CORES - uiCoreIndex - 1 ), IPI_EVT_TESTFRAMEWORK, (void *)xTfXchangeData ) )
			{
				log_info( "\n\r[Test Framework] IPISendData fail : Core - %d", uiCoreIndex );
			}
		}
	}

	/* No more parameters were found.  Make sure the write buffer does
	   not contain a valid string. */
	pcWriteBuffer[ 0 ] = 0x00;

	return pdFALSE;
}
#endif	/* TESTFRAMEWORK_ENABLE */

#ifdef TDD_DEMOAPP_ENABLE
static portBASE_TYPE prvVspaCfgBufferCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	BaseType_t xParamBufNoStringLength = 0;
	BaseType_t xParamBufAddrStringLength = 0;
	BaseType_t xParamBufSizeStringLength = 0;
	BaseType_t xParamDcsStringLength = 0;
	BaseType_t xParamAntennaIdStringLength = 0;
	BaseType_t xParamTxRxModeStringLength = 0;

	char * pcParamBufNo = NULL;
	char * pcParamBufAddr = NULL;
	char * pcParamBufSize = NULL;
	char * pcParamDcs = NULL;
	char * pcParamAntennaId = NULL;
	char * pcParamTxRxMode = NULL;

	/* vspa_cfg_buff <buf-no> <buf-addr> <buf-size> <dcs> <antenna-id> <tx/rx> */

	uint32_t xBufNo = 0;
	uint32_t xBufAddr = 0;
	uint32_t xBufSize = 0;
	uint32_t xDcs = 0;
	uint32_t xAntennaId = 0;
	uint32_t xTxRxMode = 0; /* 0=TX, 1=RX */

	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	pcParamBufNo = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 1U, &xParamBufNoStringLength );
	pcParamBufAddr = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 2U, &xParamBufAddrStringLength );
	pcParamBufSize = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 3U, &xParamBufSizeStringLength );
	pcParamDcs = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 4U, &xParamDcsStringLength );
	pcParamAntennaId = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 5U, &xParamAntennaIdStringLength );
	pcParamTxRxMode = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 6U, &xParamTxRxModeStringLength );

	xBufNo = strtoul( pcParamBufNo , (char **)NULL, BASE_DEC );
	xBufAddr = strtoul( pcParamBufAddr , (char **)NULL, BASE_HEXA );
	xBufSize = strtoul( pcParamBufSize , (char **)NULL, BASE_HEXA );
	xAntennaId = strtoul( pcParamAntennaId , (char **)NULL, BASE_DEC );
	TDD_STRING_PARAM_TO_VALUE( pcParamDcs, xDcs, uint32_t );
	TDD_STRING_PARAM_TO_VALUE( pcParamTxRxMode, xTxRxMode, uint32_t );

#ifdef TDD_DEMOAPP_DEBUG
	PRINTF( "trx = [%s], dcs = [%s]\r\n", pcParamTxRxMode, pcParamDcs );
#endif

	vVspaAddNewBuffer( xBufNo, xBufAddr, xBufSize, xDcs, xAntennaId, xTxRxMode );

	pcWriteBuffer [ 0 ] = 0;

	return pdFALSE;
}

static portBASE_TYPE prvVspaCfgPatternCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	BaseType_t xParamPattNoStringLength = 0;
	BaseType_t xParamBufNoStringLength = 0;
	BaseType_t xParamIsLastStringLength = 0;
	BaseType_t xParamTotalSymsStringLength = 0;
	BaseType_t xParamActiveSymsStringLength = 0;
	BaseType_t xParamDcsStringLength = 0;
	BaseType_t xParamAntennaIdStringLength = 0;
	BaseType_t xParamTxRxModeStringLength = 0;

	char * pcParamPatternNo = NULL;
	char * pcParamBufNo = NULL;
	char * pcParamIsLast = NULL;
	char * pcParamTotalSyms = NULL;
	char * pcParamActiveSyms = NULL;
	char * pcParamDcs = NULL;
	char * pcParamAntennaId = NULL;
	char * pcParamTxRxMode = NULL;

	/* vspa_cfg_patt <pattern-no> <buf-no> <is-last> <total-syms> <active-syms> <dcs> <antenna-id> <tx/rx> */

	uint32_t xPatternNo = 0;
	uint32_t xBufNo = 0;
	uint32_t xIsLast = 0;
	uint32_t xTotalSyms = 0;
	uint32_t xActiveSyms = 0;
	uint32_t xDcs = 0;
	uint32_t xAntennaId = 0;
	uint32_t xTxRxMode = 0; /* 0=TX, 1=RX */

	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	pcParamPatternNo = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 1U, &xParamPattNoStringLength );
	pcParamBufNo = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 2U, &xParamBufNoStringLength );
	pcParamIsLast = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 3U, &xParamIsLastStringLength );
	pcParamTotalSyms = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 4U, &xParamTotalSymsStringLength );
	pcParamActiveSyms = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 5U, &xParamActiveSymsStringLength );
	pcParamDcs = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 6U, &xParamDcsStringLength );
	pcParamAntennaId = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 7U, &xParamAntennaIdStringLength );
	pcParamTxRxMode = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 8U, &xParamTxRxModeStringLength );

	xPatternNo = strtoul( pcParamPatternNo , (char **)NULL, BASE_DEC );
	xBufNo = strtoul( pcParamBufNo , (char **)NULL, BASE_DEC );
	xTotalSyms = strtoul( pcParamTotalSyms , (char **)NULL, BASE_DEC );
	xActiveSyms = strtoul( pcParamActiveSyms , (char **)NULL, BASE_DEC );
	xAntennaId = strtoul( pcParamAntennaId , (char **)NULL, BASE_DEC );

	TDD_STRING_PARAM_TO_VALUE ( pcParamDcs, xDcs, uint32_t );
	TDD_STRING_PARAM_TO_VALUE ( pcParamIsLast, xIsLast, uint32_t );
	TDD_STRING_PARAM_TO_VALUE ( pcParamTxRxMode, xTxRxMode, uint32_t );

	vVspaAddNewPattern( xPatternNo, xBufNo, xIsLast, xTotalSyms, xActiveSyms, xDcs, xAntennaId, xTxRxMode );

	pcWriteBuffer [ 0 ] = 0;

	return pdFALSE;
}

static portBASE_TYPE prvTddStartCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	BaseType_t xParamDlSlotStringLength = 0;
	BaseType_t xParamUlSlotStringLength = 0;
	BaseType_t xParamDlSymsStringLength = 0;
	BaseType_t xParamUlSymsStringLength = 0;
	BaseType_t xParamDcsStringLength = 0;
	BaseType_t xParamAntennaIdStringLength = 0;

	char * pcParamDlSlot = NULL;
	char * pcParamUlSlot = NULL;
	char * pcParamDlSyms = NULL;
	char * pcParamUlSyms = NULL;
	char * pcParamDcs = NULL;
	char * pcParamAntennaId = NULL;

	uint32_t xDlSlotNum = 0;
	uint32_t xDlSymsNum = 0;
	uint32_t xUlSlotNum = 0;
	uint32_t xUlSymsNum = 0;
	uint32_t xDcs = 0;
	uint32_t xAntennaId = 0;

	/* start_tdd <dl-slots> <dl-syms> <ul-slots> <ul-syms> <dcs> <antenna-id> */

	/* Remove compile time warnings about unused parameters, and check the
	   write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	   write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	pcParamDlSlot = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 1U, &xParamDlSlotStringLength );
	pcParamDlSyms = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 2U, &xParamDlSymsStringLength );
	pcParamUlSlot = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 3U, &xParamUlSlotStringLength );
	pcParamUlSyms = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 4U, &xParamUlSymsStringLength );
	pcParamDcs = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 5U, &xParamDcsStringLength );
	pcParamAntennaId = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 6U, &xParamAntennaIdStringLength );

	xDlSlotNum = strtoul( pcParamDlSlot , (char **)NULL, BASE_DEC );
	xDlSymsNum = strtoul( pcParamDlSyms , (char **)NULL, BASE_DEC );
	xUlSlotNum = strtoul( pcParamUlSlot , (char **)NULL, BASE_DEC );
	xUlSymsNum = strtoul( pcParamUlSyms , (char **)NULL, BASE_DEC );
	xAntennaId = strtoul( pcParamAntennaId, (char **)NULL, BASE_DEC );
	TDD_STRING_PARAM_TO_VALUE( pcParamDcs, xDcs, uint32_t );

#ifdef TDD_DEMOAPP_DEBUG
	PRINTF("dcs=[%s], xDcs=%d\r\n", pcParamDcs, xDcs);
#endif

	vTbgenTddStart( xDlSlotNum, xDlSymsNum, xUlSlotNum, xUlSymsNum, xDcs, xAntennaId );

	pcWriteBuffer [ 0 ] = 0;

	return pdFALSE;
}

static portBASE_TYPE prvTddStopCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	/* Remove compile time warnings about unused parameters, and check the
	   write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	   write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	vTbgenTddStop();

	pcWriteBuffer [ 0 ] = 0;

	return pdFALSE;
}
#endif

#ifdef FSPI_CMD_ENABLE 
static portBASE_TYPE prvFspiUtil(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
	char *pcSrcAddr;
	char *pcDstAddr;
	char *pcLen;
	char cInCmd[16];
	uint32_t xSrcAddr;
	uint32_t xDstAddr;
	uint32_t xLen;
	BaseType_t xParamStringLength = 0;
	char *pcFunc = &cInCmd[0];

	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	sscanf(pcCommandString,"%s", pcFunc);

	PRINTF("Command:Len (%s:%s:%d)\r\n", pcCommandString, pcFunc, xParamStringLength);
	if (!strncmp(pcFunc, "sf_probe", strlen("sf_probe")))
	{
		iFspiInit();
	}
	else if (!strncmp(pcFunc, "sf_erase", strlen("sf_erase")))
	{
		pcSrcAddr = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 1U, &xParamStringLength );
		xSrcAddr = strtoul( pcSrcAddr , (char **)NULL, BASE_HEXA);
		pcLen = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 2U, &xParamStringLength );
		xLen = strtoul( pcLen , (char **)NULL, BASE_HEXA);

		if(!iFspiSecErase(xSrcAddr, xLen))
			PRINTF("Erase Addr=[%x], Len=%x  ..... Success\r\n", xSrcAddr, xLen);
		else
			PRINTF("Erase Addr=[%x], Len=%x  ..... Failed\r\n", xSrcAddr, xLen);
	}
	else if (!strncmp(pcFunc, "sf_rd", strlen("sf_rd")))
	{
		pcSrcAddr = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 1U, &xParamStringLength );
		xSrcAddr = strtoul( pcSrcAddr , (char **)NULL, BASE_HEXA);
		pcDstAddr = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 2U, &xParamStringLength );
		xDstAddr = strtoul( pcDstAddr , (char **)NULL, BASE_HEXA);
		pcLen = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 3U, &xParamStringLength );
		xLen = strtoul( pcLen , (char **)NULL, BASE_HEXA);

		if (!iFspiRead(xSrcAddr, (u32 *)xDstAddr, xLen))
			PRINTF("Read Addr from:to=[%x:%x], Len=%x  ..... Success\r\n", xSrcAddr, xDstAddr, xLen);
		else
			PRINTF("Read Addr from:to=[%x:%x], Len=%x  ..... Failed\r\n", xSrcAddr, xDstAddr, xLen);
	}
	else if (!strncmp(pcFunc, "sf_wr", strlen("sf_wr")))
	{
		pcSrcAddr = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 1U, &xParamStringLength );
		xSrcAddr = strtoul( pcSrcAddr , (char **)NULL, BASE_HEXA);
		pcDstAddr = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 2U, &xParamStringLength );
		xDstAddr = strtoul( pcDstAddr , (char **)NULL, BASE_HEXA);
		pcLen = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, 3U, &xParamStringLength );
		xLen = strtoul( pcLen , (char **)NULL, BASE_HEXA);

		if (!iFspiIpWrite(xDstAddr, (u32 *)xSrcAddr, xLen))
			PRINTF("Write Addr from:to=[%x:%x], Len=%x  ..... Success\r\n", xSrcAddr, xDstAddr, xLen);
		else
			PRINTF("Write Addr from:to=[%x:%x], Len=%x  ..... Failed\r\n", xSrcAddr, xDstAddr, xLen);
	}
	else
	{
		PRINTF("Usage:\r\n");
		PRINTF("sf probe                             - Init FSPI\r\n");
		PRINTF("sf erase offset len                  - Erase FSPI\r\n");
		PRINTF("sf read offset toAddr len            - Read from FSPI to given address\r\n");
		PRINTF("sf write FromAddr offset len         - Write from given Adress to FSPI\r\n");
	}
	
	pcWriteBuffer[0] = 0;

	return pdFALSE;
}
#endif

/*-----------------------------------------------------------*/
