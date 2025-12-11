// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2024 NXP
 */

#include <fsl_dspi.h>
#include <config.h>
#include "mpic.h"
#include "Time.h"

struct LA12xxDspiInstance * pxDspiHandle[ DSPI_MAX_BLOCK ] = { NULL };

void vChkTxFifoFull( struct LA12xxDspiInstance * xDspiHandle )
{
    uint32_t ulSrVal;
    while( 1 )
    {
        ulSrVal = in_dspile32( &xDspiHandle->DspiRegs->ulSr );

        if( ulSrVal & DSPI_SR_TFFF )
        {
            break;
        }
	else
	{
		for(int ctr = 0; ctr < 10; ctr++)
			;
	}
    }
}


void vWaitFifo( struct LA12xxDspiInstance * xDspiHandle )
{
    uint32_t ulSrVal;

    while( 1 )
    {
        ulSrVal = in_dspile32( &xDspiHandle->DspiRegs->ulSr );

        if( ulSrVal & DSPI_SR_TCF )
        {
            out_le32( &xDspiHandle->DspiRegs->ulSr, ulSrVal | DSPI_SR_TCF );
            break;
        }
    }
}

void vDspiHalt( struct LA12xxDspiInstance * xDspiHandle,
                uint8_t ucHalt )
{
    uint32_t ulMcrVal;

    ulMcrVal = in_dspile32( &xDspiHandle->DspiRegs->ulMcr );

    if( ucHalt )
    {
        ulMcrVal |= DSPI_MCR_HALT;
    }
    else
    {
        ulMcrVal &= ( uint32_t ) ( ~DSPI_MCR_HALT );
    }

    out_le32( &xDspiHandle->DspiRegs->ulMcr, ulMcrVal );
}

void vDspiFslClearFifo( struct LA12xxDspiInstance * xDspiHandle )
{
    uint32_t ulMcrVal;

    vDspiHalt( xDspiHandle, 1 );
    ulMcrVal = in_dspile32( &xDspiHandle->DspiRegs->ulMcr );
    /* flush RX and TX FIFO */
    ulMcrVal |= ( DSPI_MCR_CTXF | DSPI_MCR_CRXF );
    out_le32( &xDspiHandle->DspiRegs->ulMcr, ulMcrVal );
    vDspiHalt( xDspiHandle, 0 );
}

int8_t lDspiClaimBus( struct LA12xxDspiInstance * xDspiHandle )
{
    uint32_t ulSrVal, retry = 10;
    int8_t ret = -1;
    log_dbg("====DSPI Control Regs====\r\n");
    vDspiFslClearFifo( xDspiHandle );

    log_dbg( "MCR    %x\r\n", in_le32( &xDspiHandle->DspiRegs->ulMcr ) );
    log_dbg( "TCR    %x\r\n", in_le32( &xDspiHandle->DspiRegs->ulTcr ) );
    log_dbg( "CTAR0  %x\r\n", in_le32( &xDspiHandle->DspiRegs->ulCtar[ 0 ] ) );
    log_dbg( "CTAR1  %x\r\n", in_le32( &xDspiHandle->DspiRegs->ulCtar[ 1 ] ) );
    log_dbg( "SR     %x\r\n", in_le32( &xDspiHandle->DspiRegs->ulSr ) );
    log_dbg( "RSER   %x\r\n", in_le32( &xDspiHandle->DspiRegs->ulIrsr ) );
    log_dbg( "CTARE0 %x\r\n", in_le32( &xDspiHandle->DspiRegs->ulCtarX[ 0 ] ) );
    log_dbg( "CTARE1 %x\r\n", in_le32( &xDspiHandle->DspiRegs->ulCtarX[ 1 ] ) );

    /*Check module TX and RX status */
    while( retry )
    {
        ulSrVal = in_dspile32( &xDspiHandle->DspiRegs->ulSr );

        if( ( ulSrVal & DSPI_SR_TXRXS ) == DSPI_SR_TXRXS )
        {
            ret = 0;
            break;
        }

        retry--;
        log_err( " DSPI RX/TX not ready! SR[%x]\r\n", ulSrVal );
    }

    return ret;
}

