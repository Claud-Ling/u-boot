#
# (C) Copyright 2000-2010
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# SPDX-License-Identifier:	GPL-2.0+
#

/* beware: this has been copied from
 * repo/hwdep_xos/xos/xboot4/armuk/boot/armuk_asm.S
 * Report any modification over there! */
.globl do_smc_call
do_smc_call:
       push    {r4-r12, lr}
       mov     ip, r0
       mov     r0, r1
       mov     r1, r2
       .word   0xe1600070      @ SMC #0 - hand assembled because -march=armv5
                               @ call ROM Code API to set control register
       pop     {r4-r12, pc}



