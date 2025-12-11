/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2022 NXP */

#ifndef _L1C_CLI_H
#define _L1C_CLI_H

/* helper macro for command parameter parsing */
#if !(defined(ARRAY_SIZE))
#define ARRAY_SIZE(arr)   (sizeof(arr) / sizeof((arr)[0]))
#endif /* !defined(ARRAY_SIZE) */

#define L1C_CLI_COMMAND_DESCRIPTION \
	"\r\nL1C Commands:" \
	"\r\n    l1c help" \
	"\r\nUse 'l1c help' command for more details about every 'l1c' command.\r\n" \

extern portBASE_TYPE prvL1CDemoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

#endif
