// SPDX-License-Identifier: BSD-3-Clause
/*
 *  Copyright 2019-2021 NXP
 */
/**
 * Description:  NXP FlexSpi Controller Driver.
 *
*/
#include <stdio.h>
#include <types.h>
#include <io.h>
#include "Time.h"
#include <debug_console.h>
#include "fspi.h"
#include "flash_info.h"
#include <fspi_api.h>
#include "mpu.h"

//#define DEBUG_FLEXSPI 0
//#define CONFIG_FSPI_AHB 1
#define POLL_TOUT_US   5000

static void prvFspiRDSR(u32 *, const void *, u32 );

#ifdef DEBUG_FLEXSPI
static void prvFspiDumpRegisters();
#endif

static inline void prvFspiWrite32( uint32_t xAddr, uint32_t xVal )
{
	out_le32( ( uint32_t * )( CCSR_BASE_ADDR + FSPI_CCSR_BASE_OFST + xAddr ),\
			( uint32_t ) xVal );
}

static inline uint32_t prvFspiRead32( uint32_t xAddr )
{
	return in_le32( ( uint32_t * )( CCSR_BASE_ADDR + FSPI_CCSR_BASE_OFST + xAddr ) );
}



static void prvFspiDisableModule( u8 xDisable )
{
	uint32_t  uiReg;

	uiReg = prvFspiRead32( FSPI_MCR0 );
	if( xDisable )
	{
		uiReg |= FSPI_MCR0_MDIS;
	}
	else
	{
		uiReg &= ( uint32_t ) ( ~ FSPI_MCR0_MDIS );
	}
	prvFspiWrite32( FSPI_MCR0, uiReg );
}

static void prvFspiLutLock()
{
	prvFspiWrite32( FSPI_LUTKEY, FSPI_LUTKEY_VALUE );
	prvFspiWrite32( FSPI_LCKCR, FSPI_LCKER_LOCK );
}

static void prvFspiLutUnlock()
{
	prvFspiWrite32( FSPI_LUTKEY,  FSPI_LUTKEY_VALUE );
	prvFspiWrite32( FSPI_LCKCR, FSPI_LCKER_UNLOCK );
}

