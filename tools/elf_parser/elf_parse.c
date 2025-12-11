// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <endian.h>
#include "elf_parse.h"
//#define ELF_LOG 1
#define BIN_DIR "./release_tmp/"
char elf_name[100]={0};

static int fw_read_and_load_sections(
		uint8_t *file_start, uint32_t pg_hd_off,
		uint32_t sec_header_off, int sec_cnt, int str_section_off,
		uint32_t fw_size, uint32_t ph_num)
{
	int i = 0, ret = -1, overlay_link_add = 1;
	int8_t    sec_type     = 0;
	uint32_t  axi_align    = 0;
	uint32_t *section_ptr  = NULL;
	uint8_t  *str          = NULL;
	uint32_t  section_size = 0, section_vma = 0;
	uint32_t  section_lma  = 0;
	struct section_header *sec_header = NULL;
	struct program_header *pg_header = NULL;
	uint8_t  section_parsed = 0x0;
	uint32_t axi_data_width;
	char *section_name;
	FILE *fp;
	char file_name[256]={0};

	if (file_start == NULL) {
		printf("Load sections: file start is NULL");
		return -1;
	}

	pg_header = (struct program_header *)(file_start + pg_hd_off);
#if ELF_LOG
	  printf("\n Program Section count=%d sec_header_off=0x%x pg_header_off=0x%x str_section_off=%d\n",sec_header_off,pg_hd_off, str_section_off);	
	  printf("PROGRAM 0x%x 0x%x 0x%x\
            0x%x 0x%x 0x%x\
            0x%x \n",
            pg_header->prg_type,      /* Section name (string tbl index) */
            pg_header->prg_offset,     /* Section type */
            pg_header->prg_vaddr,     /* Section flags */
            pg_header->prg_paddr,     /* Section virtual addr at execution */
            pg_header->prg_filesz, /* Section size in bytes */
            pg_header->prg_flags,     /* Link to another section */
            pg_header->prg_align      /* Additional section information */
            );

#endif
	/*Fetching Section header from firmware file */
	sec_header = (struct section_header *)(file_start + pg_hd_off);
#if ELF_LOG
	printf("INFO %s: Program Sections Count: %d\n", __func__, ph_num);
#endif
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
            section_size = be32toh(pg_header[i].prg_filesz);
	#if ELF_LOG
	    printf("\n\rSection_size=%d\n",section_size);
	#endif
        /* skip zero size section */
        if (!section_size)
            continue;

		/* Write  sections to different file */
	            sprintf(file_name, 
        	            "%s%s%s%d",
                	    BIN_DIR,elf_name,".bin.",
	                    i );
	
	    	    printf("\nCreating %s",file_name);
        	    fp = fopen(file_name, "wb");
		
	            section_ptr = (uint32_t *)(file_start + be32toh(pg_header[i].prg_offset));
        	    fwrite(section_ptr, 1, section_size, fp);
		fclose(fp);
	}
	printf("\n");
	return 0;
}

static int gul_load_geul_image(char *vaddr, int geul_fw_size)
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
	if (fw_read_and_load_sections(vaddr, be32toh(hd->fh_phoff),
				be32toh(hd->fh_shoff), be16toh(hd->fh_shnum), be16toh(hd->fh_shstrndx),
				geul_fw_size, be16toh(hd->fh_phnum))) {
		printf("ERR %s: Image Section Loading Failed",
						__func__);
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	char *token, *last_occurance;
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
	if (fstat(fd, &geul_st))
	{
		printf("\nfstat error");
		close(fd);
		return -1;
	}
	geul_fw = (char*)mmap(NULL, geul_st.st_size,
			PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	last_occurance=strrchr(argv[1],'/');
#if ELF_LOG
	printf("\nlast occurance=%s", last_occurance+1);
#endif
	/* get the first token */
	token = strtok(last_occurance+1, ".");
#if ELF_LOG
	printf("\nelf=%s",token);
#endif
	strncpy(elf_name, token, sizeof(elf_name)-1);

	gul_load_geul_image(geul_fw, geul_st.st_size);
	return 0;
}
