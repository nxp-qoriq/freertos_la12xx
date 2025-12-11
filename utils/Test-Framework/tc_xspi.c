// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022 NXP
 */

#include "tc_xspi.h"
#include "FreeRTOS.h"
#include "task.h"
#include <types.h>
#include "fspi_api.h"
#include "Time.h"

#define XSPI_RD_WRITE_LEN		4
#define XSPI_RD_WRITE_ADDR		0x100000
#define XSPI_ERASE_LEN			0x10000
#define XSPI_CMD_DELAY			200000
#define XSPI_DEMO_DATA			0xab

void vXspiReadWrite()
{
	static BaseType_t src_addr;
	static BaseType_t dst_addr;
	int iRet = 0, i;
	char *tempdstaddr;
	iFspiInit();
	iRet = iFspiSecErase(XSPI_RD_WRITE_ADDR, XSPI_ERASE_LEN);
	if(0 != iRet) {
		log_err("Not able to erase\r\n");
		return;
	}
	vUdelay(XSPI_CMD_DELAY);
	src_addr = (BaseType_t)pvGeulMalloc(XSPI_RD_WRITE_LEN);
	if (!src_addr) {
		log_err("xspi:failed to malloc src");
		return;
	}
	memset((char *)src_addr, XSPI_DEMO_DATA, XSPI_RD_WRITE_LEN);

	iRet = iFspiIpWrite( XSPI_RD_WRITE_ADDR, (u32*)src_addr, XSPI_RD_WRITE_LEN );
	if(0 != iRet) {
		log_err("Not able to write\r\n");
		return;
	}
	dst_addr = (BaseType_t)pvGeulMalloc(XSPI_RD_WRITE_LEN);
	if (!dst_addr) {
		log_err("xspi:failed to malloc dst");
		return;
	}
	memset((char *)dst_addr, 0x55, XSPI_RD_WRITE_LEN);

	iRet = iFspiRead(XSPI_RD_WRITE_ADDR, (u32*)dst_addr, XSPI_RD_WRITE_LEN);
	if(0 != iRet) {
		log_err("Not able to read\r\n");
		return;
	}

	tempdstaddr = (char *)dst_addr;
	for (i = 0; i < XSPI_RD_WRITE_LEN; i++) {
		if(*tempdstaddr != XSPI_DEMO_DATA) {
			log_err("XSPI Read/Write Test Failed");
			return;
		}
		tempdstaddr++;
	}
	log_info("XSPI Read/Write Test Case Passed\r\n");
	return;
}