static int iFspiPollTout(u32 RegOffset,u32 mask, u32 delay)
{
	u32 intr;

	do {
		intr = prvFspiRead32(RegOffset);
		if (intr & mask)
			break;
	} while (delay--);

        if (!delay) {
		return 0;
	}
	return intr;
}
static void prvFspiSetupLut()
{
	u32 xAddr, xInstr0, xInstr1;

	prvFspiLutUnlock();

	/* LUT Setup for READ Command */
	xAddr = FSPI_LUT_REG_OFST + ( u32 )( 0x10 * FSPI_READ_SEQ_ID );
	if (F_FLASH_SIZE_BYTES <= SZ_16M_BYTES) {
		xInstr0 = FSPI_INSTR_OPRND0( FSPI_NOR_CMD_READ ) \
			  | FSPI_INSTR_PAD0( FSPI_LUT_PAD1 ) \
			  | FSPI_INSTR_OPCODE0( FSPI_LUT_CMD );
		xInstr1 = FSPI_INSTR_OPRND1( FSPI_LUT_ADDR24BIT ) \
			  | FSPI_INSTR_PAD1( FSPI_LUT_PAD1 ) \
			  | FSPI_INSTR_OPCODE1( FSPI_LUT_ADDR );
	} else {
		xInstr0 = FSPI_INSTR_OPRND0( FSPI_NOR_CMD_READ_4B ) \
			  | FSPI_INSTR_PAD0( FSPI_LUT_PAD1 ) \
			  | FSPI_INSTR_OPCODE0( FSPI_LUT_CMD );
		xInstr1 = FSPI_INSTR_OPRND1( FSPI_LUT_ADDR32BIT ) \
			  | FSPI_INSTR_PAD1( FSPI_LUT_PAD1 ) \
			  | FSPI_INSTR_OPCODE1( FSPI_LUT_ADDR );
	}
	prvFspiWrite32( ( xAddr ), xInstr1 | xInstr0 );
	xInstr0 = FSPI_INSTR_OPRND0( 0 ) \
		  | FSPI_INSTR_PAD0(FSPI_LUT_PAD1) \
		  | FSPI_INSTR_OPCODE0(FSPI_LUT_READ);
	xInstr1 = 0;
	prvFspiWrite32( ( xAddr + 0x4 ), ( xInstr1 + xInstr0 ) );
	prvFspiWrite32( ( xAddr + 0x8 ), ( u32 ) 0x0 );	/* STOP command - unused instruction */
	prvFspiWrite32( ( xAddr + 0xc ), ( u32 ) 0x0 );   /* STOP command - unused instruction */

	/* LUT Setup for FAST READ Command */
	xAddr = FSPI_LUT_REG_OFST + ( u32 )( 0x10 * FSPI_FASTREAD_SEQ_ID );
	if (F_FLASH_SIZE_BYTES <= SZ_16M_BYTES) {
		xInstr0 = FSPI_INSTR_OPRND0( FSPI_NOR_CMD_FASTREAD ) \
			  | FSPI_INSTR_PAD0( FSPI_LUT_PAD1 ) \
			  | FSPI_INSTR_OPCODE0( FSPI_LUT_CMD ) \
			  | FSPI_INSTR_OPRND1( FSPI_LUT_ADDR24BIT ) \
			  | FSPI_INSTR_PAD1( FSPI_LUT_PAD1 ) \
			  | FSPI_INSTR_OPCODE1( FSPI_LUT_ADDR );
	} else {
		xInstr0 = FSPI_INSTR_OPRND0( FSPI_NOR_CMD_FASTREAD_4B ) \
			| FSPI_INSTR_PAD0( FSPI_LUT_PAD1 ) \
			| FSPI_INSTR_OPCODE0( FSPI_LUT_CMD ) \
			| FSPI_INSTR_OPRND1( FSPI_LUT_ADDR32BIT ) \
			| FSPI_INSTR_PAD1( FSPI_LUT_PAD1 ) \
			| FSPI_INSTR_OPCODE1( FSPI_LUT_ADDR );
	}
	prvFspiWrite32( ( xAddr ), xInstr0 );
	xInstr1 = FSPI_INSTR_OPRND0( 8 ) \
		| FSPI_INSTR_PAD0(FSPI_LUT_PAD1) \
		| FSPI_INSTR_OPCODE0(FSPI_DUMMY_SDR) \
		| FSPI_INSTR_OPRND1( 0 ) \
		| FSPI_INSTR_PAD1(FSPI_LUT_PAD1) \
		| FSPI_INSTR_OPCODE1(FSPI_LUT_READ);
	prvFspiWrite32( ( xAddr + 0x4 ), xInstr1 );
	prvFspiWrite32( ( xAddr + 0x8 ), ( u32 ) 0x0 );	/* STOP command - unused instruction */
	prvFspiWrite32( ( xAddr + 0xc ), ( u32 ) 0x0 );   /* STOP command - unused instruction */

	/* LUT Setup for Page Program */
	xAddr = FSPI_LUT_REG_OFST + ( u32 )( 0x10 * FSPI_WRITE_SEQ_ID );
	if (F_FLASH_SIZE_BYTES <= SZ_16M_BYTES) {
		xInstr0 = FSPI_INSTR_OPRND0( FSPI_NOR_CMD_PP ) \
			| FSPI_INSTR_PAD0( FSPI_LUT_PAD1 ) \
			| FSPI_INSTR_OPCODE0( FSPI_LUT_CMD );
		xInstr1 = FSPI_INSTR_OPRND1( FSPI_LUT_ADDR24BIT ) \
			| FSPI_INSTR_PAD1( FSPI_LUT_PAD1 ) \
			| FSPI_INSTR_OPCODE1( FSPI_LUT_ADDR );
	} else {
		xInstr0 = FSPI_INSTR_OPRND0( FSPI_NOR_CMD_PP_4B ) \
			  | FSPI_INSTR_PAD0( FSPI_LUT_PAD1 ) \
			  | FSPI_INSTR_OPCODE0( FSPI_LUT_CMD );
		xInstr1 = FSPI_INSTR_OPRND1( FSPI_LUT_ADDR32BIT ) \
			  | FSPI_INSTR_PAD1( FSPI_LUT_PAD1 ) \
			  | FSPI_INSTR_OPCODE1( FSPI_LUT_ADDR );
	}
	prvFspiWrite32( ( xAddr + 0x0 ), ( xInstr1 | xInstr0 ) );
	xInstr0 = FSPI_INSTR_OPRND0( 0 ) \
		| FSPI_INSTR_PAD0( FSPI_LUT_PAD1 ) \
		| FSPI_INSTR_OPCODE0( FSPI_LUT_WRITE );
	xInstr1 = 0;
	prvFspiWrite32( ( xAddr + 0x4 ), ( xInstr1 | xInstr0 ) );
	prvFspiWrite32( ( xAddr + 0x8 ), ( u32 ) 0x0 );	/* STOP command - unused instruction */
	prvFspiWrite32( ( xAddr + 0xc ), ( u32 ) 0x0 );   /* STOP command - unused instruction */

	/* LUT Setup for WREN */
	xAddr = FSPI_LUT_REG_OFST + ( u32 )( 0x10 * FSPI_WREN_SEQ_ID );
	xInstr0 = FSPI_INSTR_OPRND0( FSPI_NOR_CMD_WREN ) \
		| FSPI_INSTR_PAD0( FSPI_LUT_PAD1 ) \
		| FSPI_INSTR_OPCODE0( FSPI_LUT_CMD );
	prvFspiWrite32( ( xAddr + 0x0 ), ( xInstr0 ) );
	prvFspiWrite32( ( xAddr + 0x4 ), ( u32 ) 0x0 );
	prvFspiWrite32( ( xAddr + 0x8 ), ( u32 ) 0x0 );     /* STOP command - unused instruction */
	prvFspiWrite32( ( xAddr + 0xc ), ( u32 ) 0x0 );   /* STOP command - unused instruction */

	/* LUT Setup for Sector_Erase */
	xAddr = FSPI_LUT_REG_OFST + ( u32 )( 0x10 * FSPI_SE_SEQ_ID );
	if (F_FLASH_SIZE_BYTES <= SZ_16M_BYTES) {
		xInstr0 = FSPI_INSTR_OPRND0( FSPI_NOR_CMD_SE_64K ) \
			  | FSPI_INSTR_PAD0( FSPI_LUT_PAD1 ) \
			  | FSPI_INSTR_OPCODE0( FSPI_LUT_CMD ) \
			  | FSPI_INSTR_OPRND1( FSPI_LUT_ADDR24BIT ) \
			  | FSPI_INSTR_PAD1( FSPI_LUT_PAD1 ) \
			  | FSPI_INSTR_OPCODE1( FSPI_LUT_ADDR );
	} else {
		xInstr0 = FSPI_INSTR_OPRND0( FSPI_NOR_CMD_SE_64K_4B ) \
			  | FSPI_INSTR_PAD0( FSPI_LUT_PAD1 ) \
			  | FSPI_INSTR_OPCODE0( FSPI_LUT_CMD ) \
			  | FSPI_INSTR_OPRND1( FSPI_LUT_ADDR32BIT ) \
			  | FSPI_INSTR_PAD1( FSPI_LUT_PAD1 ) \
			  | FSPI_INSTR_OPCODE1( FSPI_LUT_ADDR );
	}
	prvFspiWrite32( ( xAddr + 0x0 ),  ( xInstr0 ) );
	prvFspiWrite32( ( xAddr + 0x4 ),  ( u32 ) 0x0 );
	prvFspiWrite32( ( xAddr + 0x8 ),  ( u32 ) 0x0 );
	prvFspiWrite32( ( xAddr + 0xc ),  ( u32 ) 0x0 );

	/* LUT Setup for Bulk_Erase */
	xAddr = FSPI_LUT_REG_OFST + ( u32 )( 0x10 * FSPI_BE_SEQ_ID );
	xInstr0 = FSPI_INSTR_OPRND0( FSPI_NOR_CMD_BE ) \
		  | FSPI_INSTR_PAD0( FSPI_LUT_PAD1 ) \
		  | FSPI_INSTR_OPCODE0( FSPI_LUT_CMD );
	prvFspiWrite32( ( xAddr + 0x0 ),  ( xInstr0 ) );
	prvFspiWrite32( ( xAddr + 0x4 ),  ( u32 ) 0x0 );
	prvFspiWrite32( ( xAddr + 0x8 ),  ( u32 ) 0x0 );
	prvFspiWrite32( ( xAddr + 0xc ),  ( u32 ) 0x0 );

	/* Read Status */
	xAddr = FSPI_LUT_REG_OFST + ( u32 )( 0x10 * FSPI_RDSR_SEQ_ID );
	xInstr0 = FSPI_INSTR_OPRND0(FSPI_NOR_CMD_RDSR) | FSPI_INSTR_PAD0(FSPI_LUT_PAD1) |
		FSPI_INSTR_OPCODE0(FSPI_LUT_CMD) | FSPI_INSTR_OPRND1(1) |
		FSPI_INSTR_PAD1(FSPI_LUT_PAD1) | FSPI_INSTR_OPCODE1(FSPI_LUT_READ);
	prvFspiWrite32( ( xAddr + 0x0 ),  ( xInstr0 ) );
	prvFspiWrite32( ( xAddr + 0x4 ),  ( u32 ) 0x0 );
	prvFspiWrite32( ( xAddr + 0x8 ),  ( u32 ) 0x0 );
	prvFspiWrite32( ( xAddr + 0xc ),  ( u32 ) 0x0 );

	prvFspiLutLock();
}

