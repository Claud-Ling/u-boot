/*
 * Suspend to RAM implement for Sigma DTV Soc SX6/SX7(Designware PHY)
 */


.macro plat_umac_preload_reg id, rd, rt, ofs=PCTL_OFS_STAT
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
	/*
	 * ACIOCR
	 * bit[1] - ACOE
	 * on disable: set ACIOCR to VAL with mask MSK
	 * on enable:  set ACOE to 1'b1
	 */
	cmp	r1, #0
	movne	r2, #(1 << ACIOCR_BIT_ACOE)	@ on enable
	ldreq	r2, [r0, #PUB_OFS_PHY_ACIOCR]
	ldreq	r3, ACIOCR_POWER_DOWN_MSK
	biceq	r2, r3
	ldreq	r3, ACIOCR_POWER_DOWN_VAL
	orreq	r2, r3				@ on disable
	str	r2, [r0, #PUB_OFS_PHY_ACIOCR]

	/*
	 * DXCCR
	 * bit[3]    - DXPDD
	 * bit[4]    - DXPDR
	 * bit[8:5]  - DQSRES
	 * on disable: set DXPDD to 1'b1 set DXPDR to 1'b1, set DQSRES to 4'b0000
	 * on enable:  set DXPDD to 1'b0 set DXPDR to 1'b0, set DQSRES to 4'b0100
	 */
	ldr	r2, [r0, #PUB_OFS_PHY_DXCCR]
	bic	r2, #(1 << DXCCR_BIT_DXPDD | 1 << DXCCR_BIT_DXPDR | DXCCR_DQSRES_MASK)
	cmp	r1, #0
	orreq	r2, #(1 << DXCCR_BIT_DXPDD | 1 << DXCCR_BIT_DXPDR) @ on disable
	orrne	r2, #(0x4 << DXCCR_DQSRES_SHIFT) @ on enable
	str	r2, [r0, #PUB_OFS_PHY_DXCCR]
	mov	r0, #0
	mov	pc, lr
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
	stmfd   sp!, {r3, lr}
	str	r1, [r0, #PCTL_OFS_SCTL]
1:
	ldr	r3, [r0, #PCTL_OFS_STAT]
	cmp	r3, r2
	bne	1b		@looping
	mov	r0, #0
	ldmfd   sp!, {r3, pc}
ENDPROC(__pctl_set_mode)

/*
 *int __sdram_enter_lowpower(int uid, bool recovery);
 * put one sdram (controller) in lowpower mode
 * inputs: uid  - umac id (0,1...)
 *         recovery - flag, 0 or 1
 * return: 0 on success, otherwise non-zero
 */
ENTRY(__sdram_enter_lowpower)
	stmfd   sp!, {r4, r5, lr}
	mov	r4, r0
	mov	r5, r1
	bl	umac_is_activated
	cmp	r0, #0
	moveq	r0, #1			@inactivated?
	beq	2f

	adr	r2, trix_udev_tbl
	umac_get_dev	r3, r2, r4
	umac_dev_phy	r4, r3
	cmp	r5, #0
	beq	1f
	/*on resume*/
	mov	r0, r4
	mov	r1, #PCTL_CMD_CFG
	mov	r2, #PCTL_STAT_CONFIG
	bl	__pctl_set_mode
	mov	r0, r4
	mov	r1, #PCTL_CMD_GO
	mov	r2, #PCTL_STAT_ACCESS
	bl	__pctl_set_mode
	mov	r0, r4
	mov	r1, #PCTL_CMD_SLEEP
	mov	r2, #PCTL_STAT_LOWPOWER
	bl	__pctl_set_mode
	b	2f
1:
	/*on suspend*/
	mov	r0, r4
	mov	r1, #PCTL_CMD_SLEEP
	mov	r2, #PCTL_STAT_LOWPOWER
	bl	__pctl_set_mode

	mov	r1, #0x0
	str	r1, [r4, #PCTL_OFS_TREFI]
	str	r1, [r4, #PCTL_OFS_PHYPVTUPDI]
	str	r1, [r4, #PCTL_OFS_PVTUPDI]
#ifdef CONFIG_SIGMA_PM_UMAC_PHY
	mov	r0, r4
	mov	r1, #0			@disable
	bl	__umac_phy_aco_control
#endif
2:
	ldmfd   sp!, {r4, r5, pc}
ENDPROC(__sdram_enter_lowpower)

/*
 *int __sdram_leave_lowpower(int uid, int recovery);
 * make one sdram (controller) leave lowpower mode (to access mode)
 * inputs: uid  - umac id (0,1...)
 *         recovery - flag, 0 or 1
 * return: 0 on success, otherwise non-zero
 */
ENTRY(__sdram_leave_lowpower)
	stmfd   sp!, {r4, r5, lr}
	mov	r4, r0
	mov	r5, r1
	bl	umac_is_activated
	cmp	r0, #0
	moveq	r0, #1			@inactivated?
	beq	2f

	adr	r2, trix_udev_tbl
	umac_get_dev	r3, r2, r4
	umac_dev_phy	r4, r3

	mov	r2, #0x27
	str	r2, [r4, #PCTL_OFS_TREFI]
	mov	r0, r4
	mov	r1, #PCTL_CMD_WAKEUP
	mov	r2, #PCTL_STAT_ACCESS
	bl	__pctl_set_mode
	cmp	r5, #0
	beq	1f
	/*on resume*/
	mov	r0, r4
	mov	r1, #PCTL_CMD_CFG
	mov	r2, #PCTL_STAT_CONFIG
	bl	__pctl_set_mode
	b	2f
1:
	/*on suspend*/
#ifdef CONFIG_SIGMA_PM_UMAC_PHY
	mov	r0, r4
	mov	r1, #1			@enable
	bl	__umac_phy_aco_control
#endif
2:
	ldmfd   sp!, {r4, r5, pc}
ENDPROC(__sdram_leave_lowpower)

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
	stmfd   sp!, {r4 - r5, lr}
	mov	r4, r0
	mov	r5, r1
	bl	umac_is_activated
	cmp	r0, #0
	moveq	r0, #1			@inactivated?
	beq	1f

	adr	r2, trix_udev_tbl
	umac_get_dev	r3, r2, r4
	umac_dev_phy	r0, r3
	mov	r1, r5
	bl	__umac_phy_aco_control
1:
	ldmfd   sp!, {r4 - r5, pc}
ENDPROC(__umac_phy_output_enable)

/*
 *int s2ramctl_phy_enable(void);
 * enable phy for umacs
 * inputs: void
 * return: 0 on success, otherwise non-zero
 */
ENTRY(s2ramctl_phy_enable)
	stmfd   sp!, {lr}
	umac_aco_enable 0, 1
	umac_aco_enable 1, 1
	umac_aco_enable 2, 1
	mov	r0, #0
	ldmfd   sp!, {pc}
ENDPROC(s2ramctl_phy_enable)
#endif /*CONFIG_SIGMA_PM_UMAC_PHY*/

#if defined(__CONFIG_SRAM_RESUME__)
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
	stmfd   sp!, {r4 - r6, lr}
	mov	r4, r0
	bl	umac_is_activated
	cmp	r0, #0
	moveq	r0, #1			@inactivated?
	beq	2f

	adr	r2, trix_udev_tbl
	umac_get_dev	r3, r2, r4
	umac_dev_phy	r5, r3		@phy base

	adr	r2, trb_tbl
	ldr	r0, [r2, r4, lsl #2]	@tr buff
	trace_tr_addr

	mov	r2, r0, lsr #27
	and	r2, #0xf
	mov	r3, r2, lsl #25		@[28..25]
	mov	r2, r0, lsl #8
	mov	r2, r2, lsr #23
	orr	r3, r2, lsl #16		@[24..16]
	mov	r2, r0, lsr #24
	and	r2, #0x7
	orr	r3, r2, lsl #13		@[15..13]

	add	r2, r5, #(PUB_OFS_TR_ADDR0 & (~0xf))
	add	r2, r2, #(PUB_OFS_TR_ADDR0 & 0xf)
	mov	r6, #4
1:
	str	r3, [r2], #4
	add	r3, #8
	subs	r6, r6, #1
	bne	1b
	mov	r0, #0
2:
	ldmfd   sp!, {r4 - r6, pc}
ENDPROC(__sdram_set_tr_area)
#endif
