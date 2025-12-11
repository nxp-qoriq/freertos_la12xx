// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "agave_init.h"
#include "agave_synth.h"

/* Default freq is 3510 MHz */
uint32_t AgvSynthInitReg_u[] = {
    0x460000, 0x450000, 0x440089, 0x400077,
    0x3E0000, 0x3D0001, 0x3B0000, 0x3003FD,
    0x2F00CF, 0x2E0FA3, 0x2DF78A, 0x2C0001,
    0x2B0000, 0x2A0000, 0x294240, 0x28000F,
    0x278204, 0x260072, 0x254000, 0x240011,
    0x23021B, 0x22C3EA, 0x212A0A, 0x20210A,
    0x1F0001, 0x1E0034, 0x1D0084, 0x1C2924,
    0x190000, 0x180509, 0x178842, 0x162300,
    0x14012C, 0x130965, 0x0E018C, 0x0D4000,
    0x0C7002, 0x0B0018, 0x0A10D8, 0x090302,
    0x081084, 0x0728B2, 0x041943, 0x020500,
    0x010809, 0x002218
};

/* LO = 3510MHz */
uint32_t AgvLo3510MhzReg_u[] = {
    0x260038, 0x28000B, 0x29EBC2, 0x2C0006,
    0x2DBA93
};

/* LO = 3540MHz */
uint32_t AgvLo3540MhzReg_u[] = {
    0x260038, 0x280000, 0x293EB3, 0x2C0000,
    0x2D33E0
};

/* LO = 3480MHz */
uint32_t AgvLo3480MhzReg_u[] = {
    0x260038, 0x280000, 0x296DC3, 0x2C0000,
    0x2D252F
};

int32_t prvAgvSynthWriteReg( AgvDevice_t *pxAgvDev, uint8_t addr,
                             uint16_t data )
{
    int32_t iRet;
    uint8_t ucData[ 4 ] = { 0 };

    log_dbg("%s: WriteReg[%x : %x]\r\n", __func__, addr, data);

    /* Write synthesizer register */
    ucData[ 0 ] = addr;
    ucData[ 1 ] = ( uint8_t )(( data & 0xFF00 ) >> 8 );
    ucData[ 2 ] = ( uint8_t )( data & 0x00FF);
    iRet = lDspiPush( pxAgvDev->xDspiHandle, DSPI_CS0, DSPI_DEV_WRITE,
		      ucData, 4 );
    if( 0 > iRet )
    {
	log_err( "%s: lDspiPush failed, error[%d]\r\n", __func__,
		 iRet );
    }

    return iRet;
}

int32_t prvAgvSynthReadReg( AgvDevice_t *pxAgvDev, uint8_t addr,
                            uint16_t *data )
{
    int32_t iRet;
    uint8_t ucData[ 4 ] = { 0 };

    /* Push address to be read, Set MSB bit
     * which indicate read operation */
    ucData[ 0 ] = addr | AGV_SYNTH_READ_OPR;
    iRet = lDspiPush( pxAgvDev->xDspiHandle, DSPI_CS0, DSPI_DEV_READ,
		      ucData, 4 );
    if( 0 > iRet )
    {
	log_err( "%s: lDspiPush failed, error[%d]\r\n", __func__,
		 iRet );
	return iRet;
    }

    iRet = lDspiPop( pxAgvDev->xDspiHandle, ucData, 4 );
    if( 0 > iRet )
    {
	log_err( "%s: lDspiPop failed, error[%d]\r\n", __func__,
		 iRet );
	return iRet;
    }

    /* index 0 should be ignored, this is outcome
     * of above push operation */
    *data = (( int16_t )( ucData[ 1 ] ) << 8) | ucData[ 2 ];

    return iRet;
}