static inline void prvFspiAhbInvalidate(void)
{
	u32 reg;
	u32 tout = 10;

	log_dbg("In func %s %d\n\r", __func__, __LINE__);
	reg = prvFspiRead32(FSPI_MCR0);
	reg |= FSPI_MCR0_SWRST;
	prvFspiWrite32(FSPI_MCR0, reg);

	while(tout--) {
		if ((prvFspiRead32( FSPI_MCR0 ) & FSPI_MCR0_SWRST))
			vUdelay(1000);
	}

	if (!tout)
		log_err("%s:%d Failed to invalidate AHB\n", __func__, __LINE__);
}

int iFspiRead(u32 pcRxAddr, u32* pcRxBuf, u32 xSize_Bytes)
{
#if defined(CONFIG_FSPI_AHB)
	return iFspiAhbRead32(pcRxAddr, pcRxBuf, xSize_Bytes);
#else
	return iFspiIpRead(pcRxAddr, pcRxBuf, xSize_Bytes);
#endif
}

int iFspiAhbRead32(u32 pcRxAddr, u32* pcRxBuf, u32 xSize_Bytes)
{
	log_dbg("In func %s %p\n\r", __func__, ( pcRxAddr ));

	if (F_FLASH_SIZE_BYTES <= SZ_16M_BYTES)
		pcRxAddr = ( ( u32 )( pcRxAddr & MASK_24BIT_ADDRESS ) );
	else
		pcRxAddr = ( ( u32 )( pcRxAddr & MASK_32BIT_ADDRESS ) );

	pcRxAddr = ( ( u32 )( pcRxAddr + FSPI_AHB_BASE_ADDR ) );

	log_info("In func %s %p\n\r", __func__, ( pcRxAddr ));

	if(pcRxAddr % 4 || (u32 )pcRxBuf % 4) {
		log_dbg("In func %s unaligned Start Address src=%#p dst=%#p\n\r", __func__,
				( pcRxAddr - FSPI_AHB_BASE_ADDR ), pcRxBuf);
	}

	/* Directly copy from AHB Buffer */
	memcpy( (void *)pcRxBuf, (void *)pcRxAddr, xSize_Bytes );

	prvFspiAhbInvalidate();
	return FSPI_SUCCESS;
}

int iFspiIpRead(u32 pcRxAddr, u32 *pvRxBuf, u32 uiLen )
{

	u32 i = 0, j = 0, xRem = 0;
	u32 xIteration = 0, xSizeRx = 0, xSizeWm, temp_size;
	u32 data = 0;
	u32 xLen_Bytes;
	u32 xAddr, intr;

	log_info("In func %s %p\n\r", __func__, ( pcRxAddr ));
	xAddr = ( u32 ) pcRxAddr;
	xLen_Bytes = uiLen;

	/* Watermark level : 8 bytes. (BY DEFAULT) */
	xSizeWm = 8;

#ifdef DEBUG_FLEXSPI
	prvFspiDumpRegisters();
#endif

	/* Clear  RX Watermark interrupt in INT register, if any existing.  */
	prvFspiWrite32( FSPI_INTR, FSPI_INTR_IPRXWA );

	/* Invalid the RXFIFO, to run next IP Command */
	prvFspiWrite32( FSPI_IPRXFCR, FSPI_IPRXFCR_CLR );    /* Clears all data entries in IP Rx FIFOs, R/W pointers will also reset */
	prvFspiWrite32( FSPI_INTR, FSPI_INTEN_IPCMDDONE );

	while( xLen_Bytes )
	{

		/* FlexSPI can store no more than  FSPI_RX_IPBUF_SIZE */
		xSizeRx = ( xLen_Bytes >  FSPI_RX_IPBUF_SIZE ) ?  FSPI_RX_IPBUF_SIZE : xLen_Bytes;


		/* IP Control Register0 - SF Address to be read */
		prvFspiWrite32( FSPI_IPCR0, xAddr );
		/* IP Control Register1 - SEQID_READ operation, Size */
		if (CONFIG_FSPI_FASTREAD == 0) {
			prvFspiWrite32( FSPI_IPCR1, ( u32 )( FSPI_READ_SEQ_ID << FSPI_IPCR1_ISEQID_SHIFT) | ( u16 ) xSizeRx );
		} else {
			prvFspiWrite32( FSPI_IPCR1, ( u32 )( FSPI_FASTREAD_SEQ_ID << FSPI_IPCR1_ISEQID_SHIFT) | ( u16 ) xSizeRx );
		}
		
		intr = iFspiPollTout(FSPI_STS0, (FSPI_STS0_ARB_IDLE | FSPI_STS0_SEQ_IDLE), POLL_TOUT_US);

		if (!intr) {
			log_err("FlexSPI controller is busy. FSPI_STS0=%#x \n\r", intr);
                        return FSPI_IP_READ_FAIL;
                }

		/* Trigger IP Read Command */
		prvFspiWrite32( FSPI_IPCMD, FSPI_IPCMD_TRG_MASK );

		intr = prvFspiRead32(FSPI_INTR);
		if ((intr & FSPI_INTR_IPCMDGE) ||
				(intr & FSPI_INTR_IPCMDERR ))
		{
			log_err("Error in IP READ INTR=%#x\n\r", intr);
			return FSPI_IP_READ_FAIL;
		}
		/* Will read in n iterations of each 8 FIFO's (equal to watermark level)), which */
		xIteration = xSizeRx / xSizeWm;
		for( i = 0; i < xIteration; i++ )
		{
			/* Wait for IP Rx Watermark Fill event, before reading IP RX FIFO's */
	                intr = iFspiPollTout(FSPI_INTR,FSPI_INTR_IPRXWA_MASK, POLL_TOUT_US);

			if (!intr) {
                        	log_err("Error: FSPI_INTR=%#x \n\r",  intr);
                        	return FSPI_IP_READ_FAIL;
			}

			/* Read all RX FIFO's(upto watermark level) & copy  to rxbuffer */
			for( j = 0; j < xSizeWm; j += 4 )
			{
				/* Read FIFO Data Register */
				data = prvFspiRead32( FSPI_RFDR + j );
				memcpy( pvRxBuf++, &data, 4 );

			}

			/* Clear IP_RX_WATERMARK Event in INTR register */
			/* This will reset the FIFO Read pointer, for next iteration.*/
			prvFspiWrite32( FSPI_INTR, FSPI_INTR_IPRXWA );
		}

		xRem = xSizeRx % xSizeWm;

		if( xRem )
		{
			/* Wait for data filled */
			intr = iFspiPollTout(FSPI_IPRXFSTS,FSPI_IPRXFSTS_FILL_MASK, POLL_TOUT_US);

	                if (!intr) {
        	                log_err("Error: FSPI_IPRXFSTS=%#x \n\r", intr);
                	        return FSPI_IP_READ_FAIL;
                	}

			temp_size = 0;
			j = 0;
			while (xRem > 0) {
				data = 0;
				data =  prvFspiRead32( FSPI_RFDR +j );
				temp_size = (xRem < 4) ? xRem : 4;
				memcpy( pvRxBuf++, &data, temp_size );
				xRem -=temp_size;
			}
		}


		while(!( prvFspiRead32( FSPI_INTR) & FSPI_INTR_IPCMDDONE_MASK) )
			;

		/* Invalid the RX FIFO, to run next IP Command */
		prvFspiWrite32( FSPI_IPRXFCR, FSPI_IPRXFCR_CLR );    	/* invalidates RX FIFO's */
		prvFspiWrite32( FSPI_INTR, FSPI_INTR_IPCMDDONE_MASK );  /* Clear IP Command Done flag in interrupt register*/

		/* Update remaining len, Increment xAddr read pointer. */
		xLen_Bytes -= xSizeRx;
		xAddr += xSizeRx;
	}

	return FSPI_SUCCESS;
}

