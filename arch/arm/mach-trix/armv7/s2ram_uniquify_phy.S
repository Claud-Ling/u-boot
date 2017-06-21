/*
 * Suspend to RAM implement for Sigma DTV Soc SX8(Uniquify PHY)
 */

.macro plat_umac_preload_reg id, rd, rt

.endm


#define PCTL_RST_PHY	(0x1)
#define PCTL_RST_L1	(0x2)
#define PCTL_RST_L2	(0x4)
#define PCTL_RST_L3	(0x8)

#define PCTL_SELF_REFRESH_ST		(1<<31)
#define PCTL_MINTF_SIGNAL_OFF		(1<<31)


#define PCTL_PHY_PAD_CTL_ON		(1<<28)

.macro uniquify_io_onoff on
	ldr	r1, [r4, #PCTL_OFS_PHY_PAD_CTL]
	.if \on == 1
	orr	r1, #PCTL_PHY_PAD_CTL_ON
	.else
	bic	r1, #PCTL_PHY_PAD_CTL_ON
	.endif
	str	r1, [r4, #PCTL_OFS_PHY_PAD_CTL]
.endm

.macro mem_signal_intf_onoff on
	ldr	r1, [r4, #PCTL_OFS_CFG4]
	.if \on == 1
	bic	r1, #PCTL_MINTF_SIGNAL_OFF
	.else
	orr	r1, #PCTL_MINTF_SIGNAL_OFF
	.endif
	str	r1, [r4, #PCTL_OFS_CFG4]
.endm

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

	adr	r2, trix_udev_tbl
	umac_get_dev	r3, r2, r4
	umac_dev_phy	r4, r3
	cmp	r5, #0
	beq	1f
	/*on resume*/

	/* Turn off memory interface signal */
	mov	r1, #PCTL_MINTF_SIGNAL_OFF
	ldr	r2, [r4, #PCTL_OFS_CFG4]
	orr	r2, r1
	str	r2, [r4, #PCTL_OFS_CFG4]

	
	/* Set VERF value */
	mov	r1, #0x54
	str	r1, [r4, #PCTL_OFS_VREF_TARINING]

	b	2f
1:
	/*on suspend*/
	bl	umac_is_activated
	cmp	r0, #0
	moveq	r0, #1			@inactivated?
	beq	2f

	/* Assert PCTL L1 reset, self-refresh ? can we roll back?*/
	mov	r1, #PCTL_RST_L1
	str	r1, [r4, #PCTL_OFS_REG4]

	/* Wait self refresh indicator */
11:
	mov	r1, #PCTL_SELF_REFRESH_ST
	ldr	r2, [r4, #PCTL_OFS_CFG1]
	tst	r1, r2
	beq	11b

	/* Turn off memory interface signal */
	mem_signal_intf_onoff 0

	/* IO PAD OFF */
	uniquify_io_onoff 0


	/* Set VERF value */
	mov	r1, #0x50
	str	r1, [r4, #PCTL_OFS_VREF_TARINING]


	/* Set DDR4 config */
	mov	r1, #0x80
	str	r1, [r4, #PCTL_OFS_DDR4_CONFIG_1]

	/* Disable uniquify IO */
	mov	r1, #0
	str	r1, [r4, #PCTL_OFS_UNIQUIFY_IO_1]


	/* Reset PHY */
	mov	r1, #(PCTL_RST_PHY | PCTL_RST_L1)
	str	r1, [r4, #PCTL_OFS_REG4]
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

	adr	r2, trix_udev_tbl
	umac_get_dev	r3, r2, r4
	umac_dev_phy	r4, r3

	cmp	r5, #0
	beq	1f
	/*on resume*/

	/* Turn on memory interface signal */
	mem_signal_intf_onoff 1

	/* IO PAD ON */
	uniquify_io_onoff 1
	
	/* Set VERF value */
	mov	r1, #0x50
	str	r1, [r4, #PCTL_OFS_VREF_TARINING]

	/* Enable dynamic impedance calibration */
	mov	r1, #1
	str	r1, [r4, #PCTL_OFS_UNIQUIFY_IO_1]

	b	2f
1:
	/*on suspend*/
	//TODO: recovery from suspend fail case.
2:
	ldmfd   sp!, {r4, r5, pc}
ENDPROC(__sdram_leave_lowpower)

#if defined(__CONFIG_SRAM_RESUME__)
/*
 *int __sdram_set_tr_area(int id);
 * set training memory (size=8k) for one umac
 * return: 0 on success. Otherwise none-zero
 * Due to on SX8 MCTL have many of interleave scheme
 * each of them have common part, that is:
 * 	bit[31] -> bit[27]
 * 	bit[30] -> bit[15]
 * 	bit[29] -> bit[14]
 * 	bit[28] -> bit[13]
 * 	bit[27] -> bit[12]
 * 	bit[23] -> bit[11]
 * 	bit[22] -> bit[10]
 * 	bit[21] -> bit[09]
 * 	bit[26] -> bit[02]
 * 	bit[25] -> bit[01]
 * 	bit[24] -> bit[00]
 * So, here, we only support 2MB algined address map to
 * UAMC physically address.(otherwise refer to DTVSXE-1429)
 */
ENTRY(__sdram_set_tr_area)
	stmfd   sp!, {r4 - r6, lr}
	mov	r4, r0

	adr	r2, trix_udev_tbl
	umac_get_dev	r3, r2, r4
	umac_dev_phy	r5, r3		@phy base

	adr	r2, trb_tbl
	ldr	r0, [r2, r4, lsl #2]	@tr buff
	trace_tr_addr

	mov 	r2, r0, LSR #27		@[30..27]
	and	r2, #0xf
	mov	r3, r2, LSL #12		@[30..27] shift to [15..12]

	mov 	r2, r0, LSR #24		@[26..24]
	and	r2, #0x7
	orr	r3, r2, LSL #0		@[26..24] shift to [2..0]

	mov 	r2, r0, LSR #21		@[23..21]
	and	r2, #0x7
	orr	r3, r2, LSL #9		@[23..21] shift to [11..9]

	mov 	r0, r0, LSR #31		@[31]
	and	r0, #1
	mov	r0, r0, LSL #27		@[31] shift to [27]
	orr	r0, r0, r3

	str 	r0, [r5, #PCTL_OFS_SCL_START_ADDR]
2:
	ldmfd   sp!, {r4 - r6, pc}
ENDPROC(__sdram_set_tr_area)
#endif

