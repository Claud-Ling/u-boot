/*
 *  s2ramctrl.S
 *
 *  Copyright (c) 2014 Sigma Designs Limited
 *   tony_he@sigmadesigns.com
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Suspend to RAM low level control interfaces for DTV Soc (SX6/SX7)
 *  It supports up to 3 umacs at most by now
 *
 */

#if defined(__PREBOOT__)
# include <config.h>
# include <assembler.h>
# include <generated/asm-offsets.h>
# include <umac.h>
# define __CONFIG_SRAM_RESUME__
# define trace_str	bl	serial_puts
# define trace_int	bl	put32
# define trace_ch	bl	serial_putc
#elif defined(__UBOOT__)
# include <config.h>
# include "../asm-offsets.h"
# include <asm/arch/umac.h>
# define __CONFIG_SRAM_SUSPEND__
# define trace_str
# define trace_int
# define trace_ch
#elif defined(__LINUX__)
# include <linux/linkage.h>
# include <asm/assembler.h>
# include "asm-offsets.h"
# include "umac.h"
# define __CONFIG_SRAM_SUSPEND__
# define trace_str
# define trace_int
# define trace_ch
#endif

#include "../umac_dev.inc"

#ifndef CONFIG_SIGMA_NR_UMACS
	#error "CONFIG_SIGMA_NR_UMACS is NOT set"
#elif CONFIG_SIGMA_NR_UMACS > 3
	#error "CONFIG_SIGMA_NR_UMACS exceeds max number (3)"
#endif


/*
 * common compiler options
 */
#define MCUCOMM_SUPPORT_TIMEOUT
//#define HOST_CTRL_STB_GPIO

/*
 * command from host to mcu
 */
#define HOST_CMD_RMODE_ENABLE	0x75
#define HOST_CMD_POWEROFF	0x76
#define HOST_CMD_RMODE_DISABLE	0x77

/*
 * command from mcu to host
 */
#define MCU_CMD_POWERMODE	0x90
#define MCU_CMD_ACK_RMODE_ON	0x91
#define MCU_CMD_ACK_RMODE_OFF	0x92

/*
 * MCU registers
 */
#define MCU_REG_BASE		0xf5000000
#define MCU_REG_GPIO_OFS	0xa

/*
 * share reg M0~M3
 *  HOST R/W, MCU RO
 */
#define SHREG_MBASE_OFS 0x10
#define SHREG_M0_OFS SHREG_MBASE_OFS
#define SHREG_M1_OFS (SHREG_MBASE_OFS + 1)
#define SHREG_M2_OFS (SHREG_MBASE_OFS + 2)
#define SHREG_M3_OFS (SHREG_MBASE_OFS + 3)

/*
 * share reg E0~E5
 *  MCU R/W, HOST RO
 */
#define SHREG_EBASE_OFS 0x30
#define SHREG_E0_OFS SHREG_EBASE_OFS
#define SHREG_E1_OFS (SHREG_EBASE_OFS + 1)
#define SHREG_E2_OFS (SHREG_EBASE_OFS + 2)
#define SHREG_E3_OFS (SHREG_EBASE_OFS + 3)
#define SHREG_E4_OFS (SHREG_EBASE_OFS + 4)
#define SHREG_E5_OFS (SHREG_EBASE_OFS + 5)

/*timer 0*/
#define TIMER0_REG_BASE		0xf5027000
#define TIMER0_OFS_TCVRN	0x00
#define TIMER0_OFS_TRVRN	0x04
#define TIMER0_OFS_TCRN		0x08

#define MCU_TIMEOUT_US	2000000	/*2s*/

.macro umac_lowpower_mode id, flag
	.if \id < CONFIG_SIGMA_NR_UMACS
	mov	r0, #\id
	mov	r1, #\flag
	bl	__sdram_enter_lowpower
	.endif
.endm

.macro umac_access_mode id, flag
	.if \id < CONFIG_SIGMA_NR_UMACS
	mov	r0, #\id
	mov	r1, #\flag
	bl	__sdram_leave_lowpower
	.endif
.endm


