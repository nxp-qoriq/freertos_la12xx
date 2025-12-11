/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2021-2022 NXP
 */

/* Entry Point */
ENTRY(_entry)

__DMEM_START = 0xE0000000;
__DMEM_SIZE  = 0x00010000;
__DMEM_END   = __DMEM_START + __DMEM_SIZE;

__STACK_SIZE = 0x00002C00; /* Per each e200 core, is taken from __DMEM_SIZE */

__SMEM_DATA_START = 0xE01F0000;
__SMEM_DATA_SIZE  = 0x00008000;
__SMEM_DATA_END   = __SMEM_DATA_START + __SMEM_DATA_SIZE;

__SMEM_TEXT_START = 0xE01F8000;
__SMEM_TEXT_SIZE  = 0x00008000;
__SMEM_TEXT_END   = __SMEM_TEXT_START + __SMEM_TEXT_SIZE;

__HIF_START = 0xE0200000;
__HIF_SIZE  = 0x00038010; /* IBR + HIF + MD + MEMPOOL */
__HIF_END   = __HIF_START + __HIF_SIZE;

__TEXT_START = __HIF_END;
__TEXT_SIZE  = 0x00038000;
__TEXT_END   = __TEXT_START + __TEXT_SIZE;

__RODATA_START = __TEXT_END;
__RODATA_SIZE  = 0x0000b000;
__RODATA_END   = __RODATA_START + __RODATA_SIZE;

__PEB_END = 0xE0380000; /* No address shall exceed this value (This is the VSPA Overlay area) */

__CSHARED_START = __RODATA_END;
__CSHARED_SIZE  = 0x00032000;
__CSHARED_END   = __CSHARED_START + __CSHARED_SIZE;

__SHARED_START = __CSHARED_END;
__SHARED_SIZE = __PEB_END - __SHARED_START;

__HEAP_SIZE = 0x00020000; /* For all 4 cores, will get split between cores at runtime and is taken from __SHARED_SIZE */

__HRAM_START = 0xE5100000; /* Offset 1 MB in FECA HRAM */
__HRAM_SIZE  = 0x00200000; /* 2 MB */

__FRAM_START = 0xE6000000; /* Offset 16 MB in FECA Memory */
__FRAM_SIZE  = 0x000C0000; /* 768 KB */

__CORE_SMEM_START = 0xE0A00000;
__CORE_SMEM_SIZE = 0x00040000; /* 256 KB */
__CORE_SMEM_END = __CORE_SMEM_START + __CORE_SMEM_SIZE;

__CORE_SMEM_TEXT_START = 0xE0A40000;
__CORE_SMEM_TEXT_SIZE  = 0x00040000; /* 256 KB */
__CORE_SMEM_TEXT_END   = __CORE_SMEM_TEXT_START + __CORE_SMEM_TEXT_SIZE;

MEMORY
{
    /* DMEM */
    dmem : org = __DMEM_START, len = __DMEM_SIZE

    /* SMEM */
    smem : org = __SMEM_DATA_START, len = __SMEM_DATA_SIZE

        /* SMEM_TEXT */
    smem_text : org = __SMEM_TEXT_START, len = __SMEM_TEXT_SIZE

    /* PEBM */
    hif    : org = __HIF_START,    len = __HIF_SIZE
    text   : org = __TEXT_START,   len = __TEXT_SIZE
    rodata : org = __RODATA_START, len = __RODATA_SIZE
    cshared : org = __CSHARED_START, len = __CSHARED_SIZE
    shared : org = __SHARED_START, len = __SHARED_SIZE

    /* HRAM */
    hram : org = __HRAM_START, len = __HRAM_SIZE

    /* FRAM */
    fram : org = __FRAM_START, len = __FRAM_SIZE

    /* CORE SMEM */
    core_smem : org = __CORE_SMEM_START, len = __CORE_SMEM_SIZE

    /* CORE_SMEM_TEXT */
    core_smem_text : org = __CORE_SMEM_TEXT_START, len = __CORE_SMEM_TEXT_SIZE


    other : org = 0x0, len = 0x4 /* Other symbols, not included in the e200 image */
}

