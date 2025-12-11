// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "types.h"
#include "common.h"
#include "immap.h"
#include "pmux.h"

int switchPMuxMode(enum pmux_num num, enum pmux_index index,
        enum pmux_mode mode)
{
    uint32 pmux_val, pmux_mask;
    struct ccsr_pmux *pmux = NULL;
    if (num < PMUX_5)
        pmux = (struct ccsr_pmux *)(PMUXCR_BASE_ADDR_BANK1);
    else {
        pmux = (struct ccsr_pmux *)(PMUXCR_BASE_ADDR_BANK2);
        num = num - PMUX_5;
    }

    /* Read the PMUX value */
    if ( num > PMUX_8 || index > PMUX8_RES_30_31 || mode > ALT_MODE8 )
        return false;

    /* Read the current PMUXCR register */
    pmux_val = in_le32(&(pmux->ulPMuxCR[num]));

    /* Write the PMUX Mode to respective PMUX_CR register
     * */

    /* UART are 3-bits so that's why special treatment */
    if ( PMUX2_UART1 == index || PMUX2_UART2 == index )
    {
        /* TODO : Special handling for UART PMUX */
    }
    else
    {
	pmux_mask = ~ ((FLAG_0b11) << ((index % 16) * 2));
	pmux_mask = pmux_val & pmux_mask;
        out_le32( &(pmux->ulPMuxCR[num]),
                (pmux_mask | ((mode & FLAG_0b11) << ((index % 16) * 2))) );
    }

    return true;
}

void vSwitchGpioToHS( void )
{
    struct ccsr_pmux *pmux = (struct ccsr_pmux *)(PMUXCR_BASE_ADDR_BANK2);
    enum pmux_num num = (PMUX_5 - PMUX_5);
    out_le32( &(pmux->ulPMuxCR[num]), PMUX_HS_MODE_ALL);
    out_le32( &(pmux->ulPMuxCR[num + 1]), PMUX_HS_MODE_ALL);

    return;
}

void vSwitchGpioToLS( void )
{
    struct ccsr_pmux *pmux = (struct ccsr_pmux *)(PMUXCR_BASE_ADDR_BANK1);
    enum pmux_num num = PMUX_3;
    out_le32( &(pmux->ulPMuxCR[num]), PMUX_LS_MODE_ALL);
    out_le32( &(pmux->ulPMuxCR[num + 1]), PMUX_LS_MODE_ALL);

    return;
}

void vSwitchHSToGpio( void )
{
    struct ccsr_pmux *pmux = (struct ccsr_pmux *)(PMUXCR_BASE_ADDR_BANK2);
    enum pmux_num num = (PMUX_5 - PMUX_5);
    out_le32( &(pmux->ulPMuxCR[num]), PMUX_HS_GPIO_MODE_ALL);
    out_le32( &(pmux->ulPMuxCR[num + 1]), PMUX_HS_GPIO_MODE_ALL);

    return;
}

void vSwitchLSToGpio( void )
{
    struct ccsr_pmux *pmux = (struct ccsr_pmux *)(PMUXCR_BASE_ADDR_BANK1);
    enum pmux_num num = PMUX_3;
    out_le32( &(pmux->ulPMuxCR[num]), PMUX_LS_GPIO_MODE_ALL);
    out_le32( &(pmux->ulPMuxCR[num + 1]), PMUX_LS_GPIO_MODE_ALL);

    return;
}
