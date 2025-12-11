// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2024 NXP
 */

#ifndef _DSPI_H_
#define _DSPI_H_

/**
 * @file        fsl_dspi.h
 * @brief       This file contains the DSPI-related information
 * @addtogroup  DSPI_API
 * @{
 */

#include <types.h>
#include <bit.h>
#include "common.h"
#include <platform_def.h>

#define ARRAY_SIZE( x )    ( sizeof( x ) / sizeof( ( x )[ 0 ] ) )
/**
 *max chipselect signals number
 *
 */
#define DSPI_REG_BASE_ADDRESS    CCSR_BASE_ADDR + 0x1170000
#define DSPI_REG_BASE_OFFSET     0x4000

#define INT_MAX         ((int)(~0U>>1))

/**
 * @brief Enum for different DSPI Module
 */
typedef enum DspiBlock
{
    DSPI_BLOCK1 = 0,
    DSPI_BLOCK2,
    DSPI_BLOCK3,
    DSPI_BLOCK4,
    DSPI_BLOCK5,
    DSPI_BLOCK6,
    DSPI_MAX_BLOCK,
} DspiBlock_t;

typedef enum DspiChipSel
{
    DSPI_CS0,
    DSPI_CS1,
    DSPI_CS2,
    DSPI_CS3
}DspiChipSel_t;

typedef enum DspiOps
{
    DSPI_DEV_READ,
    DSPI_DEV_WRITE
}DspiOps_t;

/*
 * DMA Serial Peripheral Interface (DSPI) register address
 *
 */
struct DspiReg
{
    uint32_t ulMcr;            /* 0x00 */
    uint32_t ulResv0;          /* 0x04 */
    uint32_t ulTcr;            /* 0x08 */
    uint32_t ulCtar[ 8 ];      /* 0x0C - 0x28 */
    uint32_t ulSr;             /* 0x2C */
    uint32_t ulIrsr;           /* 0x30 */
    uint32_t ulTfr;            /* 0x34 - PUSHR */
    uint32_t ulRfr;            /* 0x38 - POPR */
    uint32_t ulTfdr[ 16 ];     /* 0x3C */
    uint32_t ulRfdr[ 40 ];     /* 0x7C */
    uint32_t ulCtarX[ 2 ];     /* 0x11c */
    uint32_t resv[6];
    uint32_t ulSrX;
};

struct DspiCustom
{
	uint32_t ulBusClk;
	DspiBlock_t eBlockNumber;
	uint8_t ucCsMask;
	struct DspiReg DspiRegs;
	bool ulCtar1;
	bool ulCtarX1;
};

/**
 * @brief This function is used to initialize the DSPI instance
 * @param[in] DspiBlock the DSPI line which must be initialized
 * @param[in] ucCsMask is a chip select mask, bit[0:3] = CS[0:3]
 *
 * @return the DSPI instance structure or, NULL otherwise
 */
struct LA12xxDspiInstance * pxDspiInit( DspiBlock_t DspiBlock, uint8_t ucCsMask );
/*Custom dspi controller init function for stream mode support
 * This mode set SPI clock and various spi custom protocol delays specific for use case  
 * */ 
struct LA12xxDspiInstance * pxDspiInitStream( DspiBlock_t DspiBlock, uint8_t ucCsMask );
void vDspiExit( DspiBlock_t eBlock );

/*
 * @brief This function is used to initialize the DSPI instance with Custom Values
 * @param[in] DspiCustom the custom variable which contain all values to be
 * 		initialised
 * @return the DSPI instance structure or, NULL otherwise
 */
struct LA12xxDspiInstance * pxDspiInitCustom(struct DspiCustom *xDspiInit);
/**
 * @brief	Function is used to push the data into TX FIFO for read/write.
 * @param[in]	pxDspiHandle DSPI handler
 * @param[in]	eChipSelect DSPI chip select
 * @param[in]	eOps operation could be either read or write
 * @param[in]	pucData	data to be pushed.
 * @param[in]	ulLen length of data. This must be multiple of 4.
 *		Append zeros if required.
 * @return 0 on success, Error code otherwise.
 */
int32_t lDspiPush( struct LA12xxDspiInstance *pxDspiHandle, DspiChipSel_t eChipSelect,
		   DspiOps_t eOps, uint8_t *pucData, uint32_t ulLen);

/**
 * @brief       Function is used to pop the data from RX FIFO for read.
 * @param[in]   pxDspiHandle DSPI handler
 * @param[in]   pucData data pointer used to store popped data
 * @param[in]   ulLen length of expected data. This must be multiple of 4.
 *		Append zeros if required.
 * @return 0 on success, Error code otherwise.
 */
