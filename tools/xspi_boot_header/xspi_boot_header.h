// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021 NXP
 */

#include <stdint.h>

#define EI_NIDENT 16
#define MAX_SG_ENTRIES 8
#define MAX_CF_WORDS 200
#define GEUL_RSVD 0
#define IBR_BIN_NAME "geul_ibr.bin"
#define GEUL_PREAMBLE 0xaa55aa55
#define XSPI_BASE_OFFSET 0x100000
#define GEUL_USE_QDMA 0
#define GEUL_USE_MEMCOPY 1

#define DMEM_HACK
#define PEB_END 0xE0380000

struct ibr_sg_table {
    uint32_t len;
    uint32_t resv;
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
    uint32_t sgentries;
    struct ibr_sg_table sgtbl[MAX_SG_ENTRIES];
    uint32_t bl_entry;
    uint32_t flags;
    uint32_t cfwords_count;
    struct ibr_cf_word cfword[MAX_CF_WORDS];
};

struct file_header {
	uint8_t  fh_ident[EI_NIDENT];
	uint16_t fh_type;
	uint16_t fh_machine;
	uint32_t fh_version;
	uint32_t fh_entry;
	uint32_t fh_phoff;
	uint32_t fh_shoff;
	uint32_t fh_flags;
	uint16_t fh_ehsize;
	uint16_t fh_phentsize;
	uint16_t fh_phnum;
	uint16_t fh_shentsize;
	uint16_t fh_shnum;
	uint16_t fh_shstrndx;
}__attribute__((__packed__));

struct program_header {
	uint32_t prg_type;            /* Segment type */
	uint32_t prg_offset;          /* Segment file offset */
	uint32_t prg_vaddr;           /* Segment virtual address */
	uint32_t prg_paddr;           /* Segment physical address */
	uint32_t prg_filesz;          /* Segment size in file */
	uint32_t prg_memsz;           /* Segment size in memory */
	uint32_t prg_flags;           /* Segment flags */
	uint32_t prg_align;           /* Segment alignment */
};