int32_t lDspiPush( struct LA12xxDspiInstance * pxDspiHandle,
                   DspiChipSel_t eChipSelect,
                   DspiOps_t eOps,
                   uint8_t * pucData,
                   uint32_t ulLen )
{
    struct DspiReg * pxReg = pxDspiHandle->DspiRegs;
    uint32_t ulCmdData, count;

    if( ulLen & 0x3 )
    {
        return DSPI_INCORRECT_LEN;
    }

    if( DSPI_SR_TXCTR( in_dspile32( &pxReg->ulSr ) ) >= DSPI_TX_FIFO_SIZE - 1 )
    {
        return DSPI_TXFIFO_FULL;
    }

    /* Send data in chunk of 4 bytes */
    for( count = 0; count < ulLen / 4; count += 1 )
    {
        /* Select chip select */
        ulCmdData = DSPI_TFR_CS( eChipSelect );
        ulCmdData = ulCmdData | ( ( ( uint32_t ) ( *( pucData + 2 ) ) ) << 8 ) |
                    ( ( uint32_t ) ( *( pucData + 3 ) ) );
        out_le32( &pxReg->ulTfr, ulCmdData );

        ulCmdData = DSPI_TFR_CS( eChipSelect );
        ulCmdData = ulCmdData | ( ( ( uint32_t ) ( *pucData ) ) << 8 ) |
                    ( ( uint32_t ) ( *( pucData + 1 ) ) );

        /* last chunk of this transfer */
        if( count == ( ( ulLen / 4 ) - 1 ) )
        {
            ulCmdData |= DSPI_TFR_EOQ;
        }

        out_le32( &pxReg->ulTfr, ulCmdData );
        pucData += 4;

        /* Wait for completion */
        vWaitFifo( pxDspiHandle );

        /* Clear Tx FIFO */
        ulCmdData = in_le32( &pxReg->ulMcr );
        out_le32( &pxReg->ulMcr, ulCmdData | DSPI_MCR_CTXF );

        /* Clearing the Rx fifo after Write */
        if( DSPI_DEV_WRITE == eOps )
        {
            ulCmdData = in_le32( &pxReg->ulMcr );
            out_le32( &pxReg->ulMcr, ulCmdData | DSPI_MCR_CRXF );
        }
    }

    return DSPI_WRITE_SUCCESS;
}

int32_t lDspiPop( struct LA12xxDspiInstance * pxDspiHandle,
                  uint8_t * pucData,
                  uint32_t ulLen )
{
    uint32_t ulRegData;
    uint8_t ucAvailData, count;

    ulRegData = in_dspile32( &pxDspiHandle->DspiRegs->ulSr );

    if( ulRegData & DSPI_SR_SPEF )
    {
        out_le32( &pxDspiHandle->DspiRegs->ulSr, DSPI_SR_SPEF | ulRegData );
        return DSPI_ERROR_PARITY;
    }

    if( ulRegData & DSPI_SR_RFOF )
    {
        out_le32( &pxDspiHandle->DspiRegs->ulSr, DSPI_SR_RFOF | ulRegData );
        return DSPI_ERR_RX_OVERFLOW;
    }

    if( 0 != ( ulLen % 4 ) )
    {
        return DSPI_INCORRECT_LEN;
    }

    ucAvailData = ( ( ( ulRegData & DSPI_SR_COUNT_RX ) >> 4 ) * 4 );

    if( ucAvailData < ulLen )
    {
        return DSPI_NOT_ENOUGH_DATA;
    }

    for( count = 0; count < ulLen / 4; count += 1 )
    {
        ulRegData = in_dspile32( &pxDspiHandle->DspiRegs->ulRfr );

        *( pucData + count ) = ( ( ulRegData >> 24 ) & 0xFF );
        *( pucData + count + 1 ) = ( ( ulRegData >> 16 ) & 0xFF );
        *( pucData + count + 2 ) = ( ( ulRegData >> 8 ) & 0xFF );
        *( pucData + count + 3 ) = ( ( ulRegData ) & 0xFF );
    }

    return DSPI_READ_SUCCESS;
}

int32_t lPushR( struct LA12xxDspiInstance * xDspiHandle,
               uint16_t usDataPush )
{
    uint32_t ulCmdData = DSPI_TFR_CS0;

    if( DSPI_SR_TXCTR( in_dspile32( &xDspiHandle->DspiRegs->ulSr ) ) >= DSPI_TX_FIFO_SIZE )
    {
        log_err( "DSPI : TX FIFO is already full \r\n" );
        return DSPI_TXFIFO_FULL;
    }
    out_le32( &xDspiHandle->DspiRegs->ulTfr, ulCmdData );
    out_le32( &xDspiHandle->DspiRegs->ulTfr, ( ulCmdData | ( uint32_t )usDataPush ));
    vWaitFifo( xDspiHandle );
    return DSPI_WRITE_SUCCESS;
}