void prvFspiIpWrite(u32 pcWrAddr, u32* pvWrBuf, u32 uiLen )
{

	u32 xIteration = 0, xRem = 0;
	u32 xSizeTx = 0, xSizeWm, temp_size;
	u32 i = 0,j = 0;
	u32 uiData= 0;
	u32 xAddr, xLen_Bytes, intr;

	xSizeWm = 8;				/* Default TX WaterMark level: 8 Bytes. */
	xAddr = ( u32 )pcWrAddr;
	xLen_Bytes = uiLen;

	/* Invalid the TXFIFO, to run next IP Command */
	//prvFspiWrite32( FSPI_IPTXFCR, FSPI_IPTXFCR_CLR );    /* Clear the TX FIFO's. */

	while( xLen_Bytes )
	{

		xSizeTx = ( xLen_Bytes >  FSPI_TX_IPBUF_SIZE ) ?  FSPI_TX_IPBUF_SIZE : xLen_Bytes;

		/* IP Control Register0 - SF Address to be read */
		prvFspiWrite32( FSPI_IPCR0, xAddr );

		/*
		 * Fill TX FIFO's..
		 *
		 */

		xIteration = xSizeTx / xSizeWm;
		for( i = 0; i < xIteration; i++ )
		{

			/* Ensure TX FIFO Watermark Available, last data has already transmitted. */
                        intr = iFspiPollTout(FSPI_INTR,FSPI_INTR_IPTXWE_MASK, POLL_TOUT_US);

                        if (!intr) {
                                log_err("Error: FSPI_INTR=%#x \n\r",  intr);
                                return ;
                        }


			/* Fill TxFIFO's ( upto watermark level) */
			for( j = 0; j < xSizeWm; j += 4)
			{
				memcpy( &uiData, pvWrBuf++,  4 );
				/* Write TX FIFO Data Register */
				prvFspiWrite32( ( FSPI_TFDR + j ), uiData );

			}

			/* Clear IP_TX_WATERMARK Event in INTR register */
			/* This will start pushing  data in FIFO, reset the FIFO Write pointer to start location, for next iteration.*/
			prvFspiWrite32( FSPI_INTR, FSPI_INTR_IPTXWE );
		}
		xRem = xSizeTx % xSizeWm;
		if( xRem )
		{
			/* Wait for TXFIFO empty */
			while(! ( prvFspiRead32( FSPI_INTR ) & FSPI_INTR_IPTXWE ) )
				;

			temp_size = 0;
			j = 0;
			while (xRem > 0) {
				uiData = 0;
				temp_size = (xRem < 4) ? xRem : 4;
				memcpy( &uiData, pvWrBuf++, temp_size );
				prvFspiWrite32( ( FSPI_TFDR + j ), uiData );
				xRem -=temp_size;
			}

			/* Clear IP_TX_WATERMARK Event in INTR register */
			/* This will start pushing data, reset the FIFO's Write pointer, for next iteration.*/
			prvFspiWrite32( FSPI_INTR, FSPI_INTR_IPTXWE );
		}

		/* IP Control Register1 - SEQID_WRITE operation, Size */
		prvFspiWrite32( FSPI_IPCR1, (u32)( FSPI_WRITE_SEQ_ID << FSPI_IPCR1_ISEQID_SHIFT ) | ( u16 ) xSizeTx );
		/* Trigger IP Write Command */
		prvFspiWrite32( FSPI_IPCMD, FSPI_IPCMD_TRG_MASK );

		/* Wait for IP Write command done, before moving to next operation */
		while( !( prvFspiRead32( FSPI_INTR ) & FSPI_INTR_IPCMDDONE_MASK ) )
			;

		/* Invalidate TX FIFOs & acknowledge IP_CMD_DONE event  for next Command */
		prvFspiWrite32( FSPI_IPTXFCR, FSPI_IPTXFCR_CLR );
		prvFspiWrite32( FSPI_INTR, FSPI_INTR_IPCMDDONE_MASK );

		/* for next iteration */
		xLen_Bytes  -=  xSizeTx;
		xAddr += xSizeTx;
	}

}

