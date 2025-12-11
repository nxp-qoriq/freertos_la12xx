// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2017-2023 NXP
 */

#ifndef __SOC__H
#define __SOC__H

#include <common.h>

extern uint32_t crt_soc_rev;
extern uint32_t crt_soc_numcores;
extern uint32_t crt_soc_svr;

enum soc_fuse {
        FUSE_LA1200 = 0,
        FUSE_LA1201,
        FUSE_LA1212,
        FUSE_LA1214,
        FUSE_LA1215,
        FUSE_LA1216,
        FUSE_LA1223,
        FUSE_LA1225,
        FUSE_LA1234,
        FUSE_LA1235,
        FUSE_LA1236,
        FUSE_LA1218,
        FUSE_LA1238,
        FUSE_LA1232,
        FUSE_RESV,
        FUSE_LA1224,
} soc_fuse;

typedef enum {
	HSDCS_NO,
	HSDCS_RXONLY,
	HSDCS_FULL,
} soc_type_t;

#define get_soc_revision()	(crt_soc_rev)
#define get_soc_numcores()	(crt_soc_numcores)
#define get_soc_version()	(crt_soc_svr)

void vInitSmem(void);
void vInitSmemText(void);
void vInitCoreSmem(void);
void vInitCoreSmemText(void);
void vSoCEnableUART(void);
uint32_t get_hsdcs_support(void);
uint32_t __get_soc_revision(void);
uint32_t __get_soc_numcores(void);
uint32_t get_processor_version(void);
uint32_t get_fusesr(void);

#endif