int32_t lPushW( struct LA12xxDspiInstance * xDspiHandle,
               uint16_t usAddrPush, uint8_t ucData )
{
    uint32_t ulCmdData = DSPI_TFR_CS0;
    uint32_t ulMcr;

    if( DSPI_SR_TXCTR( in_dspile32( &xDspiHandle->DspiRegs->ulSr ) ) >= DSPI_TX_FIFO_SIZE )
    {
        log_err( "DSPI : TX FIFO is already full \r\n" );
        return DSPI_TXFIFO_FULL;
    }
    out_le32( &xDspiHandle->DspiRegs->ulTfr, ( ulCmdData | ( ( uint32_t ) ( ( uint16_t )ucData ) << 8 ) ) );
    out_le32( &xDspiHandle->DspiRegs->ulTfr, ( ulCmdData | usAddrPush | DSPI_TFR_EOQ ) );
    vWaitFifo( xDspiHandle );
    /* Clearing the Rx fifo after Write */
    ulMcr = in_le32( &xDspiHandle->DspiRegs->ulMcr);
    out_le32( &xDspiHandle->DspiRegs->ulMcr, ulMcr | DSPI_MCR_CRXF );

    return DSPI_WRITE_SUCCESS;
}


int32_t lPop( struct LA12xxDspiInstance * xDspiHandle, uint32_t *ulData )
{
    if( ( in_dspile32( &xDspiHandle->DspiRegs->ulSr ) & DSPI_SR_COUNT_RX ) == 0 )
    {
	log_err( "DSPI Read Error : Rx FIFO is empty \r\n" );
	return DSPI_RXFIFO_EMPTY;
    }
    /*Shifting the data received to remove trailing zeros*/
    *ulData = ( in_dspile32( &xDspiHandle->DspiRegs->ulRfr ) >> 5 );
    return DSPI_READ_SUCCESS;
}

struct LA12xxDspiInstance * pxDspiInit( DspiBlock_t eDspiBlock,
                                        uint8_t ucCsMask )
{
    uint32_t ulMcrCfgVal;
    uint32_t ulCtarCfgVal;
    uint32_t ulSrCfgVal;
    uint32_t ulIrsrCfgVal;
    uint32_t ulCtarXVal;


    if( eDspiBlock > DSPI_BLOCK6 )
    {
        log_err( "DSPI Err :  Invalid DSPI block number %d\r\n",eDspiBlock );
        return NULL;
    }

    if( pxDspiHandle[ eDspiBlock ] != NULL )
    {
        return pxDspiHandle[ eDspiBlock ];
    }

    pxDspiHandle[ eDspiBlock ] = ( struct LA12xxDspiInstance * ) pvGeulMalloc( sizeof( struct LA12xxDspiInstance ) );

    if( pxDspiHandle[ eDspiBlock ] == NULL )
    {
        log_err( "DSPI Handle allocation failed!! \r\n" );
        return pxDspiHandle[ eDspiBlock ];
    }

    pxDspiHandle[ eDspiBlock ]->DspiRegs = ( struct DspiReg * ) ( DSPI_REG_BASE_ADDRESS
                                                                  + ( ( eDspiBlock ) * DSPI_REG_BASE_OFFSET ) );

    pxDspiHandle[ eDspiBlock ]->eBlockNumber = eDspiBlock;
    pxDspiHandle[ eDspiBlock ]->ucCsMask = ucCsMask;

    /* frame data length in bits, default 16 bits */
    /* default: all CS signals inactive state is high */
    ulMcrCfgVal = ( DSPI_MCR_MSTR | DSPI_MCR_PCSIS( ucCsMask ) |
                    DSPI_MCR_CTXF | DSPI_MCR_CRXF | DSPI_MCR_XSPI |
                    DSPI_MCR_DTXF | DSPI_MCR_DRXF | DSPI_MCR_HALT );
    ulCtarCfgVal = ( uint32_t ) DSPI_CTAR_TRSZ( 0xf );
    ulCtarXVal = DSPI_CTAR_X_TRSZ | DSPI_CTAR_X_DTCP( 0x1 );
    ulSrCfgVal = ( DSPI_SR_TCF | DSPI_SR_EOQF | DSPI_SR_TFFF |
                   DSPI_SR_RFOF | DSPI_SR_RFDF | DSPI_SR_TFIWF |
                   DSPI_SR_SPEF | DSPI_SR_CTCF );
    ulIrsrCfgVal = DSPI_IRSR_DISABLE;

    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulMcr, ulMcrCfgVal );
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulCtar[ 0 ], ulCtarCfgVal );
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulCtarX[ 0 ], ulCtarXVal );
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulSr, ulSrCfgVal );
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulIrsr, ulIrsrCfgVal );

    /*Enabling the Tx and RxFifo*/
    ulMcrCfgVal = ( ulMcrCfgVal & DSPI_MCR_TXRX_ENABLE );
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulMcr, ulMcrCfgVal );

    /*Setting Default frequency 8MHz*/
    vDspiClkSet( pxDspiHandle[ eDspiBlock ], DSPI_DEFAULT_FREQUENCY );

    /* remove DSPI halt */
    vDspiHalt( pxDspiHandle[ eDspiBlock ], 0 );

    if( lDspiClaimBus( pxDspiHandle[ eDspiBlock ] ) < 0 )
    {
        /* Release memory for DSPI handler */
        vGeulFree( pxDspiHandle[ eDspiBlock ] );
        pxDspiHandle[ eDspiBlock ] = NULL;
        return NULL;
    }

    return pxDspiHandle[ eDspiBlock ];
}

