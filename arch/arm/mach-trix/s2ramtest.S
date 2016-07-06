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
#include "s2ramctrl.h"
#include "asm-offsets.h"
#include "macro.S"

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
	stmfd	sp!, {r4 - r5, lr}
	mov	r5, r0
	mov	r4, r1
	bl	flush_dcache_all

	mov	r4, sp
	mov	sp, r5
	bic     sp, sp, #7      @8-byte alignment for ABI compliance
	stmfd	sp!, {r4}

	ldr	r1, do_s2ram_sram_addr
	ldr	r1, [r1]
	cmp	r1, #0
	blxne	r1

	mov	r0, #1			@fail
	ldmfd	sp!, {r4}
	mov	sp, r4
	ldmfd	sp!, {r4 - r5, pc}
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
	stmfd	sp!, {r4, lr}
	nop
	.word 0xf57ff04f	@armv5 doesn't support dsb
	nop
	.word 0xf57ff06f	@armv5 doesn't support isb
	bl	s2ramctl_sdram_in_selfrefresh
	trace_in_selfrefresh
	cmp	r0, #0
	bne	10f
	nop
	bl	s2ramctl_system_poweroff
	trace_system_poweroff
	trace_enter_wfi
	nop
	.word 0xf57ff04f	@armv5 doesn't support dsb
	nop
	.word 0xe320f003	@armv5 doesn't support wfi
	nop
1:	b	1b		@trap, execution shall not come here
	nop
10:
	trace_error
	ldmfd	sp!, {r4, pc}
ENDPROC(s2ramtest_do_s2ram)

#ifdef DEBUG
/*
 * r0 - return (0 - ok; other - nok)
 * r1 - ptr of string
 * output: string [ok] or [nok]
 */
ENTRY(trace_call)
	stmfd   sp!, {r0, lr}
	cmp	r0, #0
	bne	1f
	mov	r0, r1
	bl	puts_asm
	adr	r0, ok_stage
	bl	puts_asm
	b	2f
1:
	mov	r0, r1
	bl	puts_asm
	adr	r0, nok_stage
	bl	puts_asm
2:
	ldmfd   sp!, {r0, pc}
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
	stmfd	sp!, {r1 - r3, lr}
	ldr 	r3, uart_reg_base

1:
	ldrb 	r1, [r0], #1		@post index
	strb 	r1, [r3]

	/*check it tx ready*/
2:
	ldrb 	r2, [r3, #5]
	tst	r2, #0x20
	beq 	2b

	/*check end of string*/
	teq	r1, #0
	bne 	1b

	ldmfd	sp!, {r1 - r3, pc}
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
	trace_resume
	ldr	r4, =CONFIG_SYS_TEXT_BASE
	mov	r0, #0
	mov	r1, #0
	bx	r4
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
	stmfd	sp!, {r0, r1, r4, lr}
	ldr	r4, [sp, #16]
#ifdef CONFIG_S2RAM_CHECKSUM
	str	r4, [sp]
	adr	r4, my_resume_frame
	str	r4, [sp, #4]
	bl	s2ram_set_frame
#else
	adr	r1, my_resume_frame
	str	r0, [r1]

	/*flush cache...*/
	adr	r0, my_resume_frame
	mov	r1, #S2RAM_FRAME_SIZE
	bl	flush_cache
#endif
	adr	r0, my_resume_frame	@ should be physical addr
	bl      s2ramctl_set_resume_entry
	trace_set_resume_entry
	ldmfd	sp!, {r1, r2, r4,  pc}
ENDPROC(s2ramtest_set_entry)

.global my_resume_frame
my_resume_frame:
	.rept	(S2RAM_FRAME_SIZE >> 2)
	.word	0x0badc0de
	.endr
	.size	my_resume_frame, . - my_resume_frame
