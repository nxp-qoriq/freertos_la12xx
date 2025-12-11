// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef SHARED_CH_HW_PARAM_H_
#define SHARED_CH_HW_PARAM_H_

/**
 * @file        shared_ch_hw_param_du.h
 * @brief       BBDEV to FECA HW parameter conversion APIs.
 * @addtogroup  FECA_HW_PARAM_API
 * @{
 */

#define MAX_PUSCH_HOPS 2
#define N_SYM_SLOT 14

/**
 * Takes BBDev parameters as input and computes FECA HW Command registers for shared encode operation
 *
 * @param base_graph2_input
 *	if base_graph2_input = 0 --> means base graph 1 is used. If
 *	base_graph2_input = 1 --> means base graph 2 is used  (input)
 * @param Q_m
 *	modulation order, Q_m={1,2,4,6,8} (input)
 * @param e
 *	array of size C (number of code blocks) where each entry has the
 *	number of encoded bits inside each code block (input)
 * @param rv_id
 *	redundancy version ID (input)
 * @param A
 *	transport block payload size. Amin=24, Amax=1213032. A has to be multiple of (8*C) (input)
 * @param q
 *	parameter for c_init in scrambler, q= 0 or 1 for downlink, q=0 for uplink (input)
 * @param n_ID
 *	parameter for c_init in scrambler, n_ID={0, ..., 1023} (input)
 * @param n_RNTI
 *	parameter for c_init in scrambler, n_RNTI={0, ..., 65535} (input)
 * @param scrambler_bypass
 *	when scrambler_bypass = 1, it will bypass scrambler (input)
 * @param N_cb
 *	circular buffer size for rate matching parameter (input)
 * @param codeblock_mask
 *	binary mask to indicate the number of transmitted code blocks.
 *	codeblock_mask is an array of size 8.  If the code block is transmitted,
 *	the corresponding bit is 1, otherwise, it will be zero (input)
 * @param TBS_VALID
 *	if *TBS_VALID=1 --> A is valid number. if *TBS_VALID=0 --> A is invalid number (output)
 * @param set_index
 * 	The set_index as defined in 38.212 Table 5.3.2-1(output)
 * @param base_graph2
 *	When set the LDPC encoder uses base graph 2. When cleared it uses base graph 1(output)
 * @param lifting_index
 *	Determines which lifting size is used within the set index defined in
 *	the set_index bit field. This is the column number from 38.212
 *	Table 5.3.2-1. Lifting_index=0 is the smallest value in
 *	lifting_index=7 is the largest value
 * @param mod_order
 *	This is the modulation order and is the number of bits per constellation point.
 *	It is the value of Qm from 38.212 and can be 2 4 6 or 8. Other values will
 *	give incorrect results.
 * @param tb_24_bit_crc
 *	When 1 a 24 bit transport block CRC is appended. When 0 a 16 bit CRC is
 *	appended. Both CRC's are computed according to 38.212 Section 5.1.
 * @param num_code_blocks
 *	This is the number of code blocks in the transport block.
 * @param num_input_bytes
 *	The number of input bytes into the LDPC encoder. This is the value K'/8
 *	from 38.212 Section 5.2.2. This includes code block and transport block CRC(output).
 * @param e_floor_thresh
 *	When the code block number is <= e_floor_thresh, use the E value from
 *	the e_div_qm_floor register and when it is greater, use the
 *	e_div_qm_ceiling register(output).
 * @param num_output_bits_floor
 *	The number of output bits (Er) per code block. There are two versions of
 *	this value - one uses the floor of Er and the other uses the ceiling. The
 *	two Er values are defined in 38.212 section 5.4.2.1. This value is the
 *	floor version of Er(output).
 * @param num_output_bits_ceiling
 *	The number of output bits (Er) per code block. There are two versions of
 *	this value - one uses the floor of Er and the other uses the ceiling. The
 *	two Er values are defined in 38.212 section 5.4.2.1. This value is
 *	ceiling version of Er(output).
 * @param SE_SC_X1_INIT
 *	Encode scrambler x1 initialization value for pseudo-random sequence generation(output).
 * @param SE_SC_X2_INIT
 *	Encode scrambler x2 initialization value for pseudo-random sequence generation(output).
 * @param int_start_ofst_floor
 *	Interleaver starting offset(output).
 * @param int_start_ofst_ceiling
 *	Interleaver starting offset(output).
 * @param SE_CIRC_BUF
 * 	The size in bytes of the bit selection circular buffer defined in 38.212
 *	Section 5.4.2.1. This is the value of effective Ncb from that section(output).
 */