#ifdef ENABLE_DSPI_STREAM_MODE
struct LA12xxDspiInstance * pxDspiInitStream( DspiBlock_t eDspiBlock,
                                        uint8_t ucCsMask )
{
    uint32_t ulMcrCfgVal;
    uint32_t ulCtarCfgVal;
    uint32_t ulSrCfgVal;
    uint32_t ulIrsrCfgVal;
    uint32_t ulCtarXVal;


    log_dbg("pxDspiInit eDspiBlock %d SPI Clock %d Hz \r\n",eDspiBlock,DSPI_FREQUENCY_FR1_FR2);

    if( eDspiBlock > DSPI_BLOCK6 )
    {
        log_err( "DSPI Err :  Invalid DSPI block number %d\r\n",eDspiBlock);
        return NULL;
    }

    if( pxDspiHandle[ eDspiBlock ] != NULL )
    {
        return pxDspiHandle[ eDspiBlock ];
    }

    pxDspiHandle[ eDspiBlock ] = ( struct LA12xxDspiInstance * ) pvGeulMalloc( sizeof( struct LA12xxDspiInstance ) );

    if( pxDspiHandle[ eDspiBlock ] == NULL )
    {
        log_err( "DSPI Handle allocation failed!! \r\n" );
        return pxDspiHandle[ eDspiBlock ];
    }

    pxDspiHandle[ eDspiBlock ]->DspiRegs = ( struct DspiReg * ) ( DSPI_REG_BASE_ADDRESS
                                                                  + ( ( eDspiBlock ) * DSPI_REG_BASE_OFFSET ) );

    pxDspiHandle[ eDspiBlock ]->eBlockNumber = eDspiBlock;
    pxDspiHandle[ eDspiBlock ]->ucCsMask = ucCsMask;

    /* frame data length in bits, default 16 bits */
    /* default: all CS signals inactive state is high */
    //ulMcrCfgVal = ( DSPI_MCR_MSTR | DSPI_MCR_CSCK | DSPI_MCR_PCSIS( ucCsMask ) |
    ulMcrCfgVal = ( DSPI_MCR_MSTR | DSPI_MCR_PCSIS( ucCsMask ) |
                    DSPI_MCR_CTXF | DSPI_MCR_CRXF |
                    DSPI_MCR_DTXF | DSPI_MCR_DRXF | DSPI_MCR_FCPCS | DSPI_MCR_HALT );
    /*Setting frequency 50 MHz*/
    ulCtarCfgVal = ( uint32_t )  DSPI_CTAR_CPHA | DSPI_CTAR_PCSSCK( 2 ) |  DSPI_CTAR_PASC( 2 ) | DSPI_CTAR_PDT( 0 )  | DSPI_CTAR_PBR( 1 ) | DSPI_CTAR_CSSCK( 1 ) | DSPI_CTAR_ASC( 1 ) | DSPI_CTAR_DT( 1 ) | DSPI_CTAR_BR(0);
    ulCtarXVal =  DSPI_CTAR_X_DTCP( 0x1 );
    ulSrCfgVal = ( DSPI_SR_TCF | DSPI_SR_EOQF | DSPI_SR_TFFF |
                   DSPI_SR_RFOF | DSPI_SR_RFDF | DSPI_SR_TFIWF |
                   DSPI_SR_SPEF | DSPI_SR_CTCF );
    ulIrsrCfgVal = DSPI_IRSR_DISABLE;

    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulMcr, ulMcrCfgVal );
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulCtar[ 0 ], ulCtarCfgVal | DSPI_CTAR_TRSZ( FRAME_SIZE_14 ));
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulCtar[ 1 ], ulCtarCfgVal | DSPI_CTAR_TRSZ( FRAME_SIZE_14 ));
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulCtarX[ 0 ], ulCtarXVal );
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulCtarX[ 1 ], ulCtarXVal );
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulSr, ulSrCfgVal );
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulIrsr, ulIrsrCfgVal );

    /*Enabling the Tx and RxFifo*/
    ulMcrCfgVal = ( ulMcrCfgVal & DSPI_MCR_TXRX_ENABLE );
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulMcr, ulMcrCfgVal );

    /* remove DSPI halt */
    vDspiHalt( pxDspiHandle[ eDspiBlock ], 0 );

    if( lDspiClaimBus( pxDspiHandle[ eDspiBlock ] ) < 0 )
    {
        /* Release memory for DSPI handler */
        vGeulFree( pxDspiHandle[ eDspiBlock ] );
        pxDspiHandle[ eDspiBlock ] = NULL;
        return NULL;
    }

    return pxDspiHandle[ eDspiBlock ];
}

