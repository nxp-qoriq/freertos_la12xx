// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef __GEUL_FECA_BBDEV_TV_H__
#define __GEUL_FECA_BBDEV_TV_H__
/** Flags for LDPC encoder operation and capability structure */
enum bbdev_ipc_op_ldpcenc_flag_bitmasks {
	/** Set for bit-level interleaver bypass on output stream. */
	BBDEV_IPC_LDPC_INTERLEAVER_BYPASS = (1ULL << 0),
	/** If rate matching is to be performed */
	BBDEV_IPC_LDPC_RATE_MATCH = (1ULL << 1),
	/** Set for transport block CRC-24A attach */
	BBDEV_IPC_LDPC_CRC_24A_ATTACH = (1ULL << 2),
	/** Set for code block CRC-24B attach */
	BBDEV_IPC_LDPC_CRC_24B_ATTACH = (1ULL << 3),
	/** Set for code block CRC-16 attach */
	BBDEV_IPC_LDPC_CRC_16_ATTACH = (1ULL << 4),
	/** Set if a device supports encoder dequeue interrupts. */
	BBDEV_IPC_LDPC_ENC_INTERRUPTS = (1ULL << 5),
	/** Set if a device supports scatter-gather functionality. */
	BBDEV_IPC_LDPC_ENC_SCATTER_GATHER = (1ULL << 6),
	/** Set if a device supports concatenation of non byte aligned output */
	BBDEV_IPC_LDPC_ENC_CONCATENATION = (1ULL << 7)
};

/** Flags for LDPC decoder operation and capability structure */
enum bdev_ipc_op_ldpcdec_flag_bitmasks {
	/** Set for transport block CRC-24A checking */
	BBDEV_IPC_LDPC_CRC_TYPE_24A_CHECK = (1ULL << 0),
	/** Set for code block CRC-24B checking */
	BBDEV_IPC_LDPC_CRC_TYPE_24B_CHECK = (1ULL << 1),
	/** Set to drop the last CRC bits decoding output */
	BBDEV_IPC_LDPC_CRC_TYPE_24B_DROP = (1ULL << 2),
	/** Set for bit-level de-interleaver bypass on Rx stream. */
	BBDEV_IPC_LDPC_DEINTERLEAVER_BYPASS = (1ULL << 3),
	/** Set for HARQ combined input stream enable. */
	BBDEV_IPC_LDPC_HQ_COMBINE_IN_ENABLE = (1ULL << 4),
	/** Set for HARQ combined output stream enable. */
	BBDEV_IPC_LDPC_HQ_COMBINE_OUT_ENABLE = (1ULL << 5),
	/** Set for LDPC decoder bypass.
	 *  BBDEV_IPC_LDPC_HQ_COMBINE_OUT_ENABLE must be set.
	 */
	BBDEV_IPC_LDPC_DECODE_BYPASS = (1ULL << 6),
	/** Set for soft-output stream enable */
	BBDEV_IPC_LDPC_SOFT_OUT_ENABLE = (1ULL << 7),
	/** Set for Rate-Matching bypass on soft-out stream. */
	BBDEV_IPC_LDPC_SOFT_OUT_RM_BYPASS = (1ULL << 8),
	/** Set for bit-level de-interleaver bypass on soft-output stream. */
	BBDEV_IPC_LDPC_SOFT_OUT_DEINTERLEAVER_BYPASS = (1ULL << 9),
	/** Set for iteration stopping on successful decode condition
	 *  i.e. a successful syndrome check.
	 */
	BBDEV_IPC_LDPC_ITERATION_STOP_ENABLE = (1ULL << 10),
	/** Set if a device supports decoder dequeue interrupts. */
	BBDEV_IPC_LDPC_DEC_INTERRUPTS = (1ULL << 11),
	/** Set if a device supports scatter-gather functionality. */
	BBDEV_IPC_LDPC_DEC_SCATTER_GATHER = (1ULL << 12),
	/** Set if a device supports input/output HARQ compression. */
	BBDEV_IPC_LDPC_HARQ_6BIT_COMPRESSION = (1ULL << 13),
	/** Set if a device supports input LLR compression. */
	BBDEV_IPC_LDPC_LLR_COMPRESSION = (1ULL << 14),
	/** Set if a device supports HARQ input from
	 *  device's internal memory.
	 */
	BBDEV_IPC_LDPC_INTERNAL_HARQ_MEMORY_IN_ENABLE = (1ULL << 15),
	/** Set if a device supports HARQ output to
	 *  device's internal memory.
	 */
	BBDEV_IPC_LDPC_INTERNAL_HARQ_MEMORY_OUT_ENABLE = (1ULL << 16),
	/** Set if a device supports loop-back access to
	 *  HARQ internal memory. Intended for troubleshooting.
	 */
	BBDEV_IPC_LDPC_INTERNAL_HARQ_MEMORY_LOOPBACK = (1ULL << 17)
};