int32_t lDspiPop( struct LA12xxDspiInstance * pxDspiHandle, uint8_t *pucData,
		  uint32_t ulLen );

/**
 * @brief Internal function is used to push the data for Read.
 * @param[in]  xDspiHandle DSPI handler
 * @param[in]  usDataPush DSPI data to be pushed
 * @return 0 on success, Error code otherwise.
 */
int32_t lPushR( struct LA12xxDspiInstance * xDspiHandle,
               uint16_t usDataPush );
/**
 * @brief Internal function is used to push the data for Write.
 * @param[in]  xDspiHandle DSPI handler
 * @param[in]  usAddrPush DSPI data to be pushed
 * @param[in]  ucData data to be pushed
 * @return 0 on success, Error code otherwise.
 */
int32_t lPushW( struct LA12xxDspiInstance * xDspiHandle,
               uint16_t usAddrPush, uint8_t ucData );

int32_t lPushStreamByte( struct LA12xxDspiInstance * xDspiHandle,
               uint16_t *data_ptr, uint8_t num_transfer );

/**
 * @brief Internal function is used to Pop the data.
 * @param[in]  xDspiHandle DSPI handler
 * @param[in]  ulData pointer at which data will be popped
 * @return 0 on success, Error code otherwise.
 */
int32_t lPop( struct LA12xxDspiInstance * xDspiHandle, uint32_t * ulData );

/**
 * @brief Dspi function to read the register value.
 * @param[in]  xDspiHandle DSPI handler
 * @param[in]  ulSpeed clock Speed
 * @return  does not return anything it is a void
 */
void vDspiClkSet( struct LA12xxDspiInstance * xDspiHandle,
                  uint32_t ulSpeed );

#ifdef ENABLE_NORMAL_SPI_MODE
/**
 * @brief DSPI function to initialize the DSPI in Normal SPI Mode
 * @param[in]  DspiBlock
 *      DSPI line which must be initialized
 * @param[in]  ucCsMask
 *      Chip select mask, bit[0:3] = CS[0:3]
 *
 * @return the DSPI instance structure or, NULL otherwise
 */
struct LA12xxDspiInstance * pxDspiInitNormalSPIMode( DspiBlock_t DspiBlock, uint8_t ucCsMask );

/**
 * @brief DSPI function to read and write in Normal SPI Mode
 * @param[in]  xDspiHandle
 *       DSPI handler
 * @param[in]  eChipSelect
 *       Chip Select(DSPI_CS0)
 * @param[in]  ulLen
 *       Number of words to transfer or receive
 * @param[in]  tx
 *       Tx Buffer
 * @param[out]  rx
 *       Rx Buffer
 */
int iTransferMsgNormalSPIMode( struct LA12xxDspiInstance * pxDspiHandle, DspiChipSel_t eChipSelect,
                        uint32_t ulLen, void * tx, void * rx );
#endif

int32_t dspi_write_stream(DspiBlock_t eBlock, DspiChipSel_t eCS_select, uint16_t *data_ptr, uint8_t num_transfer);

#define DSPI_ENABLE_22MHZ	0

#define DSPI_TX_FIFO_SIZE           4

#define BITSPERBYTE                 8
#define FRAME_SIZE_8    0x7 //8 bit
#define FRAME_SIZE_14   0xD //14 bit
#define FRAME_SIZE_16   0xf //16 bit


/**
 * Error Codes for DSPI
 *
 */
#define DSPI_SUCCESS                0
#define DSPI_WRITE_SUCCESS          0
#define DSPI_READ_SUCCESS           0
#define DSPI_TXFIFO_FULL            -1
#define DSPI_RXFIFO_EMPTY           -2
#define DSPI_ERR_COUNT              -3
#define DSPI_ERROR_PARITY           -4
#define DSPI_ERR_RX_OVERFLOW        -5
#define DSPI_CLKSET_ERROR           -6
#define DSPI_READ_INVALID           -7
#define DSPI_INCORRECT_LEN	    -8
#define DSPI_NOT_ENOUGH_DATA	    -9
#define DSPI_WRITE_TIMEOUT          -10

#define DSPI_INPUT_CLK_FREQUENCY    PLAT_FREQ / 2

#if ( DSPI_ENABLE_22MHZ == 1 )
#define DSPI_DEFAULT_FREQUENCY      22000000
#else
#define DSPI_DEFAULT_FREQUENCY      4000000
#define DSPI_FREQUENCY_FR1_FR2      50000000
#endif

#define DSPI_VALID_BIT_MASK         0x00000100
#define DSPI_DATA_MASK              0x000000FF