int32_t lPushStreamByte( struct LA12xxDspiInstance * xDspiHandle,
               uint16_t *data_ptr, uint8_t num_transfer )
{
	uint32_t ulCmdData = DSPI_TFR_CS0;
	uint32_t ulMcr;
	uint8_t uCtr = 0;

	if( DSPI_SR_TXCTR( in_dspile32( &xDspiHandle->DspiRegs->ulSr ) ) >= DSPI_TX_FIFO_SIZE )
	{
		log_err( "DSPI : TX FIFO is already full \r\n" );
		return DSPI_TXFIFO_FULL;
	}
	/*Send first transfer without DSPI_TFR_CONT set */
	out_le32( &xDspiHandle->DspiRegs->ulTfr, ( ulCmdData | DSPI_TFR_PP_MCSC | DSPI_TFR_PE_MASC | data_ptr[uCtr]));
	/*Send all the transfer , other than last with DSPI_TFR_CONT set to avoid CS going high*/
	for( uCtr = 1; uCtr < (num_transfer - 1); uCtr++) {
		out_le32( &xDspiHandle->DspiRegs->ulTfr, ( ulCmdData | DSPI_TFR_CONT | DSPI_TFR_PP_MCSC | DSPI_TFR_PE_MASC | data_ptr[uCtr]));
		vChkTxFifoFull( xDspiHandle );
	}
	vChkTxFifoFull( xDspiHandle );
	/*Send last transfer with DSPI_TFR_EOQ set to mark end of transfer*/
	out_le32( &xDspiHandle->DspiRegs->ulTfr, ( ulCmdData | DSPI_TFR_CTAS( 0 )  | DSPI_TFR_EOQ  | data_ptr[uCtr]));

	vWaitFifo( xDspiHandle );
	/* Clearing the Rx fifo after Write */
	ulMcr = in_le32( &xDspiHandle->DspiRegs->ulMcr);
	out_le32( &xDspiHandle->DspiRegs->ulMcr, ulMcr | DSPI_MCR_CRXF );

	return DSPI_WRITE_SUCCESS;
}

struct LA12xxDspiInstance * getDspiHandle(DspiBlock_t eBlock, DspiChipSel_t eCS_select)
{

	if(pxDspiHandle[eBlock] == NULL)
	{
		log_info("\r\nDSPI_BLOCK%d CS %d:\r\n", eBlock,eCS_select);
		pxDspiHandle[eBlock] = pxDspiInitStream( eBlock , 1 << eCS_select);
	}
	return pxDspiHandle[eBlock];
}

int32_t dspi_write_stream(DspiBlock_t eBlock, DspiChipSel_t eCS_select, uint16_t *data_ptr, uint8_t num_transfer)
{
	int32_t ret=0;
	struct LA12xxDspiInstance * xDspiHandle = getDspiHandle(eBlock,eCS_select);

	ret = lPushStreamByte( xDspiHandle, data_ptr, num_transfer);

	return ret;
}
#endif //ENABLE_DSPI_STREAM_MODE

struct LA12xxDspiInstance * pxDspiInitCustom(
		struct DspiCustom *xDspiInit)
{
    DspiBlock_t eDspiBlock = xDspiInit->eBlockNumber;

    uint8_t ucCsMask = xDspiInit->ucCsMask;

