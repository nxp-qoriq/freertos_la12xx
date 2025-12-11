/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2020-2024 NXP
 */

/* Entry Point */
ENTRY(_entry)

__DMEM_START = 0xE0000000;
__DMEM_SIZE  = 0x00010000; /* (64K) */
__DMEM_END   = __DMEM_START + __DMEM_SIZE;

__STACK_SIZE = 0x00002000; /* (8K) Per each e200 core, is taken from __DMEM_SIZE */

__SMEM_DATA_START = 0xE01F0000;
__SMEM_DATA_SIZE  = 0x00008000;
__SMEM_DATA_END   = __SMEM_DATA_START + __SMEM_DATA_SIZE;

__SMEM_TEXT_START = 0xE01F8000;
__SMEM_TEXT_SIZE  = 0x00008000;
__SMEM_TEXT_END   = __SMEM_TEXT_START + __SMEM_TEXT_SIZE;

__HIF_START = 0xE0200000;
__HIF_SIZE  = 0x000DA000; /* (872K) HIF + MD + MEMPOOL */
__HIF_END   = __HIF_START + __HIF_SIZE;

__TEXT_START = __HIF_END;
__TEXT_SIZE  = 0x00040000; /* (256K) */
__TEXT_END   = __TEXT_START + __TEXT_SIZE;

__RODATA_START = __TEXT_END;
__RODATA_SIZE  = 0x00020000; /* (128K) */
__RODATA_END   = __RODATA_START + __RODATA_SIZE;

__PEB_END = 0xE0380000; /* No address shall exceed this value (This is the VSPA Overlay area) */

__SHARED_START = __RODATA_END;
__SHARED_SIZE = __PEB_END - __SHARED_START;

/* Optional Data area, when VSPA not using overlay, application can use it for data */
__PEB_OVERLAY_END = 0xE0400000;
__OVERLAY_DATA_START = __PEB_END;
__OVERLAY_DATA_END = __PEB_OVERLAY_END;
__OVERLAY_DATA_SIZE = __OVERLAY_DATA_END - __OVERLAY_DATA_START; /* 512K */

__HEAP_SIZE = 0x00030000; /* (192K) For all cores, will get split between cores at runtime and is taken from __SHARED_SIZE */

__HRAM_START = 0xE5100000; /* Offset 1 MB in FECA HRAM */
__HRAM_SIZE  = 0x00500000; /* 5 MB */

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

        /* OVERLAY_DATA */
    ov : org = __OVERLAY_DATA_START, len = __OVERLAY_DATA_SIZE

    /* PEBM */
    hif    : org = __HIF_START,    len = __HIF_SIZE
    text   : org = __TEXT_START,   len = __TEXT_SIZE
    rodata : org = __RODATA_START, len = __RODATA_SIZE
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
        *.c.obj(.data)
        *.c.obj(.data.*)
    } > dmem

    .bss (NOLOAD) :
    {
        __bss_start = .;

        *crtbegin.o(.iplt)
        *crtbegin.o(.rela.iplt)

        _heap_end_ptr = .; /* Overwrite at runtime with correct heap pointer in PEBM, per each core */
        *sbrk.o(.bss.heap_end.*)

        /* FreeRTOS .bss area - must come at the beginning! */
        *.o(.sbss)
        *.o(.sbss.*)
        *.o(.sbss2)
        *.o(.bss.*)

        *.c.obj(.sbss)
        *.c.obj(.sbss.*)
        *.c.obj(.bss.*)

        *.c.obj(COMMON)

        __bss_end = .;

        _stack_end = .;
        . +=  __STACK_SIZE;
        _stack_addr = .;

        __dmem_end = .; /* see also __dmem_start */
    } > dmem

    .smem : ALIGN(16)
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

    .ov.data : ALIGN(16)
    {
        __ov_data_start = .; /* see also __ov_data_end */
        *.c.obj(.ov.data)
    } > ov
    
    .ov.bss (NOLOAD) : ALIGN(16)
    {
        __ov_bss_start = .;

        *.c.obj(.ov.sbss)
        *.c.obj(.ov.bss)

        __ov_bss_end = .;

        __ov_data_end = .; /* see also __ov_data_start */
    } > ov

    /* Nothing else shall exist bewteen the following output sections:
     * .hif.ibr, .hif. The MPU programming relies on this. */

    .hif.ibr (NOLOAD) : ALIGN(16)
    {
        __hif_start = .;
        *.c.obj(.hif.ibr)
    } > hif

    .hif (NOLOAD) : ALIGN(16)
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

        *.a:*(.text*)
        *.S.obj(.text*)

        /* FreeRTOS & BSP */
        *.c.obj(.text.*)

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
        *.a:*(.rodata*)

        /* FreeRTOS & BSP */
        *.c.obj(.rodata*)

        __rodata_end = .;
    } > rodata

    /* Nothing else shall exist bewteen the following output sections:
     * .shared.data, .shared.bss. The MPU programming relies on this. */

    .shared.data : ALIGN(16)
    {
        __shdata_start = .; /* see also __shdata_end */
        *.c.obj(.shared.data)
    } > shared

    .shared.bss (NOLOAD) : ALIGN(16)
    {
        __shared_bss_start = .;

         end = .; /* initial ending of heap, for sbrk */
         __heap_start = .;
        . += __HEAP_SIZE;
         __heap_end = .;

        *.c.obj(.shared.sbss)
        *.c.obj(.shared.bss)

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

/* Exceptions Handlers Location (used for IVPR initialization) */
EXCEPTION_HANDLERS = ADDR(.intvec);
}
