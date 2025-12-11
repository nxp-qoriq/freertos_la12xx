// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */

#ifndef _PMUX_H_
#define _PMUX_H_

#define FLAG_0b11	0b11	/* Mask for Two-bit bit field */
#define FLAG_0b111	0b111	/* Mask for Two-bit bit field */

#define PMUX_HS_MODE_ALL 0xAAAAAAAA
#define PMUX_LS_MODE_ALL 0xAAAAAAAA

#define PMUX_HS_GPIO_MODE_ALL 0x00000000
#define PMUX_LS_GPIO_MODE_ALL 0x00000000

#if defined(GEUL_LA1238RDB) || defined(GEUL_LA1238CPE) || defined (GEUL_LA1224)
#define PMUX_LS_GPIO_MODE_MW	0xAAA80005
#endif

enum pmux_num {
    PMUX_1 = 0,
    PMUX_2,
    PMUX_3,
    PMUX_4,
    PMUX_5,
    PMUX_6,
    PMUX_7,
    PMUX_8
};

enum pmux_index {
    /* PMUX 1 */
    PMUX1_GPIO_1_0 = 0,
    PMUX1_GPIO_1_1,
    PMUX1_GPIO_1_2,
    PMUX1_GPIO_1_3,
    PMUX1_GPIO_1_4,
    PMUX1_GPIO_1_5,
    PMUX1_GPIO_1_6,
    PMUX1_WDOG_TOUT,
    PMUX1_SPI3,
    PMUX1_GPIO_1_8_LS_TIME_GPO40,
    PMUX1_GPIO_1_9_LS_TIME_GPO41,
    PMUX1_GPIO_1_10_LS_TIME_GPO42,
    PMUX1_GPIO_1_14_LS_TIME_GPO46,
    PMUX1_SPI3_CS1_B,
    PMUX1_SPI3_CS2_B,
    PMUX1_SPI3_CS3_B,
    /* PMUX 2 */
    PMUX2_RES_00_01 = 16,
    PMUX2_SPI2_CS1_B,
    PMUX2_SPI2_CS2_B,
    PMUX2_SPI2_CS3_B,
    PMUX2_MODEM1_SYNC_IN,
    PMUX2_MODEM1_SYNC_OUT,
    PMUX2_SPI1_CS1_B,
    PMUX2_SPI1_CS2_B,
    PMUX2_SPI1_CS3_B,
    PMUX2_IIC1,
    PMUX2_IIC2,
    PMUX2_IIC3,
    PMUX2_UART2,
    PMUX2_UART1,
    PMUX2_RES_DUMMY,
    PMUX2_RES_30_31,
    /* PMUX 3 */
    PMUX3_GPIO_2_0 = 32,
    PMUX3_GPIO_2_1,
    PMUX3_GPIO_2_2,
    PMUX3_GPIO_2_3,
    PMUX3_GPIO_2_4,
    PMUX3_GPIO_2_5,
    PMUX3_GPIO_2_6,
    PMUX3_GPIO_2_7,
    PMUX3_GPIO_2_8,
    PMUX3_GPIO_2_9,
    PMUX3_GPIO_2_10,
    PMUX3_GPIO_2_11,
    PMUX3_GPIO_2_12,
    PMUX3_GPIO_2_13,
    PMUX3_GPIO_2_14,
    PMUX3_GPIO_2_15,
    /* PMUX 4 */
    PMUX4_GPIO_2_16 = 48,
    PMUX4_GPIO_2_17,
    PMUX4_GPIO_2_18,
    PMUX4_GPIO_2_19,
    PMUX4_GPIO_2_20,
    PMUX4_GPIO_2_21,
    PMUX4_GPIO_2_22,
    PMUX4_GPIO_2_23,
    PMUX4_GPIO_2_24,
    PMUX4_GPIO_2_25,
    PMUX4_GPIO_2_26,
    PMUX4_GPIO_2_27,
    PMUX4_GPIO_2_28,
    PMUX4_GPIO_2_29,
    PMUX4_GPIO_2_30,
    PMUX4_GPIO_2_31,
    /* PMUX 5 */
    PMUX5_GPIO_3_0 = 64,
    PMUX5_GPIO_3_1,
    PMUX5_GPIO_3_2,
    PMUX5_GPIO_3_3,
    PMUX5_GPIO_3_4,
    PMUX5_GPIO_3_5,
    PMUX5_GPIO_3_6,
    PMUX5_GPIO_3_7,
    PMUX5_GPIO_3_8,
    PMUX5_GPIO_3_9,
    PMUX5_GPIO_3_10,
    PMUX5_GPIO_3_11,
    PMUX5_GPIO_3_12,
    PMUX5_GPIO_3_13,
    PMUX5_GPIO_3_14,
    PMUX5_GPIO_3_15,
    /* PMUX 6 */
    PMUX6_GPIO_3_16 = 80,
    PMUX6_GPIO_3_17,
    PMUX6_GPIO_3_18,
    PMUX6_GPIO_3_19,
    PMUX6_GPIO_3_20,
    PMUX6_GPIO_3_21,
    PMUX6_GPIO_3_22,
    PMUX6_GPIO_3_23,
    PMUX6_GPIO_3_24,
    PMUX6_GPIO_3_25,
    PMUX6_GPIO_3_26,
    PMUX6_GPIO_3_27,
    PMUX6_GPIO_3_28,
    PMUX6_GPIO_3_29,
    PMUX6_GPIO_3_30_31,
    PMUX6_RES_30_31,
    /* PMUX 7 */
    PMUX7_GPIO_4_0 = 96,
    PMUX7_GPIO_4_1,
    PMUX7_GPIO_4_2,
    PMUX7_IRQ_0,
    PMUX7_IRQ_1,
    PMUX7_IRQ_2,
    PMUX7_IRQ_3,
    PMUX7_IRQ_4,
    PMUX7_SPI6,
    PMUX7_RES_18_19,
    PMUX7_RES_20_21,
    PMUX7_RES_22_23,
    PMUX7_SPI6_CS1_B,
    PMUX7_SPI6_CS2_B,
    PMUX7_SPI6_CS3_B,
    PMUX7_RES_30_31,
    /* PMUX 8 */
    PMUX8_RES_00_01 = 112,
    PMUX8_SPI4_CS2_B,
    PMUX8_SPI4_CS3_B,
    PMUX8_SPI5,
    PMUX8_RES_8_9,
    PMUX8_RES_10_11,
    PMUX8_RES_12_13,
    PMUX8_SPI5_CS1_B,
    PMUX8_SPI5_CS2_B,
    PMUX8_SPI5_CS3_B,
    PMUX8_MODEM2_SYNC_IN,
    PMUX8_MODEM2_SYNC_OUT,
    PMUX8_IIC4,
    PMUX8_IIC5,
    PMUX8_IIC6,
    PMUX8_RES_30_31
};

enum pmux_mode {
    MAIN_OUTPUT = 0,
    ALT_MODE1,
    ALT_MODE2,
    ALT_MODE3,
    ALT_MODE5,
    ALT_MODE6,
    ALT_MODE7,
    ALT_MODE8
};


/*This function toggles the PMUX_CR as per the INPUT MUX "mode
 * for the specified PMUX num
 * */
int switchPMuxMode(enum pmux_num num,
        enum pmux_index index, enum pmux_mode mode);

/* Switch's PMUX to HS Mode */
void vSwitchGpioToHS( void );
/* Switch's PMUX to LS Mode */
void vSwitchGpioToLS( void );
/* Switch's PMUX to GPIO Mode of HS */
void vSwitchHSToGpio( void );
/* Switch's PMUX to GPIO Mode of LS */
void vSwitchLSToGpio( void );
#endif
