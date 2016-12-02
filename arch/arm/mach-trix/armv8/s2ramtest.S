/*
 *  s2ramtest.S
 *
 *  Copyright (c) 2014 Sigma Designs Limited
 *   tony_he@sigmadesigns.com
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Suspend to RAM low level control interfaces for DTV Soc (SX6)
 */

#include <config.h>
#include "../asm-offsets.h"
#include "../s2ramctrl.h"
#include "../macro.S"

#define DEBUG

#ifdef DEBUG
.macro trace_set_resume_entry
	mov	r0, #0
	adr	r1, resume_entry_stage
	bl	trace_call
.endm

.macro trace_in_selfrefresh
	adr	r1, selfrefresh_stage
	bl	trace_call
.endm

.macro trace_system_poweroff
	adr	r1, poweroff_stage
	bl	trace_call
.endm

.macro trace_enter_wfi
	adr	r0, wfi_stage
	bl	puts_asm
.endm

.macro trace_error
	adr	r0, error_stage
	bl	puts_asm
.endm

.macro trace_resume
	adr	r0, resume_stage
	bl	puts_asm
.endm
#else
.macro trace_set_resume_entry
.endm

.macro trace_in_selfrefresh
.endm

.macro trace_system_poweroff
.endm

.macro trace_enter_wfi
.endm

.macro trace_error
.endm

.macro trace_resume
.endm
#endif

/*
 *int s2ramtest_finish_suspend(int stack, int state);
 * inputs:
 *	stack - new stack pointer
 *	state - suspend state, N/A
 * return:
 *	non-return or non-zero for error
 */
ENTRY(s2ramtest_finish_suspend)
	mov	x0, #1
	ret
ENDPROC(s2ramtest_finish_suspend)

do_s2ram_sram_addr:
	.word	do_s2ram_sram

/*
 *void s2ramtest_do_s2ram(int state);
 * inputs:
 *	state - suspend state, N/A
 * return: non-return
 */
.align 8
ENTRY(s2ramtest_do_s2ram)
	ret
ENDPROC(s2ramtest_do_s2ram)

#ifdef DEBUG
/*
 * r0 - return (0 - ok; other - nok)
 * r1 - ptr of string
 * output: string [ok] or [nok]
 */
ENTRY(trace_call)
	ret
ENDPROC(trace_call)

selfrefresh_stage: .ascii "sdram in selfrefresh\r\n\0"
.align 2
poweroff_stage: .ascii "power off\r\n\0"
.align 2
ok_stage: .ascii "[ok]\r\n\0"
.align 2
nok_stage: .ascii "[failed]\r\n\0"
.align 2
wfi_stage: .ascii "execute wfi\r\n\0"
.align 2
error_stage: .ascii "error\r\n\0"
.align 2
#endif

/*
 * prototype: puts_asm(char *)
 * assume serial port has been set up already
 */
ENTRY(puts_asm)
	ret
ENDPROC(puts_asm)

uart_reg_base:
	.word	0xfb005100

#include "s2ramctrl.S"

ENTRY(s2ramtest_do_s2ram_sz)
	.word   . - s2ramtest_do_s2ram
ENDPROC(s2ramtest_do_s2ram_sz)

#ifdef DEBUG
resume_entry_stage: .ascii "set resume entry\r\n\0"
.align 2
resume_stage: .ascii "resume uboot\r\n\0"
.align 2
#endif

/*
 * void s2ramtest_do_resume(void);
 */
ENTRY(s2ramtest_do_resume)
	ret
ENDPROC(s2ramtest_do_resume)

/*
 * void s2ramtest_set_entry(void* entry, void* mem0, unsigned len0, void* mem1, unsigned len1);
 * inputs:
 *	entry - resume entry, in physical addr
 *	mem0  - start addr of memory #0
 *	len0  - byte length of memory #0
 *	mem1  - start addr of memory #1
 *	len1  - byte length of memory #1
 * return: non-return
 */
ENTRY(s2ramtest_set_entry)
	ret
ENDPROC(s2ramtest_set_entry)

.global my_resume_frame
my_resume_frame:
	.rept	(S2RAM_FRAME_SIZE >> 2)
	.word	0x0badc0de
	.endr
	.size	my_resume_frame, . - my_resume_frame
