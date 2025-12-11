// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2022 NXP
 */

#ifndef __DEBUG_CONSOLE_H__
#define __DEBUG_CONSOLE_H__

#include <stdint.h>
#include <stdarg.h>
#include "gul_bsp_init.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define IO_MAXLINE  20

#define PRINT_64_HI(num)        (u32)(num >> 32) & 0xFFFFFFFF
#define PRINT_64_LO(num)        (u32)(num) & 0xFFFFFFFF

/*! @brief Configuration for toolchain's printf or NXP version printf */
#ifdef BOOT_DEBUG
#define DPRINTF          debug_printf
#else
#define DPRINTF(...)
#endif
#define PRINTF          debug_printf
#define PUTCHAR         debug_putchar

extern volatile struct debug_log_regs *pDbgLogRegs;

#if (GUL_LOG_LEVEL >= GUL_LOG_LEVEL_ERR)
#define log_err(...) do {						\
	if (in_le32(&pDbgLogRegs->log_level) >= GUL_LOG_LEVEL_ERR)		\
		PRINTF(__VA_ARGS__);					\
} while(0)
#else
#define log_err(...) do {						\
} while(0)
#endif

#if (GUL_LOG_LEVEL >= GUL_LOG_LEVEL_INFO)
#define log_info(...) do {						\
	if (in_le32(&pDbgLogRegs->log_level) >= GUL_LOG_LEVEL_INFO)		\
		PRINTF(__VA_ARGS__);					\
}while(0)
#else
#define log_info(...) do {						\
} while(0)
#endif

#if (GUL_LOG_LEVEL >= GUL_LOG_LEVEL_DBG)
#define log_dbg(...) do {						\
	if (in_le32(&pDbgLogRegs->log_level) >= GUL_LOG_LEVEL_DBG)		\
		PRINTF(__VA_ARGS__);					\
}while(0)
#else
#define log_dbg(...) do {						\
} while(0)
#endif

#if (GUL_LOG_LEVEL >= GUL_LOG_LEVEL_ISR)
#define log_isr(...) do {						\
	if (in_le32(&pDbgLogRegs->log_level) >= GUL_LOG_LEVEL_ISR)		\
		PRINTF(__VA_ARGS__);					\
}while(0)
#else
#define log_isr(...) do {						\
} while(0)
#endif

extern uint32_t ulMemLogIndex;

/*! @brief Error code for the debug console driver. */
typedef enum _debug_console_status {
    status_DEBUGCONSOLE_Success = 0U,
    status_DEBUGCONSOLE_InvalidDevice,
    status_DEBUGCONSOLE_AllocateMemoryFailed,
    status_DEBUGCONSOLE_Failed
} debug_console_status_t;

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Initializes the UART used for debug messages.
 *
 * Call this function to enable debug log messages to be output through the specified UART
 * base address and at the specified baud rate. Initialize the UART to the given baud
 * rate and 8N1. After this function returns, stdout gets connected to the
 * selected UART. The debug_printf() function also uses this UART.
 *
 * @param base Which UART instance is used to send debug messages.
 * @param clockRate The input clock of UART module.
 * @param baudRate The desired baud rate in bits per second.
 * @return Whether initialization is successful or not.
 */
debug_console_status_t xDebugConsoleInit(void* base,
                                       uint32_t clockRate,
                                       uint32_t baudRate);

/*!
 * @brief   Prints formatted output to the standard output stream.
 *
 * Call this function to print formatted output to the standard output stream.
 *
 * @param   fmt_s   Format control string.
 * @return  Returns the number of characters printed, or a negative value if an error occurs.
 */
int debug_printf(const char  *fmt_s, ...);

/*!
 * @brief   Prints formatted output to the standard output stream.
 *
 * Call this function to print formatted output to the standard output stream.
 *
 * @param   fmt_s   Format control string.
 * @param   arg   va_list of arguments.
 * @return  Returns the number of characters printed, or a negative value if an error occurs.
 */
int debug_vprintf(const char  *fmt_s, va_list arg);

/*!
 * @brief   Writes a character to stdout.
 *
 * Call this function to write a character to stdout.
 *
 * @param   ch  Character to be written.
 * @return  Returns the character written.
 */
int debug_putchar(int ch);

/*!
 * @brief   Reads a character to stdin.
 *
 * Call this function to write a character to stdout.
 *
 * @param   ch  Character to be written.
 * @return  Returns the character written.
 */
int debug_getchar(unsigned char *ch);

void vMemlogWrite(void *);
#endif /* __DEBUG_CONSOLE_H__ */
