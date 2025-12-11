// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"
#include "debug_console.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include "shared_ch_hw_param_du.h"

static void calc_int_start_ofst(int Q_m,
			           	 int rv_id,
						 int N_cb,
						 int BGnumber,
						 int K,
						 int K_dash,
						 int Zc,
						 int e_div_qm,
						 int *ncb_eff,
						 int *int_start_ofst);

static void offset_x1_x2(int c_init,
				  int offset,
				  int *X1,
				  int *X2);




static void  LDPC_evaluate_parameters(int   A ,
		                       int base_graph2_input,
							   int  *codeblock_mask,
						       int  *pBGnumber,
							   int  *pB,
							   int  *pi_LS,
							   int  *pZc,
							   int  *pN,
							   int  *pK,
							   int  *pK_dash,
							   int  *pC,
							   int  *C_prime,
							   int  *TBS_VALID);

void  sch_encode_hw_param_du(int base_graph2_input,     // if base_graph2_input = 0 --> means base graph 1 is used. base_graph2_input = 1 --> means base graph 2 is used  (input)
				 	 	  int  Q_m,                     // modulation order, Q_m={1,2,4,6,8} (input)
						  int  *e,                      // array of size C (number of code blocks) where each entry has the number of encoded bits inside each code block (input)
						  int  rv_id,                   // redundancy version ID (input)
						  int  A,                       // transport block payload size. Amin=24, Amax=1213032. A+tb_crc_size has to be multiple of (8*C) (input)
						  int  q,                       // parameter for c_init in scrambler, q= 0 or 1 for downlink, q=0 for uplink (input)
						  int  n_ID,                    // parameter for c_init in scrambler, n_ID={0, ..., 1023} (input)
						  int  n_RNTI,                  // parameter for c_init in scrambler, n_RNTI={0, ..., 65535} (input)
						  int  scrambler_bypass,        // when scrambler_bypass = 1, it will bypass scrambler (input)
						  int  N_cb,                    // circular buffer size for rate matching parameter (input)
						  int *codeblock_mask,          // binary mask to indicate the number of transmitted code blocks. codeblock_mask is an array of size 8.  If the code block is transmitted, the corresponding bit is 1, otherwise, it will be zero (input)
						  int *TBS_VALID,               // if *TBS_VALID=1 --> A is valid number. if *TBS_VALID=0 --> A is invalid number (output)
						  // HW parameters
						  int *set_index,
						  int *base_graph2,
						  int *lifting_index,
						  int *mod_order,
						  int *tb_24_bit_crc,
						  int *num_code_blocks,
						  int *num_input_bytes,
						  int *e_floor_thresh,
						  int *num_output_bits_floor,
						  int *num_output_bits_ceiling,
						  int *SE_SC_X1_INIT,
						  int *SE_SC_X2_INIT,
						  int *int_start_ofst_floor,    // Array of size 8
						  int *int_start_ofst_ceiling,  // Array of size 8
						  int *SE_CIRC_BUF)
{
	int B, i_LS, Zc, BGnumber, K_dash, i, K, N, C_prime, C, E_sum = 0;
	// evaluate parameters
	LDPC_evaluate_parameters(A, base_graph2_input, codeblock_mask, &BGnumber, &B, &i_LS, &Zc, &N, &K, &K_dash, &C, &C_prime, TBS_VALID);

	for (i=0; i<C; i++)
	{
		E_sum += e[i];
	}

	if (E_sum)
	{
		*set_index = i_LS;
		if (BGnumber ==2) {
			*base_graph2 = 1;
		}
		else {
			*base_graph2 = 0;
		}

		if (Zc==2 || Zc==3 || Zc==5 || Zc==7 || Zc==9 || Zc==11 || Zc==13 || Zc==15)
		{ *lifting_index = 0;}
		else if (Zc==4 || Zc==6 || Zc==10 || Zc==14 || Zc==18 || Zc==22 || Zc==26 || Zc==30)
		{ *lifting_index = 1;}
		else if (Zc==8 || Zc==12 || Zc==20 || Zc==28 || Zc==36 || Zc==44 || Zc==52 || Zc==60)
		{ *lifting_index = 2;}
		else if (Zc==16 || Zc==24 || Zc==40 || Zc==56 || Zc==72 || Zc==88 || Zc==104 || Zc==120)
		{ *lifting_index = 3;}
		else if (Zc==32 || Zc==48 || Zc==80 || Zc==112 || Zc==144 || Zc==176 || Zc==208 || Zc==240)
		{ *lifting_index = 4;}
		else if (Zc==64 || Zc==96 || Zc==160 || Zc==224 || Zc==288 || Zc==352)
		{ *lifting_index = 5;}
		else if (Zc==128 || Zc==192 || Zc==320)
		{ *lifting_index = 6;}
		else if (Zc==256 || Zc==384)
		{ *lifting_index = 7;}

		*mod_order = Q_m;
		if (B-A ==24) {
			*tb_24_bit_crc = 1;
		}
		else {
			*tb_24_bit_crc = 0;
		}
		*num_code_blocks = C;
		*num_input_bytes = K_dash/8;
		int cb_counter=0;
		for (i=0; i<C; i++)
		{
			if ((codeblock_mask[i/32] >> (i % 32)) & 1)
			{
				if (cb_counter==0)
				{
					*num_output_bits_floor = e[i];
					*e_floor_thresh = -1;
				}
				if (cb_counter== C_prime-1)
				{
					*num_output_bits_ceiling = e[i];
				}
				if (e[i] == *num_output_bits_floor)
				{
					*e_floor_thresh= *e_floor_thresh +1;
				}
				cb_counter++;

			}
		}
		if(scrambler_bypass)
		{
			*SE_SC_X1_INIT = 0;
			*SE_SC_X2_INIT = 0;
		}
		else
		{
			offset_x1_x2((n_RNTI << 15) + (q << 14) + n_ID, 1600, SE_SC_X1_INIT, SE_SC_X2_INIT);
		}
		int e_div_qm_floor= *num_output_bits_floor / Q_m;
		int e_div_qm_ceiling= *num_output_bits_ceiling / Q_m;
		calc_int_start_ofst(Q_m, rv_id, N_cb, BGnumber, K, K_dash, Zc, e_div_qm_floor, SE_CIRC_BUF, int_start_ofst_floor);
		calc_int_start_ofst(Q_m, rv_id, N_cb, BGnumber, K, K_dash, Zc, e_div_qm_ceiling, SE_CIRC_BUF, int_start_ofst_ceiling);
	}
	else // no ULSCH
	{
		*set_index = 3;
		*base_graph2 = 1;
		*lifting_index = 0;
		*mod_order = 1;
		*tb_24_bit_crc = 0;
		*num_code_blocks = 1;
		*num_input_bytes = 5;
		*e_floor_thresh = 0;
		*num_output_bits_floor = 0;
		*num_output_bits_ceiling = 0;
		if(scrambler_bypass)
		{
			*SE_SC_X1_INIT = 0;
			*SE_SC_X2_INIT = 0;
		}
		else
		{
			offset_x1_x2((n_RNTI << 15) + (q << 14) + n_ID, 1600, SE_SC_X1_INIT, SE_SC_X2_INIT);
		}
		*SE_CIRC_BUF = 320;
		for (i=0; i<7; i++)
		{
			int_start_ofst_floor[i]=0;
			int_start_ofst_ceiling[i]=0;
		}
	}


}