void AgvAdjustPllFreq(AgvDevice_t *pxAgvDev, int32_t freq_khz )
{
	int32_t ch_divider ;
	int16_t N_divider ;
	int32_t	F_Num;
	double vco_freq ;
	int32_t prescalar;
	uint16_t pre_r;
	uint16_t temp_F_Num;
	uint16_t temp_N_divider = 0;
	uint16_t temp_R35 = 0;
	uint16_t dbg_reg = 0;

	uint16_t buf_ch_div_reg_val;
	uint16_t mux_out_a_reg_val;
	uint16_t buf_vco_out_reg_val;

	prvAgvSynthReadReg(pxAgvDev, CH_DIV_SEG_REG, &temp_R35);
	temp_F_Num = 0;
	prescalar = 2;
	pre_r = 2;
	F_Num = 0;
	N_divider = 0;

	buf_ch_div_reg_val = 0x0421;
	mux_out_a_reg_val = 0x00CF;
	buf_vco_out_reg_val = 0x0601;

	if(freq_khz >= 700000 && freq_khz < 887500)
	{
		ch_divider = 6 ;
		vco_freq = freq_khz * ch_divider;
		N_divider = ( pre_r * vco_freq ) / ( prescalar * RFIC_OSC_FREQ );
		F_Num = (( ( pre_r * vco_freq ) / ( prescalar * RFIC_OSC_FREQ )) - N_divider) * 1000000 + 1 ;

		temp_R35 = temp_R35 | (1 << CHDIV_SEG1);
		temp_R35 = temp_R35 | (1 << CHDIV_SEG2_EN);
		temp_R35 = temp_R35 | (1 << CHDIV_SEG2);

	}
	else if(freq_khz >= 887500 && freq_khz < 1183334)
	{
		ch_divider = 4 ;
		vco_freq = freq_khz * ch_divider;
		N_divider = ( pre_r * vco_freq ) / ( prescalar * RFIC_OSC_FREQ );
		F_Num = ((( pre_r * vco_freq ) / ( prescalar * RFIC_OSC_FREQ )) - N_divider) *1000000 + 1;

		temp_R35 = temp_R35 & ~(1 << CHDIV_SEG1);
		temp_R35 = temp_R35 | (1 << CHDIV_SEG2_EN);
		temp_R35 = temp_R35 | (1 << CHDIV_SEG2);


	}
	else if(freq_khz >= 1183334 && freq_khz < 1775000)
	{
		ch_divider = 3 ;
		vco_freq = freq_khz * ch_divider;
		N_divider = (pre_r * vco_freq ) / ( prescalar * RFIC_OSC_FREQ );
		F_Num = ((( pre_r * vco_freq ) / ( prescalar * RFIC_OSC_FREQ )) - N_divider) *1000000 + 1;

		temp_R35 = temp_R35 | (1 <<  CHDIV_SEG1);
		temp_R35 = temp_R35 & ~(1 <<  CHDIV_SEG2_EN);
		temp_R35 = temp_R35 & ~(1 <<  CHDIV_SEG2);

		buf_ch_div_reg_val = 0x0411;

	}
	else if(freq_khz >= 1775000 && freq_khz < 3550000)
	{
		ch_divider = 2 ;
		vco_freq = freq_khz * ch_divider;
		N_divider = ( pre_r * vco_freq ) / (prescalar * 122880);
		F_Num = ((( pre_r * vco_freq ) / (prescalar * 122880)) - N_divider) *1000000 + 1;

		temp_R35 = temp_R35 & ~(1 << CHDIV_SEG1);
		temp_R35 = temp_R35 & ~(1 << CHDIV_SEG2_EN);
		temp_R35 = temp_R35 & ~(1 << CHDIV_SEG2);

		buf_ch_div_reg_val = 0x0411;
	}
	else if(freq_khz >= 3550000 && freq_khz <= 4500000)
	{
		vco_freq = freq_khz ;
		N_divider = ( pre_r * vco_freq ) / ( prescalar * RFIC_OSC_FREQ );
		F_Num = ((( pre_r * vco_freq ) / ( prescalar * RFIC_OSC_FREQ )) - N_divider) *1000000 + 1;

		temp_R35 = temp_R35 & ~(1 << CHDIV_SEG1);
		temp_R35 = temp_R35 & ~(1 << CHDIV_SEG2_EN);
		temp_R35 = temp_R35 & ~(1 << CHDIV_SEG2);

		buf_ch_div_reg_val = 0x0011;
		mux_out_a_reg_val = 0x08CF;
		buf_vco_out_reg_val = 0x0401;
	}

	prvAgvSynthWriteReg(pxAgvDev, BUF_CH_DIV_REG, buf_ch_div_reg_val);
	prvAgvSynthWriteReg(pxAgvDev, MUX_OUT_A_REG, mux_out_a_reg_val);
	prvAgvSynthWriteReg(pxAgvDev, BUF_VCO_OUT_REG, buf_vco_out_reg_val);
	prvAgvSynthWriteReg(pxAgvDev, CH_DIV_SEG_REG, temp_R35);

	/* INTEGER PART OF N DIVIDER */
	prvAgvSynthReadReg(pxAgvDev, INT_N_DIV_REG, &temp_N_divider);
        temp_N_divider = temp_N_divider & MASK_INT_N_DIV;
        temp_N_divider = temp_N_divider | (N_divider << 1);
	prvAgvSynthWriteReg(pxAgvDev, INT_N_DIV_REG, temp_N_divider);

	/* NUMERATOR OF FRACTION */
	temp_F_Num |= F_Num;
	prvAgvSynthWriteReg(pxAgvDev, NUM_LSB_N_DIV_FRAC_REG, temp_F_Num);
	temp_F_Num = 0;
	F_Num >>= 16;
	temp_F_Num |= F_Num;
	prvAgvSynthWriteReg(pxAgvDev, NUM_MSB_N_DIV_FRAC_REG, temp_F_Num);

	/* CALIBRATION */
	prvAgvSynthReadReg(pxAgvDev, 0x00, &dbg_reg);
	dbg_reg = dbg_reg | (1 << 3);
	prvAgvSynthWriteReg(pxAgvDev, 0x00,dbg_reg);

	/* DEBUG */
#ifdef DEBUG
	{
		int i;
		for( i = 0x40 ; i >= 0x00 ; i-- )
		{
			if( (i != 3) && (i != 5) && (i != 6) && (i != 15) && (i != 16)
				&& (i != 17) && (i != 18) && (i != 21) && (i != 26)
				&& (i != 27) && !(i >= 49 && i <=58 ) && (i != 60)
				&& (i != 63) )
			{
				prvAgvSynthReadReg(pxAgvDev, i, &dbg_reg);
				log_info(" %x : %x\r\n", i, dbg_reg);
			}
		}
	 }
#endif
}


