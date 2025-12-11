/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2023 NXP */

#ifndef _L1C_FREQ_H
#define _L1C_FREQ_H

#define KHz 1000UL
#define MHz (1000*KHz)
#define GHz (1000*MHz)
#define IN_mHz(x) (x*1000)

void print_freq_in_mHz(int64_t freq_in_mHz);
int64_t parse_freq_in_mHz(const char *s);
int32_t nco_freq_calc(int64_t freq_mHz, int64_t Fs_Hz);

#endif /* _L1C_FREQ_H */