void  sch_encode_hw_param_du(int base_graph2_input,     // if base_graph2_input = 0 --> means base graph 1 is used. base_graph2_input = 1 --> means base graph 2 is used  (input)
				 	 	  int  Q_m,                     // modulation order, Q_m={1,2,4,6,8} (input)
						  int  *e,                     // array of size C (number of code blocks) where each entry has the number of encoded bits inside each code block. Note that E_0 + E_1 + ... + E_(C-1) = G_sch (input)
						  int  rv_id,                   // redundancy version ID (input)
						  int  A,                       // transport block payload size. Amin=24, Amax=1213032. A has to be multiple of (8*C) (input)
						  int  q,                       // parameter for c_init in scrambler, q= 0 or 1 for downlink, q=0 for uplink (input)
						  int  n_ID,                    // parameter for c_init in scrambler, n_ID={0, ..., 1023} (input)
						  int  n_RNTI,                  // parameter for c_init in scrambler, n_RNTI={0, ..., 65535} (input)
						  int  scrambler_bypass,        // when scrambler_bypass = 1, it will bypass scrambler (input)
						  int  N_cb,                    // circular buffer size for rate matching parameter (input)
						  int *codeblock_mask,          // binary mask to indicate the number of transmitted code blocks. codeblock_mask is an array of size 8.  If the code block is transmitted, the corresponding bit is 1, otherwise, it will be zero (input)
						  int *TBS_VALID,               // if *TBS_VALID = 1 --> A is valid number. if *TBS_VALID = 0 --> A is invalid number (output)
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
						  int *SE_CIRC_BUF);

/**
 * Takes BBDev parameters as input and computes FECA HW Command registers for shared decode operation
 *
 * @param base_graph2_input
 *	if base_graph2_input = 0 --> means base graph 1 is used.
 *	base_graph2_input = 1 --> means base graph 2 is used  (input).
 * @param Q_m
 *	modulation order, Q_m={{2,4,6,8} (input)
 * @param e
 *	array of size C (number of code blocks) where each entry has
 *	the number of encoded bits inside each code block. (input)
 * @param rv_id
 *	redundancy version ID (input)
 * @param A
 *	transport block payload size. Amin=24, Amax=1213032.
 *	A has to be multiple of (8*C)  (input)
 * @param q
 *	parameter for c_init in descrambler, q= 0 or
 *	1 for downlink, q=0 for uplink (input)
 * @param n_ID
 *	parameter for c_init in descrambler, n_ID={0, ..., 1023} (input)
 * @param n_RNTI
 *	parameter for c_init in descrambler, n_RNTI={0, ..., 65535} (input)
 * @param scrambler_bypass
 *	when scrambler_bypass = 1, it will bypass scrambler (input)
 * @param N_cb
 *	circular buffer size for rate matching parameter (input)
 * @param remove_tb_crc
 *	If 0, transport block CRC will be attached to the decoded bits.
 *	If 1, transport block CRC will be removed from the decoded bits (input)
 * @param size_harq_buffer
 *	HARQ buffer size (output)
 * @param TBS_VALID
 *	if *TBS_VALID=1 --> A is valid number. if *TBS_VALID=0 --> A
 *	is invalid number (output)
 * @param pC
 *	number of code blocks per transport block (output)
 * @param codeblock_mask
 * 	binary mask to indicate the number of transmitted code blocks.
 *	codeblock_mask is an array of size 8.  If the code block is transmitted,
 *	the corresponding bit is 1, otherwise, it will be zero (input)
 * @param set_index
 * 	The set_index as defined in 38.212 Table 5.3.2-1(output)
 * @param base_graph2
 *	When set, the LDPC encoder uses base graph 2. When cleared, it uses base graph 1(output)
 * @param lifting_index
 *	Determines which lifting size is used within the set index defined in
 *	the set_index bit field. This is the column number from 38.212
 *	Table 5.3.2-1. Lifting_index=0 is the smallest value in
 *	lifting_index=7 is the largest value(output)
 * @param mod_order
 *	This is the modulation order and is the number of bits per constellation point.
 *	It is the value of Qm from 38.212 and can be 2 4 6 or 8. Other values will
 *	give incorrect results(output).
 * @param tb_24_bit_crc
 *	When 1 a 24 bit transport block CRC is appended. When 0 a 16 bit CRC is
 *	appended. Both CRC's are computed according to 38.212 Section 5.1(output).
 * @param one_code_block
 * 	TRUE if TB has only one code block otherwise FALSE(output).
 * @param e_floor_thresh
 * 	When the code block number is <= e_floor_thresh use the E value from the
 *	e_div_qm_floor register and when it is greater than use the e_div_qm_ceiling
 * 	register. This is the C'-mod(G/(NL*Qm) C')-1 value used in 38.212 Section
 *	5.4.2.1. The code block number used here does not include code blocks that
 *	were not transmitted(output).
 * @param num_output_bytes
 *	The output bytes from the LDPC decoder. This is the value K'/8 from
 *	38.212 Section 5.2.2. This includes code block and transport block CRC(output).
 * @param bits_per_cb
 *	The number of bits in each code block. This includes filler bits.
 *	This is the value of K as defined in 38.212 Section 5.2.2(output).
 * @param num_filler_bits
 *	The number of filler bits in each code block. Defined in 38.212
 *	Section 5.2.2(output).
 * @param SD_SC_X1_INIT
 *	Encode scrambler x1 initialization value for pseudo-random sequence generation(output).
 * @param SD_SC_X2_INIT
 *	Encode scrambler x2 initialization value for pseudo-random sequence generation(output).
 * @param e_div_qm_floor
 *	The number of input LLRs (Er) divided by mod_order (Qm).
 *	There are two versions of this value - one uses the floor of Er
 *	and the other uses the ceiling. The two Er values are defined in
 *	38.212 section 5.4.2.1. This value is the floor version of Er / Qm (output).
 * @param e_div_qm_ceiling
 *	The number of input LLRs (Er) divided by mod_order (Qm).
 *	There are two versions of this value - one uses the floor of
 *	Er and the other uses the ceiling. The two Er values are
 *	defined in 38.212 section 5.4.2.1. This value is ceiling
 *	version of Er / Qm(output).
 * @param di_start_ofst_floor
 *	The bit di-interleaver starting offset into the circular buffer
 *	for chunk x of the interleaver. Array of size 8 (output).
 * @param di_start_ofst_ceiling
 *	The bit di-interleaver starting offset into the circular buffer
 *	for chunk x of the interleaver. Array of size 8 (output).
 * @param SD_CIRC_BUF
 *	The size in bytes of the bit selection circular buffer defined in
 *	38.212 Section 5.4.2.1. This is the value of effective Ncb from
 *	that section(output).
 * @param axi_data_num_bytes
 *	The number of bytes to transfer for the data portion of the entire
 *	transport block. The code block CRC is removed from the shared decoder
 *	output therefore this byte count does not include that. Also when the
 *	remove_tb_crc bit is set this count does not include that either(output)
 */