    if( eDspiBlock > DSPI_BLOCK6 )
    {
        log_err( "DSPI Err :  Invalid DSPI block number \r\n" );
        return NULL;
    }

    if( pxDspiHandle[ eDspiBlock ] != NULL )
    {
        return pxDspiHandle[ eDspiBlock ];
    }

    pxDspiHandle[ eDspiBlock ] = ( struct LA12xxDspiInstance * ) pvGeulMalloc( sizeof( struct LA12xxDspiInstance ) );

    if( pxDspiHandle[ eDspiBlock ] == NULL )
    {
        log_err( "DSPI Handle allocation failed!! \r\n" );
        return pxDspiHandle[ eDspiBlock ];
    }

    pxDspiHandle[ eDspiBlock ]->DspiRegs = ( struct DspiReg * ) ( DSPI_REG_BASE_ADDRESS
                                + ( ( eDspiBlock ) * DSPI_REG_BASE_OFFSET ) );

    pxDspiHandle[ eDspiBlock ]->eBlockNumber = eDspiBlock;
    pxDspiHandle[ eDspiBlock ]->ucCsMask = ucCsMask;

    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulMcr,
		    xDspiInit->DspiRegs.ulMcr );
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulCtar[ 0 ],
		    xDspiInit->DspiRegs.ulCtar[0]);
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulSr,
		    xDspiInit->DspiRegs.ulSr );

    /* Set this ulCtar1 as 1 during fillDspi to use Ctar[1] */
    if (xDspiInit->ulCtar1) {
		out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulCtar[ 1 ],
				    xDspiInit->DspiRegs.ulCtar[ 1 ]);
    }

    /* The XSPI need to be set in the MCR to use the Extended mode */
    if ((xDspiInit->DspiRegs.ulMcr & DSPI_MCR_XSPI) == DSPI_MCR_XSPI) {
		out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulCtarX[ 0 ],
				xDspiInit->DspiRegs.ulCtarX[ 0 ] );
		/* Set this ulCtarX1 as 1 during fillDspi to use CtarX[1] */
		if (xDspiInit->ulCtarX1) {
			out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulCtarX[ 1 ],
						xDspiInit->DspiRegs.ulCtarX[ 1 ]);
		}
    }

    /* To disable interrupt request */
    if ((xDspiInit->DspiRegs.ulIrsr | DSPI_IRSR_DISABLE) == DSPI_IRSR_DISABLE ) {
		out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulIrsr,
				xDspiInit->DspiRegs.ulIrsr );
    }

    /* remove DSPI halt */
    vDspiHalt( pxDspiHandle[ eDspiBlock ], 0 );
    if( lDspiClaimBus( pxDspiHandle[ eDspiBlock ] ) < 0 )
    {
        /* Release memory for DSPI handler */
        vGeulFree( pxDspiHandle[ eDspiBlock ] );
        pxDspiHandle[ eDspiBlock ] = NULL;
        return NULL;
    }

    return pxDspiHandle[ eDspiBlock ];
}

void vDspiExit( DspiBlock_t eBlock )
{
    vGeulFree( pxDspiHandle[ eBlock ] );
    pxDspiHandle[ eBlock ] = NULL;
}

int32_t lDspiHzToBaud( uint32_t * lpbr,
		uint32_t * lbr,
		uint32_t ulSpeedHz,
		uint32_t ulBusClk )
{
	uint32_t lPbrTbl[ 4 ] = { 2, 3, 5, 7 };
	uint32_t lBrTbl[ 16 ] =
	{
		2,    4,    6,     8,
		16,   32,   64,    128,
		256,  512,  1024,  2048,
		4096, 8192, 16384, 32768
	};
	int lScaleNeeded, lScale, lMinScale = INT_MAX;
	uint32_t i = 0, j = 0;

	lScaleNeeded = ulBusClk / ulSpeedHz;
	if(  ulBusClk % ulSpeedHz )
		lScaleNeeded++;

	for( i = 0; i < ARRAY_SIZE( lBrTbl ); i++ )
		for( j = 0; j < ARRAY_SIZE( lPbrTbl ); j++ )
		{
			lScale = lBrTbl[i] * lPbrTbl[j];
			if( lScale >= lScaleNeeded )
			{
				if( lScale < lMinScale )
				{
					lMinScale = lScale;
					*lbr = i;
					*lpbr = j;
				}
				break;
			}
		}

	if( lMinScale == INT_MAX )
	{
		log_err( "Can not find valid baud rate,speed_hz is %d,clkrate is %ld, we use the max prescaler value.\n",
				ulSpeedHz, ulBusClk );
		*lpbr = ARRAY_SIZE( lPbrTbl ) - 1;
		*lbr =  ARRAY_SIZE( lBrTbl ) - 1;
		return DSPI_CLKSET_ERROR;
	}
	return 0;
}