/** LDPC encode code block parameters */
struct bbdev_ipc_op_enc_ldpc_cb_params {
	/** E, length after rate matching in bits.
	 *  [3GPP TS38.212, section 5.4.2.1]
	 */
	uint32_t e;
};

/** LDPC encode transport block parameters */
struct bbdev_ipc_op_enc_ldpc_tb_params {
	/** Ea, length after rate matching in bits, r < cab.
	 *  [3GPP TS38.212, section 5.4.2.1]
	 */
	uint32_t ea;
	/** Eb, length after rate matching in bits, r >= cab.
	 *  [3GPP TS38.212, section 5.4.2.1]
	 */
	uint32_t eb;
	/** The total number of CBs in the TB or partial TB
	 * [1:BBDEV_LDPC_MAX_CODE_BLOCKS]
	 */
	uint8_t c;
	/** The index of the first CB in the inbound mbuf data, default is 0 */
	uint8_t r;
	/** The number of CBs that use Ea before switching to Eb, [0:63] */
	uint8_t cab;
};

/** LDPC decode code block parameters */
struct bbdev_ipc_op_dec_ldpc_cb_params {
	/** Rate matching output sequence length in bits or LLRs.
	 *  [3GPP TS38.212, section 5.4.2.1]
	 */
	uint32_t e;
};

/** LDPC decode transport block parameters */
struct bbdev_ipc_op_dec_ldpc_tb_params {
	/** Ea, length after rate matching in bits, r < cab.
	 *  [3GPP TS38.212, section 5.4.2.1]
	 */
	uint32_t ea;
	/** Eb, length after rate matching in bits, r >= cab.
	 *  [3GPP TS38.212, section 5.4.2.1]
	 */
	uint32_t eb;
	/** The total number of CBs in the TB or partial TB
	 * [1:BBDEV_LDPC_MAX_CODE_BLOCKS]
	 */
	uint8_t c;
	/** The index of the first CB in the inbound mbuf data, default is 0 */
	uint8_t r;
	/** The number of CBs that use Ea before switching to Eb, [0:63] */
	uint8_t cab;
};

/** Operation structure for LDPC encode.
 * An operation can be performed on one CB at a time "CB-mode".
 * An operation can be performed on one or multiple CBs that logically
 * belong to a TB "TB-mode".
 *
 * The input data is the CB or TB input to the decoder.
 *
 * The output data is the ratematched CB or TB data, or the output after
 * bit-selection if BBDEV_LDPC_INTERLEAVER_BYPASS is set.
 *
 * The output mbuf data structure is expected to be allocated by the
 * application with enough room for the output data.
 */
struct bbdev_ipc_op_ldpc_enc {
	/** Flags from bbdev_op_ldpcenc_flag_bitmasks */
	uint32_t op_flags;

