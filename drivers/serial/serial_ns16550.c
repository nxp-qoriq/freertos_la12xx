// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2017-2021 NXP
 */

#include <common.h>
#include <platform_def.h>
#include <sync.h>
#include "serial_ns16550.h"

#define SERIAL_LCRVAL	SERIAL_LCR_8N1
#define SERIAL_FCRVAL	(SERIAL_FCR_FIFO_EN | \
			  SERIAL_FCR_RXSR |    \
			  SERIAL_FCR_TXSR)
#define NS16550_IER	0x0

void vSerialInit(NS16550_t base, uint32_t ulBaudRate, uint32_t ulSrcClockHz)
{

	uint32_t lBaudDivisor;

    /* See Section 10.4.2 of LA1224 RM
     * Baud rate = ((1/16) x
     * (ip_clk platform clock frequency/4 frequency ÷ divisor value))
     * Therefore, the output frequency of the baud-rate
     * generator is 16 times the baud rate.
     * */
	lBaudDivisor = ulSrcClockHz / (16 * 4 * ulBaudRate);

	OUT_8(&base->ier, NS16550_IER);
	OUT_8(&base->fcr, SERIAL_FCRVAL);
	OUT_8(&base->lcr, SERIAL_LCR_BKSE | SERIAL_LCRVAL);

	sync_dmb();
	OUT_8(&base->dll, lBaudDivisor & 0xff);
	OUT_8(&base->dlm, (lBaudDivisor >> 8) & 0xff);

	sync_dmb();
	OUT_8(&base->lcr, SERIAL_LCRVAL);
}

void vSerialWriteBlocking(NS16550_t xBase, const uint8_t *pucData,
			  size_t xLength)
{
	while (xLength--) {
		while (!(IN_8(&xBase->lsr) & SERIAL_LSR_THRE))
			;
	OUT_8(&xBase->thr, *pucData++);
	}

}

void vSerialReadBlocking(NS16550_t xBase, uint8_t *pucData, size_t xLength)
{
#if NXP_ERRATUM_A004737
	if ((IN_8(&xBase->lsr) & SERIAL_LSR_BI)) {
		IN_8(&xBase->rbr);
		return;
	}
#endif
	while (xLength) {
		if (!(IN_8(&xBase->lsr) & SERIAL_LSR_DR))
			;
		else {
			*pucData++ = IN_8(&xBase->rbr);
			xLength--;
		}
	}
}
