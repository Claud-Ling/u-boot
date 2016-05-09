/*
 * SoC-specific setup info
 *
 * (C) Copyright 2010,2011
 * NVIDIA Corporation <www.nvidia.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>

.globl lowlevel_init
lowlevel_init:
        mov    pc, lr


#ifdef CONFIG_DTV_SMC
/*
 * beware: this has been copied from
 * repo/hwdep_xos/xos/xboot4/armuk/boot/armuk_asm.S
 * Report any modification over there!
 */
.globl do_smc_call
do_smc_call:
       push    {r4-r12, lr}
       mov     ip, r0
       mov     r0, r1
       mov     r1, r2
       .word   0xe1600070      @ SMC #0 - hand assembled because -march=armv5
                               @ call ROM Code API to set control register
       pop     {r4-r12, pc}

#endif

#ifdef CONFIG_DTV_BOOTPARAM
#include <asm/arch/bootparam.h>
/*
 * void save_boot_params(int arg0, int arg1)
 *  arg0 - bootparam ptr, arg2 - N/A
 *  r0, r2 - r5 corrupted
 */
.global save_boot_params
save_boot_params:
	/*
	 * See if the rom code passed pointer is valid:
	 * It is not valid if it is not in SRAM
	 * This may happen if you are booting with the help of
	 * debugger
	 */
	ldr     r2, =BOOTPARAM_POSITION
	cmp	r2, r0
	beq	3f

	/*
	 * See if tags are valid even pointer is invalid
	 * This may happen if you are booting with the help of
	 * debugger
	 */
	mov	r0, r2
	ldr	r3, =BOOTPARAM_HEAD_TAG
	ldr	r2, [r0, #BOOTPARAM_HEAD_OFS]
	cmp	r2, r3
	bne	1f
	ldr	r3, =BOOTPARAM_TAIL_TAG
	ldr	r2, [r0, #BOOTPARAM_TAIL_OFS]
	cmp	r2, r3
	bne	1f
3:
	/*
	 * store the boot params passed from rom code or saved
	 * and passed by boot1
	 */
	cmp	r0, #0
	beq	1f
	ldr	r2, =boot_params
	add	r3, r0, #BOOTPARAM_FRM_SIZE
2:
	ldmia	r0!, {r4-r5}
	stmia   r2!, {r4-r5}
	cmp	r0, r3
	blo	2b

1:
	b	save_boot_params_ret
#endif
