// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2022 NXP
 */

#ifndef __YUC_RFIC_H__
#define __YUC_RFIC_H__

#include <io.h>
#include <sync.h>
#include <types.h>

#define LLCP1_OFFSET        0x1100000
#define LLCP2_OFFSET        0x1104000
#define LLCP1_RFIC_OFFSET	0x1200000
#define LLCP2_RFIC_OFFSET   0x1210000

#define YUC_LLCP1_ADDR           ( CCSR_BASE_ADDR + LLCP1_RFIC_OFFSET )
#define YUC_LLCP2_ADDR           ( CCSR_BASE_ADDR + LLCP2_RFIC_OFFSET )

#define LLCP_REG_R               0
#define LLCP_REG_W               1

#define llcpRFIC_READ            IN_16
#define llcpRFIC_WRITE           OUT_16

#ifndef YUCCA_LLCP_STUB
    #define YUC_RFIC_TF_RETRY    1000
#else
    #define YUC_RFIC_TF_RETRY    2
#endif

BaseType_t xYucRficCmdProc( u32 ulCmdData,
                            u32 ulRwWds,
                            u16 * uWords,
                            u8 ucNumWds );
void vYucLlcpRficRegRW( u16 * val,
                        u16 addr,
                        u8 rw );

#endif /* ifndef __YUC_RFIC_H__ */