int iFspiIpWrite(u32 pcWrAddr, u32* pvWrBuf, u32 uiLen )
{

	u32 xAddr;
	u32 xPage1_Len = 0, xPageL_Len = 0;
	u32 i, j = 0;
	u32 *Buf = pvWrBuf;

	xAddr = ( u32 )( pcWrAddr );
	if ((uiLen <= F_PAGE_256) && !(xAddr % F_PAGE_256)) {
		xPage1_Len = uiLen;
	}
	else if ((uiLen <= F_PAGE_256) && (xAddr % F_PAGE_256)) {
		xPage1_Len = (F_PAGE_256 - (xAddr % F_PAGE_256) );
		if(uiLen > xPage1_Len) {
			xPageL_Len = (uiLen - xPage1_Len) % F_PAGE_256;
		} else {
			xPage1_Len = uiLen;
			xPageL_Len = 0;
		}
		j = 0;
	}
	else if ((uiLen > F_PAGE_256) && !(xAddr % F_PAGE_256)) {
		j = uiLen / F_PAGE_256;
		xPageL_Len = uiLen % F_PAGE_256;
	}
	else if ((uiLen > F_PAGE_256) && (xAddr % F_PAGE_256)) {
		xPage1_Len = (F_PAGE_256 - (xAddr % F_PAGE_256) );
		j = (uiLen - xPage1_Len) / F_PAGE_256;
		xPageL_Len = (uiLen - xPage1_Len) % F_PAGE_256;
	}

	if(xPage1_Len) {
		iFspiWren(xAddr);
		prvFspiIpWrite(xAddr, Buf, xPage1_Len );
		while (bFlashIsBusy())
			;
		xAddr +=xPage1_Len;
		/* TODO What is buf start is not 4 aligned */
		Buf = Buf + xPage1_Len/sizeof(u32);
	}

	for(i = 0; i < j; i++)
	{
		iFspiWren(xAddr);
		prvFspiIpWrite(xAddr, Buf, F_PAGE_256 );
		while (bFlashIsBusy())
			;
		xAddr +=F_PAGE_256;
		/* TODO What is buf start is not 4 aligned */
		Buf = Buf + F_PAGE_256/4;
	}

	if(xPageL_Len) {
		iFspiWren(xAddr);
		prvFspiIpWrite(xAddr, Buf, xPageL_Len );
		while (bFlashIsBusy())
			;
	}

	prvFspiAhbInvalidate();
	return FSPI_SUCCESS;
}

static void vFspiFillTxFifos( u32 xSizeTx,  u8 iter)
{
	u32 j, i;
	u32 intr;

	for( i = 0; i < xSizeTx / 8; i++ )
	{
                /* wait for  TX Watermark Available */
 		intr = iFspiPollTout(FSPI_INTR,FSPI_INTR_IPTXWE_MASK, POLL_TOUT_US);
		if (!intr) {
			log_err("Error: vFspiFillTxFifos FSPI_INTR=%#x \n\r",  intr);
			return ;
		}

		for( j = 0; j < 8; j += 4)
		{
			prvFspiWrite32( ( u32 )( FSPI_TFDR + j ), ( i + iter ) );
			log_dbg( "Register(0x%08x) : 0x%08x \n\r",  (FSPI_TFDR + j), (i + iter) );
		}

		prvFspiWrite32( FSPI_INTR, FSPI_INTR_IPTXWE );
	}

	log_dbg( "ipWrite: Fifo's filled\n\r" );
}

void vFspiIpWrite_simple( u32 size_bytes,  u8 iter )
{

	u32 xSizeTx;
	u32 xAddr;

	xSizeTx= size_bytes;
	xAddr = 0;

	prvFspiWrite32( FSPI_IPTXFCR, FSPI_IPTXFCR_CLR );

	prvFspiWrite32( FSPI_IPCR0, xAddr);
	prvFspiWrite32( FSPI_IPCR1, (u32)( FSPI_WRITE_SEQ_ID << FSPI_IPCR1_ISEQID_SHIFT) | (u16) (xSizeTx) );
	prvFspiWrite32( FSPI_IPCMD, FSPI_IPCMD_TRG_MASK);

	vFspiFillTxFifos( xSizeTx, iter );

	/* Wait for IP Write command done, before moving to next operation */
	while( !( prvFspiRead32( FSPI_INTR) & FSPI_INTR_IPCMDDONE_MASK) )
		;

	/* Clear IP_CMD_DONE flag. */
	prvFspiWrite32( FSPI_INTR, FSPI_INTR_IPCMDDONE_MASK );
}


int iFspiWren(u32 pcWrAddr)
{

	prvFspiWrite32( FSPI_IPTXFCR, FSPI_IPTXFCR_CLR );

	prvFspiWrite32( FSPI_IPCR0, ( u32 )pcWrAddr );
	prvFspiWrite32( FSPI_IPCR1, ( ( FSPI_WREN_SEQ_ID << FSPI_IPCR1_ISEQID_SHIFT ) |  0 ) );
	prvFspiWrite32( FSPI_IPCMD, FSPI_IPCMD_TRG_MASK );

	while( !( prvFspiRead32( FSPI_INTR ) & FSPI_INTR_IPCMDDONE_MASK ) )
		;

	prvFspiWrite32( FSPI_INTR, FSPI_INTR_IPCMDDONE_MASK );
	return FSPI_SUCCESS;
}


static void prvFspiRDSR(u32 *rxbuf, const void *p_addr, u32 size)
{
	u32 iprxfcr = 0;
	u32 data = 0;

	iprxfcr = prvFspiRead32(FSPI_IPRXFCR);
	/* IP RX FIFO would be read by processor */
	iprxfcr = iprxfcr & (u32)~FSPI_IPRXFCR_CLR;
	/* Invalid data entries in IP RX FIFO */
	iprxfcr = iprxfcr | FSPI_IPRXFCR_CLR;
	prvFspiWrite32(FSPI_IPRXFCR, iprxfcr);

	prvFspiWrite32(FSPI_IPCR0, (u32) p_addr);
	prvFspiWrite32(FSPI_IPCR1, (u32) ((FSPI_RDSR_SEQ_ID << FSPI_IPCR1_ISEQID_SHIFT) | (u16) size));
	/* Trigger the command */
	prvFspiWrite32(FSPI_IPCMD, FSPI_IPCMD_TRG_MASK);
	/* Wait for command done */
	while (!(prvFspiRead32(FSPI_INTR) & FSPI_INTR_IPCMDDONE_MASK))
		;
	prvFspiWrite32(FSPI_INTR, FSPI_INTR_IPCMDDONE_MASK);

	data = prvFspiRead32(FSPI_RFDR);
	memcpy(rxbuf, &data, size);

	/* Rx FIFO invalidation needs to be done prior w1c of INTR.IPRXWA bit */
	prvFspiWrite32(FSPI_IPRXFCR, FSPI_IPRXFCR_CLR);
	prvFspiWrite32(FSPI_INTR, FSPI_INTR_IPRXWA_MASK);
	prvFspiWrite32(FSPI_INTR, FSPI_INTR_IPCMDDONE_MASK);

}