void  sch_decode_hw_param_du( int base_graph2_input,     // if base_graph2_input = 0 --> means base graph 1 is used. base_graph2_input = 1 --> means base graph 2 is used  (input)
				 	 	  int  Q_m,                     // modulation order, Q_m={1,2,4,6,8} (input)
						  int  *e,                      // array of size C (number of code blocks) where each entry has the number of encoded bits inside each code block. Note that E_0 + E_1 + ... + E_(C-1) = G_sch (input)
						  int  rv_id,                   // redundancy version ID (input)
						  int  A,                       // transport block payload size. Amin=24, Amax=1213032. A has to be multiple of (8*C)  (input)
						  int  q,                       // parameter for c_init in descrambler, q= 0 or 1 for downlink, q=0 for uplink (input)
						  int  n_ID,                    // parameter for c_init in descrambler, n_ID={0, ..., 1023} (input)
						  int  n_RNTI,                  // parameter for c_init in descrambler, n_RNTI={0, ..., 65535} (input)
						  int  scrambler_bypass,        // when scrambler_bypass = 1, it will bypass scrambler (input)
						  int  N_cb,                    // circular buffer size for rate matching parameter (input)
						  int  remove_tb_crc,           // If 0, transport block CRC will be attached to the decoded bits. If 1, transport block CRC will be removed from the decoded bits (input)
						  int *size_harq_buffer,        // HARQ buffer size (output)
						  int *TBS_VALID,               // if *TBS_VALID=1 --> A is valid number. if *TBS_VALID=0 --> A is invalid number (output)
						  int *pC,                      // number of code blocks per transport block (output)
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
						  int *axi_data_num_bytes);

/** @} */
#endif /* SHARED_CH_HW_PARAM_H_ */
