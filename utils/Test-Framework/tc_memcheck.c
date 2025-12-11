// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "tc_memcheck.h"

#if GEUL_DEMO_MEMCHECK_TEST
#include "FreeRTOS.h"
#include "immap.h"
#include <task.h>

void vMemCheck(void)
{
  static char *mem;
  static int count=0;
  u32 core_id;
  core_id=ulMpicCurrentCore();
  char ch[]="abc";

  extern char heap_start_asm __asm__ ("__heap_start");
  extern char heap_end_asm   __asm__ ("__heap_end");

  extern long __DMEM_START;
  extern long __DMEM_END;

  void *heapStartAddr = (void *)&heap_start_asm;
  void *heapEndAddr   = (void *)&heap_end_asm;

  void *dmemStartAddr = (void *)&__DMEM_START;
  void *dmemEndAddr   = (void *)&__DMEM_END;

  if  ( ((void *)ch < dmemStartAddr) || (void *)ch > dmemEndAddr)
  {
    log_err("\n MEMCHECK(TASK_STACK TEST) FAIL Task Stack allocated address out of range 0x%x startAddr=0x%x, EndAddr =0x%x\r\n",ch, dmemStartAddr, dmemEndAddr);
    goto TEST_FAIL;
  }
  log_info("\r\n  Task Stack Variable= 0x%x startAddr=0x%x, EndAddr =0x%x ",ch, dmemStartAddr, dmemEndAddr);
  if(count==0)
  {
 	 mem=(char *)pvGeulMalloc(100);
	count++;
  }

  if ( (void *)mem < heapStartAddr || (void *)mem > heapEndAddr )
  {
    log_err("\r\n MEMCHECK(HEAP_TEST) FAIL LA12XX Malloc has allocated from out of boundary\r\n");
    log_err("\r\n Allocated Dynamic Mem=0x%x Heap Mem big=0x%x end=0x%x\r\n", mem, (unsigned long)heapStartAddr, (unsigned long)heapEndAddr);
    goto TEST_FAIL;
  }
  log_info("\r\n Allocated Dynamic Mem=0x%x Heap Mem big=0x%x end=0x%x\r\n", mem, (unsigned long)heapStartAddr, (unsigned long)heapEndAddr);

SET_TEST_STATUS(core_id, GEUL_DEMO_MEMCHECK_TEST_STATUS);
return;
TEST_FAIL:
RESET_TEST_STATUS(core_id, GEUL_DEMO_MEMCHECK_TEST_STATUS);
return;
}

#endif
