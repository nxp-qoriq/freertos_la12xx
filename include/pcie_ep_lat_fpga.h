// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2024 NXP
 */

#ifndef _PCI_EP_LAT_FPGA_H
#define _PCI_EP_LAT_FPGA_H

typedef enum {
    CHANNEL_0,
    CHANNEL_1,
    CHANNEL_2,
    CHANNEL_3,
    CHANNEL_MAX,
}eChannelId_t;

typedef enum {
    CNFG_RX_CHANNEL_0 = 1,
    CNFG_RX_CHANNEL_1 = 2,
    CNFG_RX_CHANNEL_2 = 4,
    CNFG_RX_CHANNEL_3 = 8,
    CNFG_TX_CHANNEL_0 = 16,
    CNFG_TX_CHANNEL_1 = 32,
    CNFG_TX_CHANNEL_2 = 64,
    CNFG_TX_CHANNEL_3 = 128,
}eCnfgChannelId_t;

typedef enum {
    SEL_RX_DMA_CH0 = 1,
    SEL_RX_DMA_CH1 = 2,
    SEL_RX_DMA_CH2 = 4,
    SEL_RX_DMA_CH3 = 8,
    SEL_TX_DMA_CH0 = 16,
    SEL_TX_DMA_CH1 = 32,
    SEL_TX_DMA_CH2 = 64,
    SEL_TX_DMA_CH3 = 128,
}eSelDmaChannel_t;

typedef enum {
    EN_RX_DMA_CH0 = 1,
    EN_RX_DMA_CH1 = 2,
    EN_RX_DMA_CH2 = 4,
    EN_RX_DMA_CH3 = 8,
    EN_TX_DMA_CH0 = 16,
    EN_TX_DMA_CH1 = 32,
    EN_TX_DMA_CH2 = 64,
    EN_TX_DMA_CH3 = 128,
}eEnaDmaChannel_t;

/**
 * @struct channelCfg
 * @brief Structure to hold config info of channel
 *
 * @channelTotalSizeAddr: **OPTIONAL**
 *      Buffer to store total number of data received
 *      from the host to FPGA for TX.
 *      Buffer to store total number of data received
 *      from the FPGA to Host for RX.
 * @channelAddr:
 *      Host TX/RX Buffer Address.
 * @channelSize:
 *      Depth of the TX/RX buffer, 4K aligned.
 *      
 **/
typedef struct config {
    uint32_t channelTotalSizeAddr;
    uint32_t channelAddr;
    uint32_t channelSize;
}xConfig_t;

typedef struct chConfig {
    xConfig_t rx_config;
    xConfig_t tx_config; 
}xChConfig_t;

/**
 * @struct xConfigChannels_t
 * @brief Structure to Config multiple channels
 *
 * @channels: channels to be configured.
 *     BITs  : 31.........8    7    6     5     4    3     2     1     0
 *          ______________________________________________________________     
 *          |   Don't Care| CH3 | CH2 | CH1 | CH0 | CH3 | CH2 | CH1 | CH0|
 *          --------------------------------------------------------------
 *                        |<---------TX --------->|<-------RX----------->|
 *          Set the Bit for the channel which needs
 *          to be configured.
 *
 * @channelConfig[CHANNEL_MAX]:
 *     channelConfig[CHANNEL_0]: Configuration for Channel 0,
 *     channelConfig[CHANNEL_1]: Configuration for Channel 1,
 *     channelConfig[CHANNEL_2]: Configuration for Channel 2,
 *     channelConfig[CHANNEL_3]: Configuration for Channel 3
 *
 **/
typedef struct configChannel {
    uint32_t channels;
    xChConfig_t channelCfg[CHANNEL_MAX];
}xConfigChannels_t;

/**
 * @struct xSetDma_t
 * @brief Structure to enable/disable TX/RX DMA
 *
 * @selChannel: Channel which needs to be ENABLE/DISABLED respective bit will
 *               be marked as 1.
 *               
 *     BITs  : 31.........8    7    6     5     4    3     2     1     0
 *          ______________________________________________________________     
 *          |   Don't Care| CH3 | CH2 | CH1 | CH0 | CH3 | CH2 | CH1 | CH0|
 *          --------------------------------------------------------------
 *                        |<---------TX --------->|<-------RX----------->|
 *  
 *          Set the Bit for the channel which needs
 *          to be configured.
 *
 * @setChannel: Channels selected with @selChannel variable, for those
 *              select the status to be enable(1)/Disabled(0) with respective
 *              bits as mentioned below.
 *
 *     BITs  : 31.........8    7    6     5     4    3     2     1     0
 *          ______________________________________________________________     
 *          |   Don't Care| CH3 | CH2 | CH1 | CH0 | CH3 | CH2 | CH1 | CH0|
 *          --------------------------------------------------------------
 *                        |<---------TX --------->|<-------RX----------->|
 *  
 *          Set the Bit for the channel which needs to be enabled/disabled.
 *
 **/
typedef struct setDma {
    uint32_t selectChannel;
    uint32_t setChannel;
}xSetDma_t;

/*!
 * \fn void PcieEpSetDma (xlatCtrlReg *slat_ctrl_reg)
 * @brief EP host driver to Enable/Disable DMA.
 *
 * @param[in]	xSetDma_t DMA Enable/Disable config.
 *
 * @return
 *   - Returns Nothing
 */
void PcieEpSetDma (xSetDma_t *set_dma);

/*!
 * \fn void PcieEpConfigDmaBuf (xConfigChannels_t *slat_ctrl_reg)
 * @brief EP host driver to Config DMA Buffer.
 *
 * @param[in]	xConfigChannels_t DMA configuration.
 *
 * @return
 *   - Returns Nothing
 */
void PcieEpConfigDmaBuf (xConfigChannels_t *channel_config);

#endif /* _PCI_EP_LAT_FPGA_H */