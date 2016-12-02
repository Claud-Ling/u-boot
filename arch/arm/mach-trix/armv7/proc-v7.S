/*
 *  linux/arch/arm/mm/proc-v7.S
 *
 *  Copyright (C) 2001 Deep Blue Solutions Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This is the "shell" of the ARMv7 processor support.
 */

#include "../macro.S"

/* Suspend/resume support: derived from arch/arm/mach-s5pv210/sleep.S */
.globl	cpu_v7_suspend_size
.equ	cpu_v7_suspend_size, 4 * 8

.equ	PRRR, 0x034
.equ	NMRR, 0x038

ENTRY(cpu_v7_do_suspend)
	stmfd	sp!, {r4 - r10, lr}
	mrc	p15, 0, r4, c13, c0, 0	@ FCSE/PID
	mrc	p15, 0, r5, c13, c0, 3	@ User r/o thread ID
	stmia	r0!, {r4 - r5}
	mrc	p15, 0, r6, c3, c0, 0	@ Domain ID
	mrc	p15, 0, r7, c2, c0, 1	@ TTB 1
	mrc	p15, 0, r11, c2, c0, 2	@ TTB control register
	mrc	p15, 0, r8, c1, c0, 0	@ Control register
	mrc	p15, 0, r9, c1, c0, 1	@ Auxiliary control register
	mrc	p15, 0, r10, c1, c0, 2	@ Co-processor access control
	stmia	r0, {r6 - r11}
	ldmfd	sp!, {r4 - r10, pc}
ENDPROC(cpu_v7_do_suspend)

ENTRY(cpu_v7_do_resume)
	mov	ip, #0
	mcr	p15, 0, ip, c8, c7, 0	@ invalidate TLBs
	mcr	p15, 0, ip, c7, c5, 0	@ invalidate I cache
	mcr	p15, 0, ip, c13, c0, 1	@ set reserved context ID
	ldmia	r0!, {r4 - r5}
	mcr	p15, 0, r4, c13, c0, 0	@ FCSE/PID
	mcr	p15, 0, r5, c13, c0, 3	@ User r/o thread ID
	ldmia	r0, {r6 - r11}
	mcr	p15, 0, r6, c3, c0, 0	@ Domain ID
	mcr	p15, 0, r1, c2, c0, 0	@ TTB 0
	mcr	p15, 0, r7, c2, c0, 1	@ TTB 1
	mcr	p15, 0, r11, c2, c0, 2	@ TTB control register
	mrc	p15, 0, r4, c1, c0, 1	@ Read Auxiliary control register
	teq	r4, r9			@ Is it already set?
	mcrne	p15, 0, r9, c1, c0, 1	@ No, so write it
	mcr	p15, 0, r10, c1, c0, 2	@ Co-processor access control
	ldr	r4, =PRRR		@ PRRR
	ldr	r5, =NMRR		@ NMRR
	mcr	p15, 0, r4, c10, c2, 0	@ write PRRR
	mcr	p15, 0, r5, c10, c2, 1	@ write NMRR
	isb
	dsb
	mov	r0, r8			@ control register
	b	cpu_resume_nommu
ENDPROC(cpu_v7_do_resume)