bool bFlashIsBusy (void)
{
#define FSPI_ONE_BYTE 1
	u8 data[4];

	log_dbg( "In func %s\n\n\r", __func__ );
	prvFspiRDSR((u32 *) data, 0, FSPI_ONE_BYTE);

	return !!((u32) data[0] & FSPI_NOR_SR_WIP_MASK);
}

int iFspiErase()
{
	//log_dbg( "In func %s\n\r", __func__ );
	iFspiWren( ( u32 ) 0x0 );
	//prvFspiBulkErase();
        prvFspiWrite32( FSPI_IPCR0, 0x0 );
        prvFspiWrite32( FSPI_IPCR1, ( (FSPI_BE_SEQ_ID << FSPI_IPCR1_ISEQID_SHIFT) | 20 )  );
        prvFspiWrite32( FSPI_IPCMD, FSPI_IPCMD_TRG_MASK );

        while( !( prvFspiRead32( FSPI_INTR ) & FSPI_INTR_IPCMDDONE_MASK ) )
                ;
        prvFspiWrite32( FSPI_INTR, FSPI_INTR_IPCMDDONE_MASK );
	vUdelay(1000); //add some dealy

	prvFspiAhbInvalidate();
	return FSPI_SUCCESS;
}

static void prvFspiSectorErase(u32 pcWrAddr)
{
	u32 xAddr;
	u32 intr;

	xAddr = ( u32 )( pcWrAddr );
	prvFspiWrite32( FSPI_IPCR0, xAddr);
	log_dbg("In [%s][%d] Erase address %#x\n\r", __func__, __LINE__,
			(xAddr));
	prvFspiWrite32( FSPI_IPCR1, ( (FSPI_SE_SEQ_ID << FSPI_IPCR1_ISEQID_SHIFT) | 0 ) );
	prvFspiWrite32( FSPI_IPCMD, FSPI_IPCMD_TRG_MASK );

                        /* Ensure TX FIFO Watermark Available, last data has already transmitted. */
	intr = iFspiPollTout(FSPI_INTR,FSPI_INTR_IPCMDDONE_MASK, POLL_TOUT_US);

        if (!intr) {
		log_err("Error: IPCMDDONE not set. FSPI_INTR=%#x \n\r",  intr);
		return ;
	}
	prvFspiWrite32( FSPI_INTR, FSPI_INTR_IPCMDDONE_MASK );

}

int iFspiSecErase(u32 pcWrAddr, u32 uiLen)
{
	u32 xAddr, xLen_Bytes, i, j = 0;

	//log_dbg( "In func %s\n\r", __func__ );
	xAddr = ( u32 )( pcWrAddr );
	if (xAddr % F_SECTOR_ERASE_SZ) {
		log_err("!!! In func %s, unalinged start address"
			" can only be in multiples of %#x\n\r", __func__, F_SECTOR_ERASE_SZ);
		return FSPI_ERASE_FAIL;
	}

	xLen_Bytes = uiLen * 1;
	if (xLen_Bytes < F_SECTOR_ERASE_SZ) {
		log_err("!!! In func %s, Less than 1 sector"
			" can only be in multiples of %#x\n\r", __func__, F_SECTOR_ERASE_SZ);
		return FSPI_ERASE_FAIL;
	}

	if (xLen_Bytes % F_SECTOR_ERASE_SZ)
		j = xLen_Bytes/F_SECTOR_ERASE_SZ + 1;
	else
		j = xLen_Bytes/F_SECTOR_ERASE_SZ;

	for (i = 0; i < j ; i++)
	{
		iFspiWren(xAddr + (F_SECTOR_ERASE_SZ * i));
		prvFspiSectorErase(xAddr + (F_SECTOR_ERASE_SZ * i));
		while (bFlashIsBusy())
			;
	}
	prvFspiAhbInvalidate();
	return FSPI_SUCCESS;
}

#ifdef DEBUG_FLEXSPI
static void prvFspiDumpRegisters()
{
	uint32_t i;

	log_dbg( "\n\rRegisters Dump:\n\r" );
	log_dbg( "Flexspi: Register FSPI_MCR0(0x%x) = 0x%08x \n\r", FSPI_MCR0, prvFspiRead32( FSPI_MCR0 ) );
	log_dbg( "Flexspi: Register FSPI_MCR1(0x%x) = 0x%08x \n\r", FSPI_MCR1, prvFspiRead32( FSPI_MCR1 ) );
	log_dbg( "Flexspi: Register FSPI_MCR2(0x%x) = 0x%08x \n\r", FSPI_MCR2, prvFspiRead32( FSPI_MCR2 ) );
	log_dbg( "Flexspi: Register FSPI_DLL_A_CR(0x%x) = 0x%08x \n\r", FSPI_DLLACR, prvFspiRead32( FSPI_DLLACR ) );
	log_dbg( "\n\r" );

	log_dbg( "Flexspi: Register FSPI_AHBRX_BUF0CR0(0x%x) = 0x%08x \n\r",FSPI_AHBRX_BUF0CR0, prvFspiRead32(FSPI_AHBRX_BUF0CR0)  );
	log_dbg( "Flexspi: Register FSPI_AHBRX_BUF1CR0(0x%x) = 0x%08x \n\r",FSPI_AHBRX_BUF1CR0, prvFspiRead32(FSPI_AHBRX_BUF1CR0)  );
	log_dbg( "Flexspi: Register FSPI_AHBRX_BUF2CR0(0x%x) = 0x%08x \n\r",FSPI_AHBRX_BUF2CR0, prvFspiRead32(FSPI_AHBRX_BUF2CR0)  );
	log_dbg( "Flexspi: Register FSPI_AHBRX_BUF3CR0(0x%x) = 0x%08x \n\r",FSPI_AHBRX_BUF3CR0, prvFspiRead32(FSPI_AHBRX_BUF3CR0)  );
	log_dbg( "Flexspi: Register FSPI_AHBRX_BUF4CR0(0x%x) = 0x%08x \n\r",FSPI_AHBRX_BUF4CR0, prvFspiRead32(FSPI_AHBRX_BUF4CR0)  );
	log_dbg( "Flexspi: Register FSPI_AHBRX_BUF5CR0(0x%x) = 0x%08x \n\r",FSPI_AHBRX_BUF5CR0, prvFspiRead32(FSPI_AHBRX_BUF5CR0)  );
	log_dbg( "Flexspi: Register FSPI_AHBRX_BUF6CR0(0x%x) = 0x%08x \n\r",FSPI_AHBRX_BUF6CR0, prvFspiRead32(FSPI_AHBRX_BUF6CR0)  );
	log_dbg( "Flexspi: Register FSPI_AHBRX_BUF7CR0(0x%x) = 0x%08x \n\r",FSPI_AHBRX_BUF7CR0, prvFspiRead32(FSPI_AHBRX_BUF7CR0)  );
	log_dbg( "Flexspi: Register FSPI_AHB_CR(0x%x) \t  = 0x%08x \n\r", FSPI_AHBCR, prvFspiRead32(FSPI_AHBCR) );
	log_dbg( "\n\r");

	for( i = 0; i < 3; i++) {
		uint32_t j = i * 16;
		log_dbg( "Flexspi: Register FSPI_FLSH_A1_CR0,(0x%x) = 0x%08x \n\r", FSPI_FLSHA1CR0 + j, prvFspiRead32( FSPI_FLSHA1CR0 + j) );
	}
	log_dbg( "Flexspi: Register FSPI_STS0(0x%x) \t  = 0x%08x \n\r", FSPI_STS0, prvFspiRead32(FSPI_STS0) );
	log_dbg( "Flexspi: Register FSPI_STS1(0x%x) \t  = 0x%08x \n\r", FSPI_STS1, prvFspiRead32(FSPI_STS1) );
	log_dbg( "Flexspi: Register FSPI_STS2(0x%x) \t  = 0x%08x \n\r", FSPI_STS2, prvFspiRead32(FSPI_STS2) );
	log_dbg( "Flexspi: Register FSPI_AHBSPNST(0x%x) \t  = 0x%08x \n\r", FSPI_AHBSPNST, prvFspiRead32(FSPI_AHBSPNST) );

	for( i = 0; i < 4; i++) {
		uint32_t j = i*0x10;
		log_dbg( "Flexspi: Register FSPI_FLSH_LUT. (0x%x, )= 0x%08x ", FSPI_LUT_REG_OFST + j, prvFspiRead32( FSPI_LUT_REG_OFST + j));
		log_dbg( "(0x%x, )= 0x%08x ", FSPI_LUT_REG_OFST + j+4, prvFspiRead32( FSPI_LUT_REG_OFST + j+4));
		log_dbg( "(0x%x, )= 0x%08x ", FSPI_LUT_REG_OFST + j+8, prvFspiRead32( FSPI_LUT_REG_OFST + j+8));
		log_dbg( "(0x%x, )= 0x%08x \n\r", FSPI_LUT_REG_OFST + j+0xC, prvFspiRead32( FSPI_LUT_REG_OFST + j+0xC));

	}
}
#endif