void  sch_decode_hw_param_du( int base_graph2_input,     // if base_graph2_input = 0 --> means base graph 1 is used. base_graph2_input = 1 --> means base graph 2 is used  (input)
				 	 	  int  Q_m,                     // modulation order, Q_m={1,2,4,6,8} (input)
						  int  *e,                      // array of size C (number of code blocks) where each entry has the number of encoded bits inside each code block (input)
						  int  rv_id,                   // redundancy version ID (input)
						  int  A,                       // transport block payload size. Amin=24, Amax=1213032. A+tb_crc_size has to be multiple of (8*C)  (input)
						  int  q,                       // parameter for c_init in descrambler, q= 0 or 1 for downlink, q=0 for uplink (input)
						  int  n_ID,                    // parameter for c_init in descrambler, n_ID={0, ..., 1023} (input)
						  int  n_RNTI,                  // parameter for c_init in descrambler, n_RNTI={0, ..., 65535} (input)
						  int  scrambler_bypass,        // when scrambler_bypass = 1, it will bypass scrambler (input)
						  int  N_cb,                    // circular buffer size for rate matching parameter (input)
						  int  remove_tb_crc,           // (hw param) If 0, transport block CRC will be attached to the decoded bits. If 1, transport block CRC will be removed from the decoded bits (input)
						  int *size_harq_buffer,        // HARQ buffer size (output)
						  int *TBS_VALID,               // if *TBS_VALID=1 --> A is valid number. if *TBS_VALID=0 --> A is invalid number (output)
						  int *C,                       // number of code blocks per transport block (output)
						  int *codeblock_mask,          // binary mask to indicate the number of transmitted code blocks. codeblock_mask is an array of size 8.  If the code block is transmitted, the corresponding bit is 1, otherwise, it will be zero (input)
						  // HW parameters
						  int *set_index,
						  int *base_graph2,
						  int *lifting_index,
						  int *mod_order,
						  int *tb_24_bit_crc,
						  int *one_code_block,
						  int *e_floor_thresh,
						  int *num_output_bytes,
						  int *bits_per_cb,
						  int *num_filler_bits,
						  int *SD_SC_X1_INIT,
						  int *SD_SC_X2_INIT,
						  int *e_div_qm_floor,
						  int *e_div_qm_ceiling,
						  int *di_start_ofst_floor,     // Array of size 8
						  int *di_start_ofst_ceiling,   // Array of size 8
						  int *SD_CIRC_BUF,
						  int *axi_data_num_bytes)
{
	int B, i_LS, Zc, BGnumber, K_dash, i, K, N, C_prime;

	// evaluate parameters
	LDPC_evaluate_parameters(A, base_graph2_input, codeblock_mask, &BGnumber, &B, &i_LS, &Zc, &N, &K, &K_dash, C, &C_prime, TBS_VALID);

	*set_index = i_LS;
	if (BGnumber ==2) {
		*base_graph2 = 1;
	}
	else {
		*base_graph2 = 0;
	}
	if (Zc==2 || Zc==3 || Zc==5 || Zc==7 || Zc==9 || Zc==11 || Zc==13 || Zc==15)
	{ *lifting_index = 0;}
	else if (Zc==4 || Zc==6 || Zc==10 || Zc==14 || Zc==18 || Zc==22 || Zc==26 || Zc==30)
	{ *lifting_index = 1;}
	else if (Zc==8 || Zc==12 || Zc==20 || Zc==28 || Zc==36 || Zc==44 || Zc==52 || Zc==60)
	{ *lifting_index = 2;}
	else if (Zc==16 || Zc==24 || Zc==40 || Zc==56 || Zc==72 || Zc==88 || Zc==104 || Zc==120)
	{ *lifting_index = 3;}
	else if (Zc==32 || Zc==48 || Zc==80 || Zc==112 || Zc==144 || Zc==176 || Zc==208 || Zc==240)
	{ *lifting_index = 4;}
	else if (Zc==64 || Zc==96 || Zc==160 || Zc==224 || Zc==288 || Zc==352)
	{ *lifting_index = 5;}
	else if (Zc==128 || Zc==192 || Zc==320)
	{ *lifting_index = 6;}
	else if (Zc==256 || Zc==384)
	{ *lifting_index = 7;}

	*mod_order = Q_m;
	if (B-A ==24) {
		*tb_24_bit_crc = 1;
	}
	else {
		*tb_24_bit_crc = 0;
	}
	*one_code_block = (*C == 1)? 1 : 0;
	*num_output_bytes = K_dash/8;
	*bits_per_cb = K;
	*num_filler_bits = K-K_dash;
	if(remove_tb_crc)
	{
		*axi_data_num_bytes = A/8;
	}
	else
	{
		if(*tb_24_bit_crc)
		{
			*axi_data_num_bytes = (A+24)/8;
		}
		else
		{
			*axi_data_num_bytes = (A+16)/8;
		}
	}
	if(scrambler_bypass)
	{
		*SD_SC_X1_INIT = 0;
		*SD_SC_X2_INIT = 0;
	}
	else
	{
		offset_x1_x2((n_RNTI << 15) + (q << 14) + n_ID, 1600, SD_SC_X1_INIT, SD_SC_X2_INIT);
	}
	int cb_counter=0;
	for (i=0; i<*C; i++)
	{
		if ((codeblock_mask[i/32] >> (i % 32)) & 1)
		{
			if (cb_counter==0)
			{
				*e_div_qm_floor = e[i]/Q_m;
				*e_floor_thresh = -1;
			}
			if (cb_counter== C_prime-1)
			{
				*e_div_qm_ceiling = e[i]/Q_m;
			}
			if (e[i] == *e_div_qm_floor * Q_m)
			{
				*e_floor_thresh= *e_floor_thresh +1;
			}
			cb_counter++;

		}
	}
	calc_int_start_ofst(Q_m, rv_id, N_cb, BGnumber, K, K_dash, Zc, *e_div_qm_floor, SD_CIRC_BUF, di_start_ofst_floor);
	calc_int_start_ofst(Q_m, rv_id, N_cb, BGnumber, K, K_dash, Zc, *e_div_qm_ceiling, SD_CIRC_BUF, di_start_ofst_ceiling);
	int offset_harq_buffer;
	offset_harq_buffer = 128 * ((*SD_CIRC_BUF / 128) + ((*SD_CIRC_BUF % 128) != 0));
	*size_harq_buffer = (*C) * offset_harq_buffer;

}