.macro umac_set_trb id
	.if \id < CONFIG_SIGMA_NR_UMACS
	mov	r0, #\id
	bl	__sdram_set_tr_area
	.endif
.endm

.macro umac_reg_base id, base
	.if \id < CONFIG_SIGMA_NR_UMACS
pctl_reg_base_umac\id:
	.long	\base
	.endif
.endm

.macro umac_trb_item id, ofs=0
	.if \id < CONFIG_SIGMA_NR_UMACS
trb_ofs_umac\id:
	.long	\ofs
	.endif
.endm

.macro trace_tr_addr
#ifdef DEBUG
	stmfd	sp!, {r0, r1}
	ldr	r1, [sp, #4]		@id
	add	r1, #'0'
	adr	r0, tr_addr_stage
	strb	r1, [r0, #6]
	trace_str
	ldr	r0, [sp]		@addr
	trace_int
	adr	r0, cr_stage
	trace_str
	ldmfd	sp!, {r0, r1}
#endif
.endm


/*
 *void __send_mcu_cmd(unsigned char cmd, void* data, int len);
 */
ENTRY(__send_mcu_cmd)
	stmfd   sp!, {r4 - r11, lr}
	ldr	r4, mcu_reg_base
	add	r5, r4, #SHREG_MBASE_OFS
	mov	r6, #0x11
	strb	r6, [r5], #1	@M0
	mov	r6, #0x8
	strb	r6, [r5], #1	@M1
	mov	r6, #0xff
	strb	r6, [r5], #1	@M2
	strb	r0, [r5], #1	@M3
	cmp	r2, #0
	beq	2f
	add	r7, r1, r2
1:
	ldrb	r8, [r1], #1	@copy data
	strb	r8, [r5], #1
	cmp	r1, r7
	blo	1b
2:
	ldrb	r6, [r4]
	eor	r6, #0x8	@reverse intr
	strb	r6, [r4]
	ldmfd   sp!, {r4 - r11, pc}
ENDPROC(__send_mcu_cmd)


/*
 *int __query_mcu_cmd(int cmd);
 * input: cmd - cmd code
 * return: 
 *	r0  - 0 on success, otherwise non-zero
 */
ENTRY(__query_mcu_cmd)
	stmfd   sp!, {r4 - r5, lr}
	ldr	r4, mcu_reg_base
	ldrb	r5, [r4, #SHREG_E3_OFS]
	subs	r0, r5
	ldmfd   sp!, {r4 - r5, pc}
ENDPROC(__query_mcu_cmd)

/*
 *void __ack_mcu_cmd(void);
 * input: none
 * return: none
 */
ENTRY(__ack_mcu_cmd)
	stmfd   sp!, {r4 - r5, lr}
	ldr	r4, mcu_reg_base
	ldrb	r5, [r4]
	eor	r5, #0x10
	strb	r5, [r4]		@reverse resp
	ldmfd   sp!, {r4 - r5, pc}
ENDPROC(__query_mcu_cmd)

#ifdef MCUCOMM_SUPPORT_TIMEOUT
/*
 *int __wait_event_timeout(int arg, unsigned timeout, int (*chk_evt)(int arg)); 
 * inputs: 
 *        arg      - arg for check function
 *        timeout  - timeout(us, give -1 for infinite)
 *        chk_evt  - check function
 * return: 0 on success, otherwise non-zero
 */
ENTRY(__wait_event_timeout)
	stmfd   sp!, {r4 - r11, lr}
	mov	r4, r0
	mov	r10, r1
	mov	r11, #1
	mov	r7, #0
	cmp	r1, #-1		@infinite loop?
	beq	1f		@looping
	ldr	r5, timer0_reg_base
	ldr	r6, [r5, #TIMER0_OFS_TCRN]	@TIMER0_TCRN
	cmp	r6, #0
	bne	1f		@checking
	mov	r6, #-1
	str	r6, [r5, #TIMER0_OFS_TCVRN]	@TIMER0_TCVRN
	mov	r6, #0x3	@start timer
	str	r6, [r5, #TIMER0_OFS_TCRN]	@TIMER0_TCRN
	mov	r7, #1		@set flag
1:
	mov	r0, r4
	blx	r2		@chk_evt(arg)
	cmp	r0, #0
	bne	2f
	mov	r11, #0		@ok
	b	4f
2:
	cmp	r1, #-1		@infinite loop?
	beq	1b		@looping
	cmp	r10, #0		@timeout?
	beq	4f
	sub	r10, #1
	ldr	r8, [r5, #TIMER0_OFS_TRVRN]	@TIMER0_TRVRN
3:
	ldr	r9, [r5, #TIMER0_OFS_TRVRN]	@TIMER0_TRVRN
	sub	r9, r8
	cmp	r9, #200	@wait 1us
	blo	3b		@waiting
	b	1b		@try again
4:
	cmp	r7, #1
	bne	5f		@out
	mov	r6, #0		@stop timer
	str	r6, [r5, #TIMER0_OFS_TCRN]	@TIMER0_TCRN
5:
	mov	r0, r11
	ldmfd   sp!, {r4 - r11, pc}
ENDPROC(__wait_event_timeout)
#endif  //MCUCOMM_SUPPORT_TIMEOUT

/*
 *int __get_mcu_cmd(int cmd, unsigned timeout);
 * inputs: 
 *        cmd      - cmd code
 *        timeout  - timeout (us)
 * return: 
 *	r0  - 0 on success, otherwise non-zero
 *	r1  - data[0] on success, otherwise undefined
 */
ENTRY(__get_mcu_cmd)
	stmfd   sp!, {r4, lr}
#ifdef MCUCOMM_SUPPORT_TIMEOUT
	adr	r2, __query_mcu_cmd
	bl	__wait_event_timeout
#else
	mov	r4, r0
1:
	mov	r0, r4
	bl	__query_mcu_cmd
	bne	1b		@tryagain
#endif
	ldmfd   sp!, {r4, pc}
ENDPROC(__get_mcu_cmd)

/*
 *int umac_is_activated(int uid);
 * inputs: uid  - umac id (0,1,..)
 * return: 1 on YES, otherwise 0.
 */
ENTRY(umac_is_activated)
	cmp	r0, #CONFIG_SIGMA_NR_UMACS
	movcs	r0, #0
	bcs	2f
	adr	r2, trix_udev_tbl
	umac_get_dev	r1, r2, r0
	umac_dev_quirks	r2, r1
	tst	r2, #UMAC_QUIRK_BROKEN_MAC
	bne	1f
	umac_dev_mac	r2, r1
	ldr	r0, [r2, #MCTL_OFS_IF_ENABLE]
	adds	r0, r0, #0
1:
	movne	r0, #1
2:
	bx	lr
ENDPROC(umac_is_activated)



#ifdef __CONFIG_SRAM_RESUME__

# define BOOT_MODE_NORMAL	0
# define BOOT_MODE_STANDBY	1
# define BOOT_MODE_UNKNOWN	-1

/*
 *int s2ramctl_get_boot_mode(void);
 * inputs: 
 * return:
 *	0  - NORMAL
 *	1  - STANDBY
 *	-1 - UNKNOWN
 */
ENTRY(s2ramctl_get_boot_mode)
	stmfd	sp!, {lr}
	ldr	r0, boot_mode
	cmp	r0, #-1
	bne	4f
	stmfd   sp!, {r4 - r8}
	mov	r0, #MCU_CMD_POWERMODE
	mov	r1, #0			@try only once
	bl	__get_mcu_cmd
	cmp	r0, #0			@error?
	bne	2f
	ldr	r4, mcu_reg_base
	add	r5, r4, #SHREG_E4_OFS
	ldrb	r1, [r5], #1		@bootmode
	add	r6, r5, #4
	adr	r7, resume_entry_ptr
1:
	ldrb	r8, [r5], #1
	strb	r8, [r7], #1		@get entry point
	cmp	r5, r6
	blt	1b

	bl	__ack_mcu_cmd
	cmp	r1, #0			@normal?
	beq	2f
	cmp	r1, #1			@standby?
	bne	2f			@otherwise, error
	mov	r0, #BOOT_MODE_STANDBY	@standby boot
	b	3f
2:
	mov	r0, #BOOT_MODE_NORMAL	@normal boot
3:
	str	r0, boot_mode
	ldmfd   sp!, {r4 - r8}
4:
	ldmfd	sp!, {pc}
ENDPROC(s2ramctl_get_boot_mode)

/*
 *int s2ramctl_sdram_in_access(void);
 * put sdram (controller) in access mode
 * inputs: 
 * return: 0 on success, otherwise non-zero
 */
ENTRY(s2ramctl_sdram_in_access)
	stmfd   sp!, {r4, lr}
	umac_lowpower_mode 0, 1
	umac_lowpower_mode 1, 1
	umac_lowpower_mode 2, 1
	mov	r0, #HOST_CMD_RMODE_DISABLE
	mov	r1, #0
	mov	r2, #0
	bl	__send_mcu_cmd
	mov	r0, #MCU_CMD_ACK_RMODE_OFF
	ldr	r1, mcu_timeout_us
	bl	__get_mcu_cmd
	cmp	r0, #0			@error?
	bne	1f			@out
	bl	__ack_mcu_cmd
#ifdef HOST_CTRL_STB_GPIO
	ldr	r1, mcu_reg_base
	ldrb	r2, [r1, #MCU_REG_GPIO_OFS]
	orr	r2, #0xA0		@set bit[5] and bit[7]
	strb	r2, [r1]
#endif
	umac_access_mode 0, 1
	umac_access_mode 1, 1
	umac_access_mode 2, 1
	mov	r0, #0
1:
	ldmfd   sp!, {r4, pc}
ENDPROC(s2ramctl_sdram_in_access)

.macro trace_resume_entry
	stmfd   sp!, {r0}
	adr	r0, resume_stage
	trace_str
	ldr	r0, resume_entry_ptr
	ldr	r0, [r0]		@resume entry
	trace_int
	mov	r0, #'('
	trace_ch
	ldr	r0, resume_entry_ptr	@frame pointer
	trace_int
	mov	r0, #')'
	trace_ch
	adr	r0, cr_stage
	trace_str
	ldmfd   sp!, {r0}
.endm

/*
 *void s2ramctl_cpu_do_resume(void);
 * jump to resume entry
 * inputs: none
 * return: never return
 */
ENTRY(s2ramctl_cpu_do_resume)
	trace_resume_entry
	ldr	r0, resume_entry_ptr
	ldr	r0, [r0]		@resume entry
	blx	r0
1:	b	1b			@shall not reach here
ENDPROC(s2ramctl_cpu_do_resume)



/*
 *int s2ramctl_set_tr_area(void);
 * set training buffer for umacs
 * inputs: void
 * return: 0 on success, otherwise non-zero
 */
ENTRY(s2ramctl_set_tr_area)
	stmfd   sp!, {lr}
	umac_set_trb 0
	umac_set_trb 1
	umac_set_trb 2
	mov	r0, #0
	ldmfd   sp!, {pc}
ENDPROC(s2ramctl_set_tr_area)


resume_stage: .ascii "\r\nResume @\0"
.align 2

cr_stage: .ascii "\r\n\0"
.align 2

#ifdef DEBUG
tr_addr_stage: .ascii "\r\numacx training buffer:\0"
.align 2
#endif

/*
 * BOOT mode (0-normal, 1-standby, oth-reserved)
 */
boot_mode:
	.long	0xffffffff

/*
 * Training buffer table (offsets)
 */
trb_tbl:
	umac_trb_item 0, CONFIG_TR_OFS_UMAC0
	umac_trb_item 1, CONFIG_TR_OFS_UMAC1
	umac_trb_item 2, CONFIG_TR_OFS_UMAC2
	.equ trb_tbl_sz, (. - trb_tbl)

.ifne trb_tbl_sz - (CONFIG_SIGMA_NR_UMACS << 2)
.error "CONFIG_SIGMA_NR_UMACS is not equal to the size of the trb table"
.endif

#endif /*__CONFIG_SRAM_RESUME__*/

#if defined(CONFIG_SIGMA_SOC_SX7) || defined(CONFIG_SIGMA_SOC_SX6)
#include "s2ram_dwc_phy.S"
#else
#include "s2ram_uniquify_phy.S"
#endif

#if defined(__CONFIG_SRAM_SUSPEND__)

/*
 *void __prepare_sram_selfrefresh(void);
 * prepare for sram in selfrefresh, i.e. fill TLBs
 * inputs: none
 * return: none
 */
ENTRY(__prepare_sram_selfrefresh)
	stmfd   sp!, {r4 - r5, lr}
	plat_umac_preload_reg 0, r4, r5
	plat_umac_preload_reg 1, r4, r5
	plat_umac_preload_reg 2, r4, r5

#ifdef MCUCOMM_SUPPORT_TIMEOUT
	ldr	r4, timer0_reg_base
	ldr	r4, [r4, #TIMER0_OFS_TCVRN]
#endif
	ldr	r4, mcu_reg_base
	ldrb	r4, [r4, #SHREG_M0_OFS]
	ldmfd   sp!, {r4 - r5, pc}
ENDPROC(__prepare_sram_selfrefresh)

/*
 *int s2ramctl_sdram_in_selfrefresh(void);
 * put sdram (controllers) in selfrefresh mode
 * inputs: 
 * return: 0 on success, otherwise non-zero
 */
ENTRY(s2ramctl_sdram_in_selfrefresh)
	stmfd   sp!, {lr}
	bl	__prepare_sram_selfrefresh
	umac_lowpower_mode 0, 0
	umac_lowpower_mode 1, 0
	umac_lowpower_mode 2, 0
	mov	r0, #HOST_CMD_RMODE_ENABLE
	mov	r1, #0
	mov	r2, #0
	bl	__send_mcu_cmd
	mov	r0, #MCU_CMD_ACK_RMODE_ON
	ldr	r1, mcu_timeout_us
	bl	__get_mcu_cmd
	cmp	r0, #0
	bne	1f			@error
	bl	__ack_mcu_cmd
#ifdef HOST_CTRL_STB_GPIO
	ldr	r1, mcu_reg_base
	ldrb	r2, [r1, #MCU_REG_GPIO_OFS]
	orr	r2, #0x20		@set bit[5]
	bic	r2, #0x80		@clear bit[7]
	strb	r2, [r1]
#endif
	b	2f			@out
1:
	umac_access_mode 0, 0
	umac_access_mode 1, 0
	umac_access_mode 2, 0
	mov	r0, #1			@failed
2:
	ldmfd   sp!, {pc}
ENDPROC(s2ramctl_sdram_in_selfrefresh)

/*
 *int s2ramctl_system_poweroff(void);
 * inputs: 
 * return: 0 on success, otherwise non-zero
 */
ENTRY(s2ramctl_system_poweroff)
	stmfd   sp!, {lr}
	ldr	r0, resume_entry_ptr
	str	r0, [sp, #-4]!
	mov	r0, #HOST_CMD_POWEROFF
	mov	r1, sp
	mov	r2, #4
	bl	__send_mcu_cmd
	add	sp, #4
	mov	r0, #0
	ldmfd   sp!, {pc}
ENDPROC(s2ramctl_system_poweroff)

/*
 *void s2ramctl_set_resume_entry(s2ram_resume_frame* entry);
 * set resume entry point (physical address)
 * inputs: entry  - pointer to resume frame struct
 * return: 0 on success, otherwise non-zero
 */
ENTRY(s2ramctl_set_resume_entry)
	str	r0, resume_entry_ptr
	mov	pc, lr
ENDPROC(s2ramctl_set_resume_entry)

#endif /*__CONFIG_SRAM_SUSPEND__*/

/*
 * resume entry point (physical addr)
 */
resume_entry_ptr:
	.long	0x0badc0de
mcu_reg_base:
	.long	MCU_REG_BASE
timer0_reg_base:
	.long	TIMER0_REG_BASE
mcu_timeout_us:
	.long	MCU_TIMEOUT_US

/*
 * umac device table
 */
.global trix_udev_tbl
trix_udev_tbl:
	UMAC_DEV_INITIALIZER