int iFspiInit()
{
	u32 	mcrx;
	u32	flash_size;
	uint32_t	xVal;
	u32 i, xFlashCR2;

#if NXP_ERRATUM_A050426
	u32 xInstr0, xInstr1;

	log_dbg( "Flexspi: NXP_ERRATUM_A050426\n\r" );
	prvFspiWrite32( FSPI_LUT_REG_OFST + 0x0, 0x0 );
	prvFspiWrite32( FSPI_LUT_REG_OFST + 0x4, 0x0 );
	prvFspiWrite32( FSPI_LUT_REG_OFST + 0x8, 0x0 );
	prvFspiWrite32( FSPI_LUT_REG_OFST + 0xC, 0x0 );
	vCreateMpuEntry(MPU_REGION_FSPI_RX_TX_FIFO, FSPI_RX_TX_FIFO_BASE_ADDR, FSPI_RX_TX_FIFO_END_ADDR);
	out_le32( 0xD1000000, 0x0 );
	vDeleteMpuEntry(MAS0_SEL_1 | MAS0_ESEL_DATA_11);
	prvFspiWrite32( FSPI_IPTXFCR, FSPI_IPTXFCR_CLR );
	log_dbg( "Flexspi: NXP_ERRATUM_A050426.. Waiting for FSPI_STS0 %x\n\r", prvFspiRead32( FSPI_STS0 ) );
	while( !(prvFspiRead32( FSPI_STS0 ) & FSPI_STS0_ARB_IDLE ) );
	prvFspiWrite32 ( FSPI_IPRXFCR, FSPI_IPRXFCR_CLR );
	prvFspiWrite32 ( FSPI_IPCR0, 0x0 );
	prvFspiWrite32( FSPI_IPCR1, ( (FSPI_WREN_SEQ_ID << FSPI_IPCR1_ISEQID_SHIFT) | 8 ) );
	xInstr0 = FSPI_INSTR_OPRND0( FSPI_NOR_CMD_READ ) | FSPI_INSTR_PAD0( FSPI_LUT_PAD1 ) | FSPI_INSTR_OPCODE0( FSPI_LUT_CMD );
	xInstr1 = FSPI_INSTR_OPRND1( FSPI_LUT_ADDR24BIT ) | FSPI_INSTR_PAD1( FSPI_LUT_PAD1 ) |  FSPI_INSTR_OPCODE1( FSPI_LUT_ADDR );
	prvFspiWrite32( FSPI_LUT_REG_OFST + 0x10, ( xInstr0 | xInstr1 ));
	xInstr0 = FSPI_INSTR_OPRND0( FSPI_NOR_CMD_PP ) | FSPI_INSTR_PAD0( FSPI_LUT_PAD1 ) | FSPI_INSTR_OPCODE0( FSPI_LUT_NXP_READ );
	xInstr1 = 0;
	prvFspiWrite32( FSPI_LUT_REG_OFST + 0x14, ( xInstr0 | xInstr1 ));
	prvFspiWrite32( FSPI_LUT_REG_OFST + 0x18, 0x00000000);
	prvFspiWrite32( FSPI_LUT_REG_OFST + 0x1C, 0x00000000);
	prvFspiWrite32( FSPI_IPCMD, FSPI_IPCMD_TRG_MASK );
	log_dbg( "Flexspi: NXP_ERRATUM_A050426.. Waiting for FSPI_INTR %x\n\r", prvFspiRead32( FSPI_INTR ) );
	while( !(prvFspiRead32( FSPI_INTR ) & FSPI_INTR_IPRXWA_MASK ) );
	prvFspiWrite32 ( FSPI_IPRXFCR, FSPI_IPRXFCR_CLR );
	prvFspiWrite32 ( FSPI_INTR, FSPI_INTR_IPCMDDONE_MASK | FSPI_INTR_IPRXWA_MASK );
	log_info( "Flexspi: NXP_ERRATUM_A050426..Done\n\r" );
#endif

	log_info( "\n\rFlexspi driver: Version v1.0\n\r" );
	log_info( "Flexspi: Default MCR0 = 0x%08x, before reset\n\r", prvFspiRead32( FSPI_MCR0 ) );
	log_info( "Flexspi: Resetting controller...\n\r" );

	/* Reset FlexSpi Controller */
	prvFspiWrite32( FSPI_MCR0, FSPI_MCR0_SWRST | (FSPI_SER_CLK_DIV << FSPI_MCR0_SERCLKDIV_SHIFT) );
	while( (prvFspiRead32( FSPI_MCR0 ) & FSPI_MCR0_SWRST ) )
		;  /* FSPI_MCR0_SWRESET_MASK */

	/* Disable Controller Module before programming its registersi, especially MCR0 (Master Control Register0) */
	prvFspiDisableModule( 1 );
	/*
	 * Program MCR0 with default values, AHB Timeout(0xff), IP Timeout(0xff).  {FSPI_MCR0- 0xFFFF0000}
	 */

	/* Set the FlexSPI clock speed */
	xVal = in_le32( ( uint32_t * )(FLEXSPICR1_ADDR ) );
	xVal &= 0xffffffe0;     //clear FlexSPI_CLK_DIV
	xVal |= FLEXSPICR1_CLK_DIV_3;
	out_le32( ( uint32_t * )( FLEXSPICR1_ADDR ), xVal );

	/* Timeout wait cycle for AHB command grant */
	mcrx = prvFspiRead32(FSPI_MCR0);
	mcrx |= (u32)( ( FSPI_MAX_TIMEOUT_AHBCMD << FSPI_MCR0_AHBGRANTWAIT_SHIFT ) & ( FSPI_MCR0_AHBGRANTWAIT_MASK ) );

	/* Time out wait cycle for IP command grant*/
	mcrx |= (u32) ( FSPI_MAX_TIMEOUT_IPCMD << FSPI_MCR0_IPGRANTWAIT_SHIFT ) & ( FSPI_MCR0_IPGRANTWAIT_MASK );
	mcrx |= (u32) ( FSPI_SER_CLK_DIV << FSPI_MCR0_SERCLKDIV_SHIFT ) & ( FSPI_MCR0_SERCLKDIV_MASK );
	/* Not set HSEN 
	   mcrx |= (u32) ( FSPI_HSEN << FSPI_MCR0_HSEN_SHIFT) & ( FSPI_MCR0_HSEN_MASK ); */
	mcrx |= ( (0 << FSPI_MCR0_RXCLKSRC_SHIFT) & FSPI_MCR0_RXCLKSRC_MASK);

	prvFspiWrite32( FSPI_MCR0, mcrx );

#if 0
	mcrx = prvFspiRead32(FSPI_MCR2);
	mcrx &= ~FSPI_MCR2_SAMEDEVICEEN;

	prvFspiWrite32( FSPI_MCR2, mcrx );
#endif

	/* Reset the DLL register to default value */
	prvFspiWrite32( FSPI_DLLACR, FSPI_DLLACR_OVRDEN );
	prvFspiWrite32( FSPI_DLLBCR, FSPI_DLLBCR_OVRDEN );
#if NXP_ERRATUM_A050272	/* ERRATA DLL */
	for (uint8_t delay = 100U; delay > 0U; delay--)
	{
		__asm__ volatile ( "se_nop" );
	}
#endif

	/* Configure flash control registers for different chip select */
	flash_size = ( F_FLASH_SIZE_BYTES * FLASH_NUM ) / FSPI_BYTES_PER_KBYTES;    /* Flash size in Kilobytes */
	prvFspiWrite32( FSPI_FLSHA1CR0, flash_size );
	prvFspiWrite32( FSPI_FLSHA1CR1, 
			( (FSPI_FLSHXCR1_TCSH_DEFAULT << FSPI_FLSHXCR1_TCSH_SHIFT) & 
			  FSPI_FLSHXCR1_TCSH_MASK ) | ( (FSPI_FLSHXCR1_TCSS_DEFAULT << FSPI_FLSHXCR1_TCSS_SHIFT ) & FSPI_FLSHXCR1_TCSS_MASK ) );

	/* Reset AHB RX buffer CR configuration */
	for( i = 0; i < 7; i++ )
	{
		prvFspiWrite32( ( FSPI_AHBRX_BUF0CR0 + 4 * i ), 0 );
	}

	/* Set ADATSZ with the maximum AHB buffer size */
	prvFspiWrite32( FSPI_AHBRX_BUF7CR0, ( ( u32 ) ( FSPI_RX_MAX_AHBBUF_SIZE / 8 )  | ( u32 ) FSPI_AHBRXBUF0CR7_PREF ) );

	/* Known limitation handling: prefetch and no start address alignment. */
	prvFspiWrite32( FSPI_AHBCR, FSPI_AHBCR_PREF_EN);
	//        prvFspiWrite32( FSPI_AHBCR, FSPI_AHBCR_PREF_EN |  FSPI_AHBCR_RDADDROPT );


	/* Setup AHB READ sequenceID for all flashes. */
	/*      LA12xx only support one CS */
	xFlashCR2 = prvFspiRead32( FSPI_FLSHA1CR2 );
	log_dbg("xFlashCR2=%#x\n\r", xFlashCR2);

	if (CONFIG_FSPI_FASTREAD == 0) {
		xFlashCR2 |= ( (FSPI_READ_SEQ_ID << FSPI_FLSHXCR2_ARDSEQI_SHIFT) & 0x1f );
	} else {
		xFlashCR2 |= ( (FSPI_FASTREAD_SEQ_ID << FSPI_FLSHXCR2_ARDSEQI_SHIFT) & 0x1f );
	}
	prvFspiWrite32( FSPI_FLSHA1CR2,  xFlashCR2);

	/*
	   prvFspiWrite32( FSPI_FLSHA2CR2,  FSPI_FAST_READ_SEQ_ID);
	   prvFspiWrite32( FSPI_FLSHB1CR2,  FSPI_READ_SEQ_ID);
	   prvFspiWrite32( FSPI_FLSHB2CR2,  FSPI_READ_SEQ_ID);
	 */

	//	prvFspiInitAhb();

	/* RE-Enable Controller Module */
	prvFspiDisableModule( 0 );
	/* COnfigure LUT after enabling the controller */
	prvFspiSetupLut();

	/* Dump of all registers, ensure controller not disabled anymore*/

#ifdef DEBUG_FLEXSPI
	prvFspiDumpRegisters();
#endif

	log_dbg( "\nFlexspi: Init done!! \n\r" );
	return FSPI_SUCCESS;
}
