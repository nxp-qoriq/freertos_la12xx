// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2022 NXP
 */

#ifndef __RF_COMMON_H__
#define __RF_COMMON_H__

#if RF_DEBUG_HIFCTRL
    #define RF_LOGERR( fmt, ... )    log_err( "\r\n[ERR] (%s:%d) "fmt, __func__, __LINE__, __VA_ARGS__ )
    #define RF_LOGERRMSG( fmt )      log_err( "\r\n[ERR] (%s:%d) "fmt, __func__, __LINE__ )
#else
    #define RF_LOGERR( fmt, ... )    PRINTF( "\r\n[ERR] (%s:%d) "fmt, __func__, __LINE__, __VA_ARGS__ )
    #define RF_LOGERRMSG( fmt )      PRINTF( "\r\n[ERR] (%s:%d) "fmt, __func__, __LINE__ )
#endif

#if RF_DEBUG_HIFCTRL
    #define RF_LOGDBG( fmt, ... )    log_dbg( "\r\n[DBG] (%s:%d) "fmt, __func__, __LINE__, __VA_ARGS__ )
    #define RF_LOGDBGMSG( fmt )      log_dbg( "\r\n[DBG] (%s:%d) "fmt, __func__, __LINE__ )
#elif RF_DEBUG
    #define RF_LOGDBG( fmt, ... )    PRINTF( "\r\n[DBG] (%s:%d) "fmt, __func__, __LINE__, __VA_ARGS__ )
    #define RF_LOGDBGMSG( fmt )      PRINTF( "\r\n[DBG] (%s:%d) "fmt, __func__, __LINE__ )
#else
    #define RF_LOGDBG( fmt, ... )
    #define RF_LOGDBGMSG( fmt )
#endif


#endif