void vDspiClkSet( struct LA12xxDspiInstance * xDspiHandle,
                  uint32_t ulSpeed )
{
    int32_t lret;
    uint32_t lBesti, lBestj;
    uint32_t ulBusSetup, ulBusClk;

    ulBusClk = DSPI_INPUT_CLK_FREQUENCY;
    log_dbg( "DSPI Clock Set: Expected speed:%u  Bus clock:%u \r\n", ulSpeed, ulBusClk );
    ulBusSetup = in_dspile32( &xDspiHandle->DspiRegs->ulCtar[ 0 ] );
    ulBusSetup = ulBusSetup & ( ~( DSPI_CTAR_DBR | DSPI_CTAR_PBR( 0x3 ) | DSPI_CTAR_BR( 0xf ) ) );
    lret = lDspiHzToBaud( &lBesti, &lBestj, ulSpeed, ulBusClk );

    if( lret )
    {
        ulSpeed = DSPI_DEFAULT_FREQUENCY;
        log_err( "DSPI :setting failed,setting default speed:%u \r\n",ulSpeed );
        lret = lDspiHzToBaud( &lBesti, &lBestj, ulSpeed, ulBusClk );
    }

    ulBusSetup |= ( DSPI_CTAR_PBR( lBesti ) | DSPI_CTAR_BR( lBestj ) );
    out_le32( &xDspiHandle->DspiRegs->ulCtar[ 0 ], ulBusSetup );
    xDspiHandle->ulBusClk = ulSpeed;
}

#ifdef ENABLE_NORMAL_SPI_MODE
struct LA12xxDspiInstance * pxDspiInitNormalSPIMode( DspiBlock_t eDspiBlock, uint8_t ucCsMask )
{
    uint32_t ulMcrCfgVal;
    uint32_t ulCtarCfgVal;
    uint32_t ulSrCfgVal;


    if( eDspiBlock > DSPI_BLOCK6 )
    {
        log_err( "DSPI Err :  Invalid DSPI block number \r\n" );
        return NULL;
    }

    if( pxDspiHandle[ eDspiBlock ] != NULL )
    {
        return pxDspiHandle[ eDspiBlock ];
    }

    pxDspiHandle[ eDspiBlock ] = ( struct LA12xxDspiInstance * ) pvGeulMalloc( sizeof( struct LA12xxDspiInstance ) );

    if( pxDspiHandle[ eDspiBlock ] == NULL )
    {
        log_err( "DSPI Handle allocation failed!! \r\n" );
        return pxDspiHandle[ eDspiBlock ];
    }

    pxDspiHandle[ eDspiBlock ]->DspiRegs = ( struct DspiReg * ) ( DSPI_REG_BASE_ADDRESS
                                                                  + ( ( eDspiBlock ) * DSPI_REG_BASE_OFFSET ) );

    pxDspiHandle[ eDspiBlock ]->eBlockNumber = eDspiBlock;
    pxDspiHandle[ eDspiBlock ]->ucCsMask = ucCsMask;

    /* frame data length in bits, default 16 bits */
    /* default: all CS signals inactive state is high */
    vDspiClkSet( pxDspiHandle[ eDspiBlock ], DSPI_DEFAULT_FREQUENCY );
    ulMcrCfgVal = ( DSPI_MCR_MSTR | DSPI_MCR_HALT );
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulMcr, ulMcrCfgVal );

    ulMcrCfgVal = ( DSPI_MCR_MSTR | DSPI_MCR_PCSIS( ucCsMask ) |
                     DSPI_MCR_HALT );
    /*Setting Default frequency 4MHz*/
    ulCtarCfgVal= in_dspile32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulCtar[ 0 ] );
    ulCtarCfgVal &= DSPI_CTAR_TRSZ_MASK;
    ulCtarCfgVal |= ( uint32_t ) DSPI_CTAR_TRSZ( 0x7 );
    ulSrCfgVal = ( DSPI_SR_TCF | DSPI_SR_EOQF | DSPI_SR_TFFF |
                   DSPI_SR_RFOF | DSPI_SR_RFDF | DSPI_SR_TFIWF |
                   DSPI_SR_SPEF | DSPI_SR_CTCF );
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulMcr, ulMcrCfgVal );
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulCtar[ 0 ], ulCtarCfgVal );
    out_le32( &pxDspiHandle[ eDspiBlock ]->DspiRegs->ulSr, ulSrCfgVal );

    /* remove DSPI halt */
    vDspiHalt( pxDspiHandle[ eDspiBlock ], 0 );
    if( lDspiClaimBus( pxDspiHandle[ eDspiBlock ] ) < 0 )
    {
        /* Release memory for DSPI handler */
        vGeulFree( pxDspiHandle[ eDspiBlock ] );
        pxDspiHandle[ eDspiBlock ] = NULL;
        return NULL;
    }

    return pxDspiHandle[ eDspiBlock ];
}

