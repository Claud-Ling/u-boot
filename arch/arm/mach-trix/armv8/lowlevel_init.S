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
#include "../macro.S"

ENTRY(lowlevel_init)
	ret
ENDPROC(lowlevel_init)

#ifdef CONFIG_DTV_SMC
/*
 * beware: this has been copied from
 * repo/hwdep_xos/xos/xboot4/armuk/boot/armuk_asm.S
 * Report any modification over there!
 * It doesn't follow SMC32 convention
 */
ENTRY(do_smc_call)
	mov	w12, w0
	mov	w0, w1
	mov	w1, w2
	smc	#0
	ret
ENDPROC(do_smc_call)
#endif

#ifdef CONFIG_DTV_BOOTPARAM
#include <asm/arch/bootparam.h>
/*
 * void save_boot_params(int arg0, int arg1)
 *  arg0 - bootparam ptr, arg2 - N/A
 *  r0, r2 - r5 corrupted
 */
ENTRY(save_boot_params)
#if 0
	/*
	 * See if the rom code passed pointer is valid:
	 * It is not valid if it is not in SRAM
	 * This may happen if you are booting with the help of
	 * debugger
	 */
	ldr     x2, =BOOTPARAM_POSITION
	cmp	x2, x0
	beq	3f

	/*
	 * See if tags are valid even pointer is invalid
	 * This may happen if you are booting with the help of
	 * debugger
	 */
	mov	x0, x2
	ldr	w3, =BOOTPARAM_HEAD_TAG
	ldr	w2, [r0, #BOOTPARAM_HEAD_OFS]
	cmp	w2, w3
	bne	1f
	ldr	w3, =BOOTPARAM_TAIL_TAG
	ldr	w2, [r0, #BOOTPARAM_TAIL_OFS]
	cmp	w2, w3
	bne	1f
3:
	/*
	 * store the boot params passed from rom code or saved
	 * and passed by boot1
	 */
	cmp	x0, #0
	beq	1f
	ldr	x2, =boot_params
	add	x3, x0, #BOOTPARAM_FRM_SIZE
2:
	ldp	x4, x5, [x0], #16
	sdp	x4, x5, [x2], #16
	cmp	x0, x3
	blo	2b

1:
#endif
	ret
ENDPROC(save_boot_params)
#endif
