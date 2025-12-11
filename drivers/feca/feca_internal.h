// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef __FECA_INTERNAL_H
#define __FECA_INTERNAL_H



#include "feca_api.h"
#include "fsl_dbg.h"

/* Chain Specific Channel IDs
 *
 * Struct for Valid channel ids for a feca chain
 * cd_ch_ids 	:- channel ids for control decode
 * sd_ch_ids 	:- Channel Ids for Shared ch. decode
 * ce_ch_ids	:- Channel Ids for Control Encode
 * se_ch_ids	:- Channel Ids for Shared Encode
 * 
 */


struct channel_ids{
	uint32_t cd_ch_ids[CH_IN_DCM_CSI2 + 1][TB_MAX];
	uint32_t sd_ch_ids[CH_CRC_OUT + 1][TB_MAX];
	uint32_t ce_ch_ids[CH_OUT + 1];
	uint32_t se_ch_ids[CH_OUT + 1][TB_MAX];
}__attribute__((packed)) feca_channel_ids = {
		{
			{0, 0, 0, 0, 0, 0, 0, 0},
			{18, 0, 0, 0, 0, 0, 0, 0},
			{19, 0, 0, 0, 0, 0, 0, 0},
			{20, 0, 0, 0, 0, 0, 0, 0},
			{63, 64, 65, 66, 67, 68, 69, 70},
			{71, 72, 73, 74, 75, 76, 77, 78},
			{79, 80, 81, 82, 83, 84, 85, 86},
			{87, 88, 89, 90, 91, 92, 93, 94},
			{95, 96, 97, 98, 99, 100, 101, 102},
			{103, 104, 105, 106, 107, 108, 109, 110}
		},
		{
			{1, 2, 3, 4, 5, 6, 7, 8},
			{21, 22, 23, 24, 25, 26, 27, 28},
			{29, 30, 31, 32, 33, 34, 35, 36},
			{37, 38, 39, 40, 41, 42, 43, 44}		
		},
		{9, 45, 46},
		{
			{10, 11, 12, 13, 14, 15, 16, 17},
			{47, 48, 49, 50, 51, 52, 53, 54},
			{55, 56, 57, 58, 59, 60, 61, 62}
		}
};


/*This function will be used to fetch the valid channel ids and it's type */
void *feca_ch_handler(struct feca_chain *chain, feca_ch_events_t event, void *cookiee);
struct feca_channel *feca_channel_init(struct feca_chain *chain);
uint32_t mem_allocator(struct feca_channel *channel, feca_mem_t mem_type, uint32_t size);
struct feca_channel *channel_alloc(int count);
#endif /*__FECA_INTERNAL_H */
