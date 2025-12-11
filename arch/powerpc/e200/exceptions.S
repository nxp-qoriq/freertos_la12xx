// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2014-2017 Freescale Semiconductor, Inc.
 * Copyright 2019-2021 NXP
 */

    .section    .intvec, "ax"
    .global gInterruptVectorTable, gInterruptVectorTableEnd

    .extern vPortISRHandler
    .extern vPortDataStorage
    .extern vPortProgramHandle
    .extern vPortAlignment
    .extern vPortCritical
    .extern vPortMachineCheck
    .extern vPortInstructionStore
    .extern vPortPerformanceMonitor
    .extern vPortDebugHandler
    .extern vPortEFPHandler
    .extern vPortEFPRoundHandler


##############################################################################
#
#   isr_prologue
#
#   Saves the necessary registers for an interrupt service routine
#
##############################################################################

.macro isr_prologue
                e_stwu     r1,-80(r1)
                e_stw      r0,8(r1)
                se_mfctr   r0
                e_stw      r0,12(r1)
                mfspr      r0, 1
                e_stw      r0,16(r1)
                mfcr       r0
                e_stw      r0,20(r1)
                se_mflr    r0
                e_stw      r0,24(r1)
                e_stw      r3,40(r1)
                e_stw      r4,44(r1)
                e_stw      r5,48(r1)
                e_stw      r6,52(r1)
                e_stw      r7,56(r1)
                e_stw      r8,60(r1)
                e_stw      r9,64(r1)
                e_stw      r10,68(r1)
                e_stw      r11,72(r1)
                e_stw      r12,76(r1)
                mfsrr0     r0
                e_stw      r0,28(r1)
                mfsrr1     r0
                e_stw      r0,32(r1)
.endm


##############################################################################
#
#   isr_epilogue
#
#   Restores the necessary registers for an interrupt service routine
#
##############################################################################

.macro isr_epilogue
                e_lwz      r0,32(r1)
                mtsrr1     r0
                e_lwz      r0,28(r1)
                mtsrr0     r0
                e_lwz      r3,40(r1)
                e_lwz      r4,44(r1)
                e_lwz      r5,48(r1)
                e_lwz      r6,52(r1)
                e_lwz      r7,56(r1)
                e_lwz      r8,60(r1)
                e_lwz      r9,64(r1)
                e_lwz      r10,68(r1)
                e_lwz      r11,72(r1)
                e_lwz      r12,76(r1)
                e_lwz      r0,24(r1)
                se_mtlr    r0
                e_lwz      r0,20(r1)
                mtcrf      0xff,r0
                e_lwz      r0,16(r1)
                mtspr      1, r0
                e_lwz      r0,12(r1)
                mtctr      r0
                e_lwz      r0,8(r1)
                e_addi     r1,r1,80
                se_rfi
.endm

##############################################################################
#
#   cisr_prologue
#
#   Saves the necessary registers for a critical interrupt service routine
#
##############################################################################

.macro cisr_prologue
                e_stwu     r1,-80(r1)
                e_stw      r0,8(r1)
                se_mfctr   r0
                e_stw      r0,12(r1)
                mfspr      r0, 1
                e_stw      r0,16(r1)
                mfcr       r0
                e_stw      r0,20(r1)
                se_mflr    r0
                e_stw      r0,24(r1)
                e_stw      r3,40(r1)
                e_stw      r4,44(r1)
                e_stw      r5,48(r1)
                e_stw      r6,52(r1)
                e_stw      r7,56(r1)
                e_stw      r8,60(r1)
                e_stw      r9,64(r1)
                e_stw      r10,68(r1)
                e_stw      r11,72(r1)
                e_stw      r12,76(r1)
                mfspr      r0,58
                e_stw      r0,28(r1)
                mfspr      r0,59
                e_stw      r0,32(r1)
.endm


##############################################################################
#
#   cisr_epilogue
#
#   Restores the necessary registers for a critical interrupt service routine
#
##############################################################################

.macro cisr_epilogue
                e_lwz      r0,32(r1)
                mtspr      59,r0
                e_lwz      r0,28(r1)
                mtspr      58,r0
                e_lwz      r3,40(r1)
                e_lwz      r4,44(r1)
                e_lwz      r5,48(r1)
                e_lwz      r6,52(r1)
                e_lwz      r7,56(r1)
                e_lwz      r8,60(r1)
                e_lwz      r9,64(r1)
                e_lwz      r10,68(r1)
                e_lwz      r11,72(r1)
                e_lwz      r12,76(r1)
                e_lwz      r0,24(r1)
                se_mtlr    r0
                e_lwz      r0,20(r1)
                mtcrf      0xff,r0
                e_lwz      r0,16(r1)
                mtspr      1, r0
                e_lwz      r0,12(r1)
                mtctr      r0
                e_lwz      r0,8(r1)
                e_addi     r1,r1,80
                se_rfci
.endm

gInterruptVectorTable:

.org    0x000
e_b     vPortCritical
.org    0x010
e_b     vPortMachineCheck
.org    0x020
e_b     vPortDataStorage
.org    0x030
e_b     vPortInstructionStore
.org    0x040
e_b     vPortISRHandler
.org    0x050
e_b     vPortAlignment
.org    0x060
e_b     vPortProgramHandle
.org    0x070
e_b     vPortPerformanceMonitor
.org    0x080
e_b 	xPortSyscall
.org    0x090
e_b     vPortDebugHandler
.org    0x0A0
e_b     vPortEFPHandler
.org    0x0B0
e_b     vPortEFPRoundHandler