/**
 * struct geul_dspi_instance - handle data for Freescale DSPI Controller
 *
 * flags Flags for DSPI DSPI_FLAG_...
 * mode SPI mode to use for slave device (see SPI mode flags)
 * mcr_val MCR register configure value
 * bus_clk DSPI input clk frequency
 * speed_hz Default SCK frequency
 * charbit How many bits in every transfer
 * num_chipselect Number of DSPI chipselect signals
 * ctar_val CTAR register configure value of per chipselect slave device
 * regs Point to DSPI register structure for I/O access
 */
struct LA12xxDspiInstance
{
    uint32_t ulBusClk;
    DspiBlock_t eBlockNumber;
    struct DspiReg * DspiRegs;
    uint8_t ucCsMask;
};

#ifdef ENABLE_NORMAL_SPI_MODE
/**
 * \struct NormalSPIModeDspiMsg
 * DSPI Normal SPI Mode Message Format
 */
struct NormalSPIModeDspiMsg
{
    uint8_t ucBytesPerWord;             /**< Bytes to transfer in One Frame */
    uint32_t ulCmd;                     /**< Cmd to send */
    uint32_t ulTxLen;                   /**< Tx Length */
    uint32_t ulRxLen;                   /**< Rx Length */
    void * tx;                          /**< Tx buffer */
    void * rx;                          /**< Rx buffer */
};
#endif

/**
 * Module configuration
 *
 */
#define DSPI_MCR_MSTR           0x80000000
#define DSPI_MCR_CSCK           0x40000000
#define DSPI_MCR_DCONF( x )    ( ( ( x ) & 0x03 ) << 28 )
#define DSPI_MCR_FRZ            0x08000000
#define DSPI_MCR_MTFE           0x04000000
#define DSPI_MCR_PCSSE          0x02000000
#define DSPI_MCR_ROOE           0x01000000
#define DSPI_MCR_PCSIS( x )	( ( ( x ) & 0xF ) << 16 )
#define DSPI_MCR_CSIS7          0x00800000
#define DSPI_MCR_CSIS6          0x00400000
#define DSPI_MCR_CSIS5          0x00200000
#define DSPI_MCR_CSIS4          0x00100000
#define DSPI_MCR_CSIS3          0x00080000
#define DSPI_MCR_CSIS2          0x00040000
#define DSPI_MCR_CSIS1          0x00020000
#define DSPI_MCR_CSIS0          0x00010000
#define DSPI_MCR_DOZE           0x00008000
#define DSPI_MCR_MDIS           0x00004000
#define DSPI_MCR_DTXF           0x00002000
#define DSPI_MCR_DRXF           0x00001000
#define DSPI_MCR_CTXF           0x00000800
#define DSPI_MCR_CRXF           0x00000400
#define DSPI_MCR_SMPL_PT( x )    ( ( ( x ) & 0x03 ) << 8 )
#define DSPI_MCR_FCPCS          0x00000004
#define DSPI_MCR_PES            0x00000002
#define DSPI_MCR_HALT           0x00000001
#define DSPI_MCR_TXRX_ENABLE    0xffffcfff
#define DSPI_MCR_XSPI           0x00000008

/**
 * Transfer count
 *
 */
#define DSPI_TCR_SPI_TCNT( x )    ( ( ( x ) & 0x0000FFFF ) << 16 )

/**
 * Clock and transfer attributes
 *
 */
#define DSPI_CTAR( x )            ( 0x0c + ( x * 4 ) )
#define DSPI_CTAR_DBR            0x80000000
#define DSPI_CTAR_TRSZ( x )       ( ( ( x ) & 0x0F ) << 27 )
#define DSPI_CTAR_TRSZ_MASK      0x87FFFFFF
#define DSPI_CTAR_CPOL           0x04000000
#define DSPI_CTAR_CPHA           0x02000000
#define DSPI_CTAR_LSBFE          0x01000000
#define DSPI_CTAR_PCSSCK( x )    ( ( ( x ) & 0x03 ) << 22 )
#define DSPI_CTAR_PCSSCK_7CLK    0x00A00000
#define DSPI_CTAR_PCSSCK_5CLK    0x00800000
#define DSPI_CTAR_PCSSCK_3CLK    0x00400000
#define DSPI_CTAR_PCSSCK_1CLK    0x00000000
#define DSPI_CTAR_PASC( x )    ( ( ( x ) & 0x03 ) << 20 )
#define DSPI_CTAR_PASC_7CLK      0x00300000
#define DSPI_CTAR_PASC_5CLK      0x00200000
#define DSPI_CTAR_PASC_3CLK      0x00100000
#define DSPI_CTAR_PASC_1CLK      0x00000000
#define DSPI_CTAR_PDT( x )    ( ( ( x ) & 0x03 ) << 18 )
#define DSPI_CTAR_PDT_7CLK       0x000A0000
#define DSPI_CTAR_PDT_5CLK       0x00080000
#define DSPI_CTAR_PDT_3CLK       0x00040000
#define DSPI_CTAR_PDT_1CLK       0x00000000
#define DSPI_CTAR_PBR( x )    ( ( ( x ) & 0x03 ) << 16 )
#define DSPI_CTAR_PBR_7CLK       0x00030000
#define DSPI_CTAR_PBR_5CLK       0x00020000
#define DSPI_CTAR_PBR_3CLK       0x00010000
#define DSPI_CTAR_PBR_1CLK       0x00000000
#define DSPI_CTAR_CSSCK( x )    ( ( ( x ) & 0x0F ) << 12 )
#define DSPI_CTAR_ASC( x )      ( ( ( x ) & 0x0F ) << 8 )
#define DSPI_CTAR_DT( x )       ( ( ( x ) & 0x0F ) << 4 )
#define DSPI_CTAR_BR( x )       ( ( x ) & 0x0F )
#define DSPI_CTAR_X_TRSZ        ( 1 << 16 )
#define DSPI_CTAR_X_DTCP( x )   ( ( x ) & 0x7FF )
/*
 * Status register FLAGS
 *
 */