static void calc_int_start_ofst(int Q_m,
			           int rv_id,
				       int N_cb,
					   int BGnumber,
				       int K,
					   int K_dash,
				       int Zc,
				       int e_div_qm,
					   int *ncb_eff,
					   int *int_start_ofst)
{
	int i, k0 = 0, num_filler_bits=K-K_dash;
	if (rv_id == 0)
	{
		k0 = 0;
	}
	else if (rv_id == 1)
	{
		k0 = (BGnumber == 1) ?  ((17*N_cb)/(66*Zc))*Zc :  ((13*N_cb)/(50*Zc))*Zc ;
	}
	else if (rv_id == 2)
	{
		k0 = (BGnumber == 1) ?  ((33*N_cb)/(66*Zc))*Zc :  ((25*N_cb)/(50*Zc))*Zc ;
	}
	else if (rv_id == 3)
	{
		k0 = (BGnumber == 1) ?  ((56*N_cb)/(66*Zc))*Zc :  ((43*N_cb)/(50*Zc))*Zc ;
	}

    int filler_end   = K - 2*Zc;                     // index after filler end
    int filler_start = filler_end - num_filler_bits; // index at filler_start

    int k0_eff    = (k0 > filler_end) ?
                      k0 - num_filler_bits :    // k0 outside and after filler
                      (k0 > filler_start) ?
                        filler_start :          // k0 inside filler region
                        k0;

    *ncb_eff   = (N_cb > filler_end) ?   N_cb - num_filler_bits :  // ncb outside and after filler
                 (N_cb > filler_start) ? filler_start :         // ncb inside filler region
				  N_cb;

    for (i=0; i<Q_m; i++){
      int_start_ofst[i] = (k0_eff + i*e_div_qm) % *ncb_eff;
    }
    for (int i=Q_m; i<8; i++){
      int_start_ofst[i] = 0;  // optional, not used
    }
}

