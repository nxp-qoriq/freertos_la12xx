/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2023 NXP */

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <debug_console.h>
#include "l1c_freq.h"

void print_freq_in_mHz(int64_t freq_in_mHz)
{
	uint64_t freq_int, freq_frac;

	if (freq_in_mHz < 0) {
		PRINTF("-");
		freq_in_mHz = -freq_in_mHz;
	}

	if (freq_in_mHz / 1000 >= GHz)
	{
		freq_int = freq_in_mHz / 1000 / GHz;
		freq_frac = freq_in_mHz - freq_int * GHz * 1000;
		freq_frac /= 1000 * KHz; // 6 digits visible only

		PRINTF("%u.%06u GHz\n", (uint32_t)freq_int, (uint32_t)freq_frac);

		return;
	}
	if (freq_in_mHz / 1000 >= MHz)
	{
		freq_int = freq_in_mHz / 1000 / MHz;
		freq_frac = freq_in_mHz - freq_int * MHz * 1000;
		freq_frac /= 1000; // 6 digits visible only

		PRINTF("%u.%06u MHz\n", (uint32_t)freq_int, (uint32_t)freq_frac);

		return;
	}
	if (freq_in_mHz / 1000 >= KHz)
	{
		freq_int = freq_in_mHz / 1000 / KHz;
		freq_frac = freq_in_mHz - freq_int * KHz * 1000;

		PRINTF("%u.%06u KHz\n", (uint32_t)freq_int, (uint32_t)freq_frac);

		return;
	}

	freq_int = freq_in_mHz / 1000;
	freq_frac = freq_in_mHz - freq_int * 1000;

	PRINTF("%u.%03u Hz\n", (uint32_t)freq_int, (uint32_t)freq_frac);
}

int64_t parse_freq_in_mHz(const char *s)
{
	int64_t freq_int = strtol(s, NULL, 10);
	int64_t freq_frac = 0;
	int32_t freq_div = 1;
	int64_t mult = 1000; // defaults to Hz, resulting value in mHz
	char *p;

	if(strchr(s, 'm'))
		mult = 1; // input in mHz

	p = strchr(s, '.');

	if (p) {
		p++;
		while (*p >= '0' && *p <= '9') {
			freq_frac *= 10;
			freq_frac += *p - '0';
			freq_div *= 10;
			p++;
		}
	}

	if (strchr(s, 'k') || strchr(s, 'K'))
		mult *= 1000; // input in KHz
	if (strchr(s, 'M'))
		mult *= 1000000; // input in MHz

	freq_frac *= mult;
	freq_frac /= freq_div;

	freq_int = freq_int * mult;

	freq_int += freq_frac;

	return freq_int;
}

int32_t nco_freq_calc(int64_t freq_mHz, int64_t Fs)
{
	uint32_t Resn = 0x80000000;
        int32_t f_nco, f_s;
	int64_t f_out;

	if (Fs < Resn)
		f_s = (int32_t)Fs;
	else
		return 0;

	if (freq_mHz > Resn || -freq_mHz > Resn)
	{
		// change to Hz computation
		f_nco = (int32_t)(((freq_mHz/1000) << 32) / f_s);
	} else {
		f_nco = (int32_t)((freq_mHz << 32) / f_s/ 1000);
	}

	f_out = f_nco;
	f_out /= 2;
	f_out *= Fs;
	f_out /= Resn;

#if 0	
	PRINTF("Requested freq is %lld [mHz], ", freq_mHz);
	print_freq_in_mHz(freq_mHz);

	PRINTF("Sampling freq is %lld [Hz], ", Fs);
	print_freq_in_mHz(Fs*1000);
	PRINTF("Resolution is %lld [mHz], ", IN_mHz(Fs)/2/Resn);
	print_freq_in_mHz(IN_mHz(Fs)/2/Resn);

        PRINTF("NCO freq norm: %d, hex %#x\n", f_nco, f_nco);
	PRINTF("NCO output freq %lld [Hz], ", f_out);
	print_freq_in_mHz(f_out*1000);
#endif
	return f_nco;
}