/* Define output sections */
SECTIONS
{
    /* Nothing else shall exist bewteen the following output sections:
     * .sdata, .sdata2, .data, .bss. The MPU programming relies on this. */

    .sdata :
    {
        __dmem_start = .; /* see also __dmem_end */

        /* FreeRTOS .sdata area - FreeRTOS must be the only user of DMEM initialized data! */

        *.o(.sdata)
        *.o(.sdata.*)

        *.c.obj(.sdata.*)
    } > dmem

    .sdata2 :
    {
        /* FreeRTOS .sdata2 area - FreeRTOS must be the only user of DMEM initialized data! */

        *.o(.sdata2)
    } > dmem

    .data :
    {
        /* FreeRTOS .data area - FreeRTOS must be the only user of DMEM initialized data! */
        *.o(.got2)
        *.o(.data)
        *.o(.data.*)

        *crtbegin.o(.rela.got2)
        *crtbegin.o(.rela.data*)

	/* TODO get rid of these! */
	*.c.obj(.data.*)
    } > dmem

    .bss (NOLOAD) :
    {
        __bss_start = .;

        *crtbegin.o(.bss)
        *crtbegin.o(.iplt)
        *crtbegin.o(.rela.iplt)

        _heap_end_ptr = .; /* Overwrite at runtime with correct heap pointer in PEBM, per each core */
        *(.bss.heap_end.*)

        /* FreeRTOS .bss area - must come at the beginning! */
        *.o(.sbss)
        *.o(.sbss.*)
        *.o(.sbss2)
        *.o(.bss.*)

        /* BSP .bss area */
        *.c.obj(.sbss)
        *.c.obj(.sbss.*)
        *.c.obj(.bss.*)

        *.c.obj(COMMON)

        /* L1C .sbss*/
        *libl1c.a:*(.sbss*)
        /* L1C .bss*/
        *libl1c.a:*(.bss*)
        /* L1C COMMON */
        *libl1c.a:*(COMMON)

        __bss_end = .;

        _stack_end = .;
        . +=  __STACK_SIZE;
        _stack_addr = .;

        __dmem_end = .; /* see also __dmem_start */
    } > dmem

    .smem (NOLOAD) : ALIGN(16)
    {
        __smem_start = .;
        *(.smem)
        __smem_end = .;
    } > smem

    .smem_text : ALIGN(256)
    {
	    __smem_text_start = .;
	    src\\*(.smem_text*)
		    *(.smem_text)
		    *(.rela.smem_text)
		    __smem_text_end = .;
    } > smem_text

    .core_smem (NOLOAD) : ALIGN(16)
    {
	    __core_smem_start = .;
	    *(.core_smem)
	    __core_smem_end = .;
    } > core_smem

   .core_smem_text : ALIGN(256)
   {
	   __core_smem_text_start = .;
	   src\\*(.core_smem_text*)
	   *(.core_smem_text)
	   *(.rela.core_smem_text)
	   __core_smem_text_end = .;
   } > core_smem_text

    /* Nothing else shall exist bewteen the following output sections:
     * .hif.ibr, .hif.dbg, .hif. The MPU programming relies on this. */

    .hif.ibr (NOLOAD) : ALIGN(2048)
    {
        __hif_start = .;
        *.c.obj(.hif.ibr)
    } > hif

    .hif.dbg (NOLOAD) : ALIGN(16)
    {
        *libl1c.a:*(.hif.dbg)
    } > hif

    .hif (NOLOAD) : ALIGN(2048)
    {
        *.c.obj(.hif.start)
        *.c.obj(.hif)
        __hif_end = .;
    } > hif

    /* Nothing else shall exist bewteen the following output sections:
     * .entry, .intvec, .startup, .text, .ctors, .dtors. The MPU programming relies on this. */

    .entry : ALIGN(16)
    {
        __text_start = .; /* See also __text_end */
        __start = .;
        *(.entry)
    } > text

    .intvec : ALIGN(256)
    {
        *(.intvec)
        *(.rela.intvec)
    } > text

    .startup : ALIGN(256)
    {
        *(.startup)
        *(.rela.startup)
    } > text

    .text :
    {

        KEEP (*(.init))
        /* libc */
        *crtbegin.o(.eh_frame)
        *crtbegin.o(.glink)
        *crtbegin.o(.text)

        *.a:*(EXCLUDE_FILE(*libl1c.a*) .text*)

        *.S.obj(.text*)

        /* FreeRTOS & BSP */
        *.c.obj(.text.*)

        /* L1C */
        *libl1c.a:*(.text*)

        *crtbegin.o(.rela.text*)
        *crtend.o(.text)
        KEEP (*(.fini))
	KEEP (*(.tm_clone_table))
    } > text

    .ctors :
    {
      __CTOR_LIST__ = .;
      /* gcc uses crtbegin.o to find the start of
         the constructors, so we make sure it is
         first.  Because this is a wildcard, it
         doesn't matter if the user does not
         actually link against crtbegin.o; the
         linker won't look for a file to match a
         wildcard.  The wildcard also means that it
         doesn't matter which directory crtbegin.o
         is in.  */
      KEEP (*crtbegin.o(.ctors))
      KEEP (*crtbegin?.o(.ctors))
      /* We don't want to include the .ctor section from
         from the crtend.o file until after the sorted ctors.
         The .ctor section from the crtend file contains the
         end of ctors marker and it must be last */
      KEEP (*(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors))
      KEEP (*(SORT(.ctors.*)))
      KEEP (*(.ctors))
      __CTOR_END__ = .;
    } > text

    .dtors :
    {
      __DTOR_LIST__ = .;
      KEEP (*crtbegin.o(.dtors))
      KEEP (*crtbegin?.o(.dtors))
      KEEP (*(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors))
      KEEP (*(SORT(.dtors.*)))
      KEEP (*(.dtors))
      __DTOR_END__ = .;
    } > text
        
    .preinit_array :
    {
      PROVIDE_HIDDEN (__preinit_array_start = .);
      KEEP (*(.preinit_array*))
      PROVIDE_HIDDEN (__preinit_array_end = .);
    } > text

    .init_array :
    {
      PROVIDE_HIDDEN (__init_array_start = .);
      KEEP (*(SORT(.init_array.*)))
      KEEP (*(.init_array*))
      PROVIDE_HIDDEN (__init_array_end = .);
    } > text

    .fini_array :
    {
      PROVIDE_HIDDEN (__fini_array_start = .);
      KEEP (*(SORT(.fini_array.*)))
      KEEP (*(.fini_array*))
      PROVIDE_HIDDEN (__fini_array_end = .);
        __text_end = .; /* see also __text_start */
    } > text

    .rodata : ALIGN(16)
    {
        __rodata_start = .;

	KEEP (*(.jcr))

        /* libc */
	*crtbegin.o(.rela.rodata*)
        *.a:*(EXCLUDE_FILE(*libl1c.a*) .rodata*)

        /* FreeRTOS & BSP */
        *.c.obj(.rodata*)

        /* L1C */
        *libl1c.a:*(.rodata*)

        __rodata_end = .;
    } > rodata

    .cshared.bss (NOLOAD) : ALIGN(16)
    {
	    __cshdata_start = .;
	    __cshared_bss_start = .;

	    /* L1C .shared.sbss */
	    src\\*(.cshared.sbss)

	    /* L1C .shared.bss */
	    src\\*(.cshared.bss)

	    /* BSP .shared.bss */
	    drivers\\*(.cshared.bss)

	    __cshared_bss_end = .;
	    __cshdata_end = .;
    } > cshared

    /* Nothing else shall exist bewteen the following output sections:
     * .shared.data, .shared.bss. The MPU programming relies on this. */

    .shared.data : ALIGN(16)
    {
        __shdata_start = .; /* see also __shdata_end */
        *.c.obj(.shared.data)

        /* L1C .data */
        *libl1c.a:*(.shared.data)
    } > shared

    .shared.bss (NOLOAD) : ALIGN(16)
    {
        __shared_bss_start = .;

         end = .; /* initial ending of heap, for sbrk */
         __heap_start = .;
        . += __HEAP_SIZE;
         __heap_end = .;

        /* L1C .shared.sbss */
        *libl1c.a:*(.shared.sbss)

        /* BSP .shared.(s)bss */
        *.c.obj(.shared.sbss)
        *.c.obj(.shared.bss)

        /* L1C .shared.bss */
        *libl1c.a:*(.shared.bss)

        __shared_bss_end = .;

        __shdata_end = .; /* see also __shdata_start */
    } > shared

    .hram (NOLOAD) : ALIGN(16)
    {
        __hram_start = .;

        /* L1C trace buffers for all cores (ISR and non-ISR) */
        *libl1c.a:l1ca_trace.o(.hram.slot)
        *libl1c.a:l1ca_trace.o(.hram)
        *(.hram)

        __hram_end = .;
    } > hram

    .fram (NOLOAD) : ALIGN(16)
    {
        __fram_start = .;

        *(.fram)

        __fram_end = .;
    } > fram

/*-------- LABELS USED IN CODE -------------------------------*/

__bss_size = __bss_end - __bss_start;

__shared_bss_size = __shared_bss_end - __shared_bss_start;

__cshared_bss_size = __cshared_bss_end - __cshared_bss_start;

/* Exceptions Handlers Location (used for IVPR initialization) */
EXCEPTION_HANDLERS = ADDR(.intvec);
}