##############################################################################
#
#   Critical Input
#
##############################################################################
CriticalInput_Handler:

        cisr_prologue

        e_li    r3, 0x0000
        e_lis   r4, InterruptHandler@h
        e_or2i  r4, InterruptHandler@l
        se_mtlr r4
        se_blrl

        cisr_epilogue


##############################################################################
#
#   Machine Check
#
##############################################################################
MachineCheck_Handler:

        e_stwu     r1,-80(r1)
        e_stw      r0,8(r1)
        se_mfctr   r0
        e_stw      r0,12(r1)
        mfspr      r0, 1
        e_stw      r0,16(r1)
        mfcr       r0
        e_stw      r0,20(r1)
        se_mflr    r0
        e_stw      r0,24(r1)
        e_stw      r3,40(r1)
        e_stw      r4,44(r1)
        e_stw      r5,48(r1)
        e_stw      r6,52(r1)
        e_stw      r7,56(r1)
        e_stw      r8,60(r1)
        e_stw      r9,64(r1)
        e_stw      r10,68(r1)
        e_stw      r11,72(r1)
        e_stw      r12,76(r1)
        mfspr      r0,570
        e_stw      r0,28(r1)
        mfspr      r0,571
        e_stw      r0,32(r1)

        e_li    r3, 0x0010
        e_lis   r4, InterruptHandler@h
        ori     r4, r4, InterruptHandler@l
        se_mtlr r4
        se_blrl

        e_lwz      r0,32(r1)
        mtspr      571,r0
        e_lwz      r0,28(r1)
        mtspr      570,r0
        e_lwz      r3,40(r1)
        e_lwz      r4,44(r1)
        e_lwz      r5,48(r1)
        e_lwz      r6,52(r1)
        e_lwz      r7,56(r1)
        e_lwz      r8,60(r1)
        e_lwz      r9,64(r1)
        e_lwz      r10,68(r1)
        e_lwz      r11,72(r1)
        e_lwz      r12,76(r1)
        e_lwz      r0,24(r1)
        se_mtlr    r0
        e_lwz      r0,20(r1)
        mtcrf      0xff,r0
        e_lwz      r0,16(r1)
        mtspr      1, r0
        e_lwz      r0,12(r1)
        mtctr      r0
        e_lwz      r0,8(r1)
        e_addi     r1,r1,80
        se_rfmci


##############################################################################
#
#   Data Storage
#
##############################################################################
DataStorage_Handler:

        isr_prologue

        e_li    r3, 0x0020
        e_lis   r4, InterruptHandler@h
        e_or2i  r4, InterruptHandler@l
        se_mtlr r4
        se_blrl

        isr_epilogue

##############################################################################
#
#   Instruction Storage
#
##############################################################################
InstructionStorage_Handler:

        isr_prologue

        e_li    r3, 0x0030
        e_lis   r4, InterruptHandler@h
        e_or2i  r4, InterruptHandler@l
        se_mtlr r4
        se_blrl

        isr_epilogue

##############################################################################
#
#   Alignment
#
##############################################################################
Alignment_Handler:

        isr_prologue

        e_li    r3, 0x0050
        e_lis   r4, InterruptHandler@h
        e_or2i  r4, InterruptHandler@l
        se_mtlr r4
        se_blrl

        isr_epilogue

##############################################################################
#
#   Program
#
##############################################################################
Program_Handler:

        isr_prologue

        e_li    r3, 0x0060
        e_lis   r4, InterruptHandler@h
        e_or2i  r4, InterruptHandler@l
        se_mtlr r4
        se_blrl

        isr_epilogue

##############################################################################
#
#   Performance Monitor exception
#
##############################################################################
PerformanceMonitor_Handler:

        isr_prologue

        e_li    r3, 0x0070
        e_lis   r4, InterruptHandler@h
        e_or2i  r4, InterruptHandler@l
        se_mtlr r4
        se_blrl

        isr_epilogue

##############################################################################
#
#   Debug
#
##############################################################################
Debug_Handler:

        cisr_prologue

        e_li    r3, 0x0090
        e_lis   r4, InterruptHandler@h
        e_or2i  r4, InterruptHandler@l
        se_mtlr r4
        se_blrl

        cisr_epilogue

##############################################################################
#
#   Embedded Floating-point Data
#
##############################################################################
EmbeddedFPData_Handler:

        isr_prologue

        e_li    r3, 0x00A0
        e_lis   r4, InterruptHandler@h
        e_or2i  r4, InterruptHandler@l
        se_mtlr r4
        se_blrl

        isr_epilogue

##############################################################################
#
#   Embedded Floating-point Round
#
##############################################################################
EmbeddedFPRound_Handler:

        isr_prologue

        e_li    r3, 0x00B0
        e_lis   r4, InterruptHandler@h
        e_or2i  r4, InterruptHandler@l
        se_mtlr r4
        se_blrl

        isr_epilogue

gInterruptVectorTableEnd:
