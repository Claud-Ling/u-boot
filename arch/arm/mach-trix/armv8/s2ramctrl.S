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

.macro umac_preload_reg id, rd, rt, ofs=PCTL_OFS_STAT
	.if \id < CONFIG_SIGMA_NR_UMACS
	mov	r0, #\id
	bl	umac_is_activated
	cmp	r0, #0
	beq	1f

	mov	r0, #\id
	adr	\rt, trix_udev_tbl
	umac_get_dev	\rd, \rt, r0
	umac_dev_phy	\rd, \rd
	ldr	\rd, [\rd, #\ofs]
1:
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

/*
 *void __send_mcu_cmd(unsigned char cmd, void* data, int len);
 */
ENTRY(__send_mcu_cmd)
	ret
ENDPROC(__send_mcu_cmd)


/*
 *int __query_mcu_cmd(int cmd);
 * input: cmd - cmd code
 * return: 
 *	r0  - 0 on success, otherwise non-zero
 */
ENTRY(__query_mcu_cmd)
	ret
ENDPROC(__query_mcu_cmd)

/*
 *void __ack_mcu_cmd(void);
 * input: none
 * return: none
 */
ENTRY(__ack_mcu_cmd)
	ret
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
	ret
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
	ret
ENDPROC(__get_mcu_cmd)

/*
 *int umac_is_activated(int uid);
 * inputs: uid  - umac id (0,1,..)
 * return: 1 on YES, otherwise 0.
 */
ENTRY(umac_is_activated)
	cmp	x0, #0
	mov	x0, #0
	cset	x0, eq
	ret
ENDPROC(umac_is_activated)

#ifdef CONFIG_SIGMA_PM_UMAC_PHY

/*PHY_ACIOCR bits assignments*/
#define ACIOCR_BIT_ACOE		1
/*PHY_DXCCR bits assignments*/
#define DXCCR_BIT_DXPDD		3
#define DXCCR_BIT_DXPDR		4
#define DXCCR_DQSRES_SHIFT	5
#define DXCCR_DQSRES_MASK	(0xf << DXCCR_DQSRES_SHIFT) /*bit[8:5]*/

ACIOCR_POWER_DOWN_MSK:
	.long 0x13fc3f1a
ACIOCR_POWER_DOWN_VAL:
	.long 0x13fc3f18

/*
 * void __umac_phy_aco_control(int base, int state);
 * disable (enable) PHY address/command output by control ACIOCR.ACOE
 * and power down (on) DQ input buffer
 * inputs:
 *    base  - umac register base address
 *    state - 0 disable, 1 enable
 * return: 0 on success, otherwise non-zero
 */
ENTRY(__umac_phy_aco_control)
	ret
ENDPROC(__umac_phy_aco_control)
#endif	/* CONFIG_SIGMA_PM_UMAC_PHY */

/*
 *int __pctl_set_mode(unsigned base, unsigned cmd, unsigned stat);
 * inputs: 
 * 	base - controller base addr
 *	cmd  - cmd code
 *	stat - state code
 * return: 0 on success, otherwise non-zero
 */
ENTRY(__pctl_set_mode)
	ret
ENDPROC(__pctl_set_mode)

/*
 *int __sdram_enter_lowpower(int uid, bool recovery);
 * put one sdram (controller) in lowpower mode
 * inputs: uid  - umac id (0,1...)
 *         recovery - flag, 0 or 1
 * return: 0 on success, otherwise non-zero
 */
ENTRY(__sdram_enter_lowpower)
	ret
ENDPROC(__sdram_enter_lowpower)

/*
 *int __sdram_leave_lowpower(int uid, int recovery);
 * make one sdram (controller) leave lowpower mode (to access mode)
 * inputs: uid  - umac id (0,1...)
 *         recovery - flag, 0 or 1
 * return: 0 on success, otherwise non-zero
 */
ENTRY(__sdram_leave_lowpower)
	ret
ENDPROC(__sdram_leave_lowpower)

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
	mov	x0, #BOOT_MODE_NORMAL
	ret
ENDPROC(s2ramctl_get_boot_mode)

/*
 *int s2ramctl_sdram_in_access(void);
 * put sdram (controller) in access mode
 * inputs: 
 * return: 0 on success, otherwise non-zero
 */
ENTRY(s2ramctl_sdram_in_access)
	mov	x0, #0
	ret
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
1:	b	1b			@shall not reach here
	ret
ENDPROC(s2ramctl_cpu_do_resume)

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
 *int __sdram_set_tr_area(int id);
 * set training memory (size=8k) for one umac
 * criteria on mem area: 32k aligned, and <2G
 * [31..30] Reserved
 * [29..13] Row Address
 *  Register_bit[29] = 0;
 *  Register_bit[28:25] = start_address[30:27];
 *  Register_bit[24:16] = start_address[23:15];
 *  Register_bit[15:13] = start_address[26:24];
 * [12..10] Bank Address, 3'b000
 * [9..0] Colomn Address, start from 0 and increase at a step of 0x8
 * inputs: id    - umac id (0, 1...)
 * return: 0 on success. Otherwise none-zero
 */
ENTRY(__sdram_set_tr_area)
	mov	x0, #0
	ret
ENDPROC(__sdram_set_tr_area)

/*
 *int s2ramctl_set_tr_area(void);
 * set training buffer for umacs
 * inputs: void
 * return: 0 on success, otherwise non-zero
 */
ENTRY(s2ramctl_set_tr_area)
	mov	x0, #0
	ret
ENDPROC(s2ramctl_set_tr_area)

/*
 * Notes:
 * The default register values on coming out of reset already meet the
 * requirement as's stated in 'Self Refresh Note_v1.0'
 *   PHY_ACIOCR = 0x00000002
 *   PHY_DXCCR  = 0x00001884
 * So just comment out the code below since it's not necessary any more
 */
#if 0
//#ifdef CONFIG_SIGMA_PM_UMAC_PHY
.macro umac_aco_enable id, state
	.if \id < CONFIG_SIGMA_NR_UMACS
	ldr	r0, #\id
	mov	r1, \state
	bl	__umac_phy_output_enable
	.endif
.endm

/*
 *int __umac_phy_output_enable(int uid, int state);
 * enable/disable addr/cmd output from umac phy
 * inputs:
 *    uid   - umac id (0,1...)
 *    state - 0 disable, 1 enable
 * return:  0 on success, otherwise non-zero
 */
ENTRY(__umac_phy_output_enable)
	mov	x0, #0
	ret
ENDPROC(__umac_phy_output_enable)

/*
 *int s2ramctl_phy_enable(void);
 * enable phy for umacs
 * inputs: void
 * return: 0 on success, otherwise non-zero
 */
ENTRY(s2ramctl_phy_enable)
	mov	x0, #0
	ret
ENDPROC(s2ramctl_phy_enable)
#endif /*CONFIG_SIGMA_PM_UMAC_PHY*/

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

#elif defined(__CONFIG_SRAM_SUSPEND__) /*__CONFIG_SRAM_RESUME__*/

/*
 *void __prepare_sram_selfrefresh(void);
 * prepare for sram in selfrefresh, i.e. fill TLBs
 * inputs: none
 * return: none
 */
ENTRY(__prepare_sram_selfrefresh)
	ret
ENDPROC(__prepare_sram_selfrefresh)

/*
 *int s2ramctl_sdram_in_selfrefresh(void);
 * put sdram (controllers) in selfrefresh mode
 * inputs: 
 * return: 0 on success, otherwise non-zero
 */
ENTRY(s2ramctl_sdram_in_selfrefresh)
	mov 	x0, #0
	ret
ENDPROC(s2ramctl_sdram_in_selfrefresh)

/*
 *int s2ramctl_system_poweroff(void);
 * inputs: 
 * return: 0 on success, otherwise non-zero
 */
ENTRY(s2ramctl_system_poweroff)
	mov	x0, #1
	ret
ENDPROC(s2ramctl_system_poweroff)

/*
 *void s2ramctl_set_resume_entry(s2ram_resume_frame* entry);
 * set resume entry point (physical address)
 * inputs: entry  - pointer to resume frame struct
 * return: 0 on success, otherwise non-zero
 */
ENTRY(s2ramctl_set_resume_entry)
	ret
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
 * note it requires 8-byte alignment in case of arm64
 */
.global trix_udev_tbl ; .align 3
trix_udev_tbl:
	UMAC_DEV_INITIALIZER
