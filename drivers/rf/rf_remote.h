// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef __RF_REMOTE_H__
#define __RF_REMOTE_H__

int32_t iRaiseRemoteRFEvent( RficHandle_t xHandle,
                             uint32_t ulEventId,
                             uint32_t ulDestCoreId );

#endif /* __RF_REMOTE_H__ */
