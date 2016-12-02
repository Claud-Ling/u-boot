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
.globl	cpu_v8_suspend_size
.equ	cpu_v8_suspend_size, 4 * 8

.equ	PRRR, 0x034
.equ	NMRR, 0x038

ENTRY(cpu_v8_do_suspend)
	/*TODO*/
	ret
ENDPROC(cpu_v8_do_suspend)

ENTRY(cpu_v8_do_resume)
	/*TODO*/
	b	cpu_resume_nommu
ENDPROC(cpu_v8_do_resume)