	/** Rate matching redundancy version */
	uint8_t rv_index;
	/** 1: LDPC Base graph 1, 2: LDPC Base graph 2.
	 *  [3GPP TS38.212, section 5.2.2]
	 */
	uint8_t basegraph;
	/** Zc, LDPC lifting size.
	 *  [3GPP TS38.212, section 5.2.2]
	 */
	uint16_t z_c;
	/** Ncb, length of the circular buffer in bits.
	 *  [3GPP TS38.212, section 5.4.2.1]
	 */
	uint16_t n_cb;
	/** Qm, modulation order {2,4,6,8,10}.
	 *  [3GPP TS38.212, section 5.4.2.2]
	 */
	uint8_t q_m;
	/** Number of Filler bits, n_filler = K – K’
	 *  [3GPP TS38.212 section 5.2.2]
	 */
	uint16_t n_filler;
	/** [0 - TB : 1 - CB] */
	uint8_t code_block_mode;
	union {
		/** Struct which stores Transport Block specific parameters */
		struct bbdev_ipc_op_enc_ldpc_tb_params tb_params;
		/** Struct which stores Code Block specific parameters */
		struct bbdev_ipc_op_enc_ldpc_cb_params cb_params;
	};
};

/** Operation structure for LDPC decode.
 *
 * An operation can be performed on one CB at a time "CB-mode".
 * An operation can also be performed on one or multiple CBs that logically
 * belong to a TB "TB-mode" (Currently not supported).
 *
 * The input encoded CB data is the Virtual Circular Buffer data stream.
 *
 * Each byte in the input circular buffer is the LLR value of each bit of the
 * original CB.
 *
 * Hard output is a mandatory capability . This is the decoded CBs (CRC24A/B
 * is the last 24-bit in each decoded CB).
 *
 * Soft output is an optional capability. If supported, an LLR rate matched
 * output is computed in the soft_output buffer structure.
 * These are A Posteriori Probabilities (APP) LLR samples for coded bits.
 *
 * HARQ combined output is an optional capability. If supported, a LLR output
 * is streamed to the harq_combined_output buffer.
 *
 * HARQ combined input is an optional capability. If supported, a LLR input is
 * streamed from the harq_combined_input buffer.
 *
 * The output mbuf data structure is expected to be allocated by the
 * application with enough room for the output data.
 */
struct bbdev_ipc_op_ldpc_dec {
	/** Flags from bbdev_op_ldpcdec_flag_bitmasks */
	uint32_t op_flags;

	/** Rate matching redundancy version
	 *  [3GPP TS38.212, section 5.4.2.1]
	 */
	uint8_t rv_index;
	/** The maximum number of iterations to perform in decoding CB in
	 *  this operation - input
	 */
	uint8_t iter_max;
	/** The number of iterations that were performed in decoding
	 * CB in this decode operation - output
	 */
	uint8_t iter_count;
	/** 1: LDPC Base graph 1, 2: LDPC Base graph 2.
	 * [3GPP TS38.212, section 5.2.2]
	 */
	uint8_t basegraph;
	/** Zc, LDPC lifting size.
	 *  [3GPP TS38.212, section 5.2.2]
	 */
	uint16_t z_c;
	/** Ncb, length of the circular buffer in bits.
	 *  [3GPP TS38.212, section 5.4.2.1]
	 */
	uint16_t n_cb;
	/** Qm, modulation order {1,2,4,6,8}.
	 *  [3GPP TS38.212, section 5.4.2.2]
	 */
	uint8_t q_m;
	/** Number of Filler bits, n_filler = K – K’
	 *  [3GPP TS38.212 section 5.2.2]
	 */
	uint16_t n_filler;
	/** [0 - TB : 1 - CB] */
	uint8_t code_block_mode;
	union {
		/** Struct which stores Code Block specific parameters */
		struct bbdev_ipc_op_dec_ldpc_cb_params cb_params;
		/** Struct which stores Transport Block specific parameters */
		struct bbdev_ipc_op_dec_ldpc_tb_params tb_params;
	};
};
#endif //__GEUL_FECA_BBDEV_TV_H__