static u32 ulDspiPopTxNormalSPIMode( struct NormalSPIModeDspiMsg * pxMsg )
{
    u32 ulTxData = 0;

    if (pxMsg->tx) {
        if ( pxMsg->ucBytesPerWord == 1 ) {
            ulTxData = *(u8 *)pxMsg->tx;
            pxMsg->tx = (u8 *)(pxMsg->tx) + pxMsg->ucBytesPerWord;
        } else {
            ulTxData = *(u16 *)pxMsg->tx;
            pxMsg->tx = (u16 *)(pxMsg->tx) + pxMsg->ucBytesPerWord;
        }
    }
    pxMsg->ulTxLen -= pxMsg->ucBytesPerWord;

    return ulTxData;
}

static void vDspiPushRxNormalSPIMode( struct NormalSPIModeDspiMsg * pxMsg, u32 ulReadData )
{
    if ( !pxMsg->rx ) {
        return;
    }

    ulReadData &= (1 << (pxMsg->ucBytesPerWord * BITSPERBYTE)) - 1;
    if ( pxMsg->ucBytesPerWord == 1 ) {
        *(u8 *)pxMsg->rx = ulReadData;
        pxMsg->rx = (u8 *)(pxMsg->rx) + pxMsg->ucBytesPerWord;
    } else {
        *(u16 *)pxMsg->rx = ulReadData;
        pxMsg->rx = (u16 *)(pxMsg->rx) + pxMsg->ucBytesPerWord;
    }
}

static void vFifoWriteNormalSPIMode( struct LA12xxDspiInstance * pxDspiHandle, struct NormalSPIModeDspiMsg * pxMsg )
{
    u32 ulCmdData = pxMsg->ulCmd;

    ulCmdData |= DSPI_TFR_CTCNT;
    if (pxMsg->ulTxLen > 1)
        ulCmdData |= DSPI_TFR_CONT;
    ulCmdData |= ulDspiPopTxNormalSPIMode( pxMsg );
    out_le32( &pxDspiHandle->DspiRegs->ulTfr, ulCmdData );
}

static void vFifoReadNormalSPIMode( struct LA12xxDspiInstance * pxDspiHandle, struct NormalSPIModeDspiMsg * pxMsg )
{
    u32 ulReadData = 0x0;

    /* Wait for completion */
    vWaitFifo( pxDspiHandle );
    ulReadData = in_dspile32( &pxDspiHandle->DspiRegs->ulRfr );
    vDspiPushRxNormalSPIMode( pxMsg, ulReadData );
}

int iTransferMsgNormalSPIMode( struct LA12xxDspiInstance * pxDspiHandle, DspiChipSel_t eChipSelect,
                        uint32_t ulLen, void *tx, void *rx )
{
    struct NormalSPIModeDspiMsg xMsg;
    uint32_t i, ulMcrCfgVal;
    struct DspiReg * pxReg = pxDspiHandle->DspiRegs;

    xMsg.ulTxLen = ulLen;
    xMsg.ulRxLen = ulLen;
    xMsg.tx = tx;
    xMsg.rx = rx;
    xMsg.ucBytesPerWord = 1;
    xMsg.ulCmd = DSPI_TFR_CS( eChipSelect ) | DSPI_TFR_CTAS(0);

    ulMcrCfgVal= in_dspile32( &pxReg->ulMcr );
    ulMcrCfgVal |= ( DSPI_MCR_CTXF | DSPI_MCR_CRXF );
    out_le32( &pxReg->ulMcr, ulMcrCfgVal );

    for( i = 0; i < (ulLen / xMsg.ucBytesPerWord); i++ )
    {
       vFifoWriteNormalSPIMode( pxDspiHandle, &xMsg );
       vFifoReadNormalSPIMode( pxDspiHandle, &xMsg );
    }

    return DSPI_SUCCESS;
}
#endif