#define DSPI_SR_TCF         0x80000000
#define DSPI_SR_TXRXS       0x40000000
#define DSPI_SR_EOQF        0x10000000
#define DSPI_SR_TFUF        0x08000000
#define DSPI_SR_TFFF        0x02000000
#define DSPI_SR_SPEF        0x00200000
#define DSPI_SR_CTCF        0x00800000
#define DSPI_SR_RFOF        0x00080000
#define DSPI_SR_RFDF        0x00020000
#define DSPI_SR_TFIWF       0x00040000
#define DSPI_SR_COUNT_RX    0x000000F0
#define DSPI_SR_TXCTR( x )    ( ( ( x ) & 0x0000F000 ) >> 12 )
#define DSPI_SR_TXPTR( x )    ( ( ( x ) & 0x00000F00 ) >> 8 )
#define DSPI_SR_RXCTR( x )    ( ( ( x ) & 0x000000F0 ) >> 4 )
#define DSPI_SR_RXPTR( x )    ( ( x ) & 0x0000000F )

/**
 * DMA/interrupt request selct and enable
 *
 */
#define DSPI_IRSR_DISABLE    0x00000000
#define DSPI_IRSR_TCFE       0x80000000
#define DSPI_IRSR_EOQFE      0x10000000
#define DSPI_IRSR_TFUFE      0x08000000
#define DSPI_IRSR_TFFFE      0x02000000
#define DSPI_IRSR_TFFFS      0x01000000
#define DSPI_IRSR_RFOFE      0x00080000
#define DSPI_IRSR_RFDFE      0x00020000
#define DSPI_IRSR_RFDFS      0x00010000

/**
 * Transfer control - 32-bit access
 *
 */
#define DSPI_TFR_PCS( x )     ( ( ( 1 << x ) & 0x0000003f ) << 16 )
#define DSPI_TFR_CONT     0x80000000
#define DSPI_TFR_CTAS( x )    ( ( ( x ) & 0x07 ) << 28 )
#define DSPI_TFR_EOQ      0x08000000
#define DSPI_TFR_CTCNT    0x04000000
#define DSPI_TFR_PE_MASC  0x02000000
#define DSPI_TFR_PP_MCSC  0x01000000
#define DSPI_TFR_CS7      0x00800000
#define DSPI_TFR_CS6      0x00400000
#define DSPI_TFR_CS5      0x00200000
#define DSPI_TFR_CS4      0x00100000
#define DSPI_TFR_CS3      0x00080000
#define DSPI_TFR_CS2      0x00040000
#define DSPI_TFR_CS1      0x00020000
#define DSPI_TFR_CS0      0x00010000
#define DSPI_TFR_CS( x )  (1 << ( 16 + ( x ) ) )
/**
 *Transfer Fifo
 *
 */
#define DSPI_TFR_TXDATA( x )     ( ( x ) & 0x0000FFFF )

/**
 * Bit definitions and macros for DRFR
 *
 */
#define DSPI_RFR_RXDATA( x )     ( ( x ) & 0x000000FF )

/**
 * Bit definitions and macros for DTFDR group
 *
 */
#define DSPI_TFDR_TXDATA( x )    ( ( x ) & 0x0000FFFF )
#define DSPI_TFDR_TXCMD( x )     ( ( ( x ) & 0x0000FFFF ) << 16 )

/**
 * Bit definitions and macros for DRFDR group
 *
 */
#define DSPI_RFDR_RXDATA( x )    ( ( x ) & 0x0000FFFF )

/** @} */

#endif /*DSPI_H*/