static void offset_x1_x2(int c_init,
		          int offset,
				  int *X1,
				  int *X2)
{
    int n;
    // static int x1[100000];
    //static int x2[100000];

    extern int *x1;
    extern int *x2;

    for(n = 0; n < 31; n++){
        x1[n] = 0;
        x2[n] = (c_init >> n) & 0x01;
    }
    x1[0] = 1;

    for(n = 0; n < offset; n++){
        x1[n+31] = (x1[n+3] + x1[n]) & 0x01;
        x2[n+31] = (x2[n+3] + x2[n+2] + x2[n+1] + x2[n]) & 0x01;
    }
    *X1 = 0;
    *X2 = 0;
    for(n = 0; n < 31; n++){
    	*X1 ^= (x1[offset+n] << n);
        *X2 ^= (x2[offset+n] << n);
    }

}




static void  LDPC_evaluate_parameters(int   A ,
		                       int   base_graph2_input,
							   int  *codeblock_mask,
						       int  *BGnumber,
							   int  *B,
							   int  *i_LS,
							   int  *Zc,
							   int  *N,
							   int  *K,
							   int  *K_dash,
							   int  *C,
							   int  *C_prime,
							   int  *TBS_VALID)
{
	// find BGnumber
	if (base_graph2_input == 1)
	{
		*BGnumber = 2;
	}
	else
	{
		*BGnumber = 1;
	}

	// find B
	if (A > 3824)   // CRC 24-bits type A
	{
		*B = A+24;
	}
	else    // CRC 16-bits
	{
		*B = A+16;
	}

	//find K_b, K_cb
	int i, K_cb = 0, K_b = 0;
	if (*BGnumber ==1)
	{
		K_cb = 8448;
		K_b = 22;
	}
	else if (*BGnumber ==2)
	{
		K_cb = 3840;
		if (*B > 640)
		{
			K_b = 10;
		}
		else if (*B > 560)
		{
			K_b = 9;
		}
		else if (*B > 192)
		{
			K_b = 8;
		}
		else
		{
			K_b = 6;
		}
	}
	// find K_dash, C
	if (*B <= K_cb)  // single code block (no CRC)
	{
		*C = 1;
		*K_dash = *B;
	}
	else       // more thank one code block (CRC is added)
	{
		*C = ((*B) / (K_cb-24)) + (((*B) % (K_cb-24)) != 0);
		*K_dash = ((*B)+(24*(*C)))/(*C);
	}

	// find out Zc, i_LS
	*Zc = 384;
	*i_LS = 1;
	static int Z[51] = {2, 4, 8, 16, 32, 64, 128, 256, 3, 6, 12, 24, 48, 96, 192, 384, 5, 10, 20, 40, 80, 160, 320, 7, 14, 28, 56, 112, 224, 9, 18, 36, 72, 144, 288, 11, 22, 44, 88, 176, 352, 13, 26, 52, 104, 208, 15, 30, 60, 120, 240};
	static int i_LS_vec[51] = {0, 0, 0,  0,  0,  0,   0,   0, 1, 1,  1,  1,  1,  1,   1,   1, 2,  2,  2,  2,  2,   2,   2, 3,  3,  3,  3,   3,   3, 4,  4,  4,  4,   4,   4,  5,  5,  5,  5,   5,   5,  6,  6,  6,   6,   6,  7,  7,  7,   7,   7};
	for (i = 0; i < 51; i++)
	{
		if ((K_b*Z[i] >= (*K_dash)) && Z[i] < *Zc)
		{
			*Zc = Z[i];
			*i_LS = i_LS_vec[i];
		}
	}

	// calculate K, N
	if (*BGnumber == 1)
	{
		*K = 22 * (*Zc);
		*N = 66 * (*Zc);
	}
	else
	{
		*K = 10 * (*Zc);
		*N = 50 * (*Zc);
	}

	// check if transport block size A is valid or not
	if (*B % (8*(*C)))
	{
		*TBS_VALID=0;
	}
	else
	{
		*TBS_VALID=1;
	}

	*C_prime = 0;
	for (i=0; i < *C; i++)
	{
		*C_prime += ((codeblock_mask[i/32] >> (i % 32)) & 1);
	}
}
