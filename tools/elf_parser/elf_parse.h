// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020 NXP
 */

#include <stdint.h>
#define EI_NIDENT 16
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

struct section_header {
	uint32_t sec_name;      /* Section name (string tbl index) */
	uint32_t sec_type;      /* Section type */
	uint32_t sec_flags;     /* Section flags */
	uint32_t sec_addr;      /* Section virtual addr at execution */
	uint32_t sec_offset;    /* Section file offset */
	uint32_t sec_size;      /* Section size in bytes */
	uint32_t sec_link;      /* Link to another section */
	uint32_t sec_info;      /* Additional section information */
	uint32_t sec_addralign; /* Section alignment */
	uint32_t sec_entsize;   /* Entry size if section holds table */
};

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

