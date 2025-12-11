// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2021 NXP
 */

#ifndef __IBR_H__
#define __IBR_H__

#define MAX_SG_ENTRIES 8
#define MAX_CF_WORDS 200

struct ibr_sg_table {
    uint32_t len;
    uint32_t rsvd;
    uint32_t src;
    uint32_t dest;
};

struct ibr_cf_word {
    uint32_t addr;
    uint32_t value;
};

// Header structure
struct ibr_header {
    uint32_t preamble;
    uint32_t sgentries_num;
    struct ibr_sg_table sgtable[MAX_SG_ENTRIES];
    uint32_t entry_point;
    uint32_t flags;
    uint32_t cfwords_count;
    struct ibr_cf_word cfword[MAX_CF_WORDS];
};

#endif
