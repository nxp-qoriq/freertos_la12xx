// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021 NXP
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <endian.h>
#include "xspi_boot_header.h"

static int fw_read_and_make_header(
		uint8_t *file_start, uint32_t pg_hd_off, uint32_t ph_num, uint32_t fh_entry)
{
	int i = 0, ret = -1, prog_idx = 0;
	uint8_t  *str          = NULL;
	uint32_t program_offset = 0, program_size = 0, program_vma = 0;
	struct program_header *pg_header = NULL;
	FILE *fp;
    struct ibr_header ibr_hdr;

	if (file_start == NULL) {
		printf("Load sections: file start is NULL");
		return -1;
	}
    fp = fopen(IBR_BIN_NAME, "wb");
    memset(&ibr_hdr, 0, sizeof(struct ibr_header));
    ibr_hdr.preamble = htobe32(GEUL_PREAMBLE);

	pg_header = (struct program_header *)(file_start + pg_hd_off);
	str = (file_start + be32toh(pg_header[pg_hd_off].prg_offset));
	for (i = 0; i < ph_num; i++)
	{
	#if ELF_LOG
	    printf("PROGRAM 0x%x 0x%x 0x%x\
            0x%x 0x%x 0x%x\
            0x%x \n",
            pg_header[i].prg_type,      /* Section name (string tbl index) */
            pg_header[i].prg_offset,     /* Section type */
            pg_header[i].prg_vaddr,     /* Section flags */
            pg_header[i].prg_paddr,     /* Section virtual addr at execution */
            pg_header[i].prg_filesz, /* Section size in bytes */
            pg_header[i].prg_flags,     /* Link to another section */
            pg_header[i].prg_align      /* Additional section information */
            );
	#endif
            program_size = be32toh(pg_header[i].prg_filesz);
            program_vma = be32toh(pg_header[i].prg_vaddr);
            program_offset = be32toh(pg_header[i].prg_offset);
	#if ELF_LOG
	    printf("\n\rSection_size=%d\n",program_size);
	#endif
        /* skip zero size section */
        if (!program_size)
            continue;
        ibr_hdr.sgtbl[prog_idx].len = htobe32(program_size);
		ibr_hdr.sgtbl[prog_idx].resv = htobe32(GEUL_RSVD);
		ibr_hdr.sgtbl[prog_idx].src =
			htobe32((long int)XSPI_BASE_OFFSET + program_offset);
		ibr_hdr.sgtbl[prog_idx].dest = htobe32(program_vma);
		prog_idx++;
	}
    /* This hack is required As in FreeRTOS code,
    in flexspi boot, Dmem content of core 1,2 and 3
    are copied from PEBM.
    When the code is fixed to copy from Flexspi Memory
    This HACK needs to remove*/
    #ifdef DMEM_HACK
    ibr_hdr.sgtbl[prog_idx].len = pg_header[0].prg_filesz;
    ibr_hdr.sgtbl[prog_idx].resv = htobe32(GEUL_RSVD);
    ibr_hdr.sgtbl[prog_idx].src =
        htobe32((long int)XSPI_BASE_OFFSET + be32toh(pg_header[0].prg_offset));
    ibr_hdr.sgtbl[prog_idx].dest = htobe32(PEB_END);
    prog_idx++;
    #endif
    ibr_hdr.sgentries = htobe32(prog_idx);
    ibr_hdr.flags = htobe32(GEUL_USE_MEMCOPY);
    ibr_hdr.bl_entry = htobe32(fh_entry);
	#if ELF_LOG
    printf("Write IBR HEADER of %ld \n", sizeof(struct ibr_header));
    #endif
    /* Copy IBR header to BIN file */
    fwrite(&ibr_hdr, 1, sizeof(struct ibr_header), fp);
	printf("\n");
	return 0;
}

static int make_xspi_boot_header(char *vaddr)
{
	struct file_header *hd;

	/* FW Header Initialization*/
	hd = (struct file_header *) vaddr;
#if ELF_LOG
    printf("IDENT %s\n TYPE 0x%x\n MACHINE %d \nVERSION %d\n \
            ENTRY 0x%x\n PHOFF 0x%x\n SHOFF 0x%x\n\
            FLAGS 0x%x\n EHSIZE 0x%x PHENTSIZE 0x%x\n\
            PHNUM 0x%x SHENTSIZE 0x%x SHNUM 0x%x\n SHSTRINDX 0x%x\n",
            hd->fh_ident,
            be16toh(hd->fh_type),
            be16toh(hd->fh_machine),
            be32toh(hd->fh_version),
            be32toh(hd->fh_entry),
            be32toh(hd->fh_phoff),
            be32toh(hd->fh_shoff),
            be32toh(hd->fh_flags),
            be16toh(hd->fh_ehsize),
            be16toh(hd->fh_phentsize),
            be16toh(hd->fh_phnum),
            be16toh(hd->fh_shentsize),
            be16toh(hd->fh_shnum),
            be16toh(hd->fh_shstrndx));
	printf("\nProgram Head num=%d\n", be16toh(hd->fh_phnum));
#endif

	/*FW image parsing and load sections*/
	if (fw_read_and_make_header(vaddr, be32toh(hd->fh_phoff),be16toh(hd->fh_phnum), be32toh(hd->fh_entry))) {
		printf("ERR %s: Image Section Loading Failed",
						__func__);
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	char *geul_fw;
	struct stat geul_st;
	int fd = -1;
	if(argc!=2)
	{
		printf("\nWrong format");
		printf("\nCorrect Format:");
		printf("\n./elf_parse path_of_la12xx.elf");
	}
	fd = open(argv[1], O_RDWR, 0);
	if (fd == -1)
	{
	    printf("Error opening LA12XX ELF image");
        return -1;
    }
	fstat(fd, &geul_st);
	geul_fw = (char*)mmap(NULL, geul_st.st_size,
			PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    make_xspi_boot_header(geul_fw);
	return 0;
}