int32_t AgvSynthInit( AgvDevice_t *pxAgvDev )
{
    int32_t iRet;
    uint16_t usData;
    uint8_t ucReg, i;

    /* soft reset */
    usData = AGV_SYNTH_RESET;
    iRet = prvAgvSynthWriteReg( pxAgvDev, AGV_SYNTH_REG0, usData );
    if( iRet < 0 )
    {
        log_err( "%s: synth write reg[%x] failed\r\n", __func__, AGV_SYNTH_REG0 );
	return -1;
    }

    /* program register */
    for( i = 0; i < ( sizeof( AgvSynthInitReg_u ) / sizeof( uint32_t )); i++ )
    {
	ucReg = (( AgvSynthInitReg_u[ i ] & AGV_SYNTH_ADDR_MASK ) >> 16 );
	usData = AgvSynthInitReg_u[ i ] & AGV_SYNTH_DATA_MASK;

	iRet = prvAgvSynthWriteReg( pxAgvDev, ucReg, usData );
	if( iRet < 0 )
	{
	    log_err( "%s: synth write reg[%x] failed\r\n", __func__, ucReg );
	    return -1;
	}
    }

#if 0
    /* frequency calibrate */
    iRet = prvAgvSynthReadReg( pxAgvDev, AGV_SYNTH_REG0, &usData );
    if( iRet < 0 )
    {
        log_err( "%s: synth read reg[%x] failed\r\n", __func__, AGV_SYNTH_REG0 );
	return -1;
    }
    usData |= AGV_SYNTH_FCAL;
    iRet = prvAgvSynthWriteReg( pxAgvDev, AGV_SYNTH_REG0, usData );
    if( iRet < 0 )
    {
        log_err( "%s: synth write reg[%x] failed\r\n", __func__, AGV_SYNTH_REG0 );
	return -1;
    }
#endif
    return 0;
}
