/*
 * Copyright  2016
 * Sigma Designs, Inc. All Rights Reserved
 * Proprietary and Confidential
 *
 * This file is to define register files to describe
 * UMAC_IP_2034 and UMAC_IP_2037 of SX6/SX7/UXLB/SX8 onwards
 * DTV SoC chipsets.
 *
 * The host can override PMAN_IP_1820_CON base address by
 * defining CONFIG_REG_BASE_PMAN_CON.
 *
 * Author: Tony He <tony_he@sigmadesigns.com>
 * Date:   05/16/2016
 */

#ifndef __ASM_ARCH_TRIX_MACH_UMAC_H__
#define __ASM_ARCH_TRIX_MACH_UMAC_H__

/*
 * PCTL CMD/STATE
 */
#define PCTL_CMD_INIT			0x0
#define PCTL_CMD_CFG			0x1
#define PCTL_CMD_GO			0x2
#define PCTL_CMD_SLEEP			0x3
#define PCTL_CMD_WAKEUP			0x4

#define PCTL_STAT_INIT			0x0
#define PCTL_STAT_CONFIG		0x1
#define PCTL_STAT_CONFIG_REQ		0x2
#define PCTL_STAT_ACCESS		0x3
#define PCTL_STAT_ACCESS_REQ		0x4
#define PCTL_STAT_LOWPOWER		0x5
#define PCTL_STAT_LOWPOWER_ENTRY_REQ	0x6
#define PCTL_STAT_LOWPOWER_EXIT_REQ	0x7

/*
 * UMAC width
 */
#define UMAC_DATA_WIDTH16		16
#define UMAC_DATA_WIDTH32		32

/*
 * UMAC quirks
 */
#define UMAC_QUIRK_LEGACY_TR_OFS	(1<<0)
#define UMAC_QUIRK_BROKEN_MAC		(1<<1)
#define UMAC_QUIRK_VARIABLE_ADDR	(1<<2)

#ifndef __ASSEMBLY__
typedef unsigned int U32;

#ifdef NW
#undef NW
#endif
#define NW(s,e) (((e)-(s))>>2)	/*Number of Words*/

#ifndef NULL
# define NULL ((void *)0)
#endif

/*
 * Misc
 */
#define UMAC_NR_OF_LANES(id) (umac_get_width(id) >> 3) /*number of byte lanes*/

/*MAC Control*/
struct umac_mctl{
	volatile U32 if_enable;			/* +0x000 */
	volatile U32 if_busy;			/* +0x004 */
	volatile U32 if0_coh_contro;		/* +0x008 */
	volatile U32 if1_coh_contro;		/* +0x00c */
	volatile U32 if2_coh_contro;		/* +0x010 */
	volatile U32 if3_coh_contro;		/* +0x014 */
	volatile U32 if4_coh_contro;		/* +0x018 */
	volatile U32 if5_coh_contro;		/* +0x01c */
	volatile U32 pad0[NW(0x020,0xff0)];	/* +0x020 */
	volatile U32 sw_reset;			/* +0xff0 */
	volatile U32 powermode_ctrl;		/* +0xff4 */
	volatile U32 pad1[NW(0xff8,0xffc)];	/* +0xff8 */
	volatile U32 module_id;			/* +0xffc */
};

/*State Configuration Register*/
typedef union _tagPCTL_SCFGReg {
	struct { __extension__ U32 /*lsbs...*/
		lpifen:  1  /*low-power interface enable*/,
		lben:    1  /*loopback enable*/,
		hole0:   1  /*reserved*/,
		rctl:    1  /*role control*/,
		hsen:    1  /*handshake enable*/,
		genrken: 1  /*generate rank info enable*/,
		nfifo:   1  /*NFIFO test*/,
		hole1:   25;/*... to msbs*/
	}bits;
	U32 val;
} PCTL_SCFGReg;

/*Operational State Control Register*/
typedef union _tagPCTL_SCTLReg {
	struct { __extension__ U32 /*lsbs...*/
		req:     3  /*state transition request*/,
		hole0:   29;/*... to msbs*/
	}bits;
	U32 val;
} PCTL_SCTLReg;

/*Operational State Status Register*/
typedef union _tagPCTL_STATReg {
	struct { __extension__ U32 /*lsbs...*/
		stat:    3  /*current operational state*/,
		hole0:   29;/*... to msbs*/
	}bits;
	U32 val;
} PCTL_STATReg;

/*D Valid, DS Enable and QS Enable Configuration Register*/
typedef union _tagPCTL_DQSECFGReg {
	struct { __extension__ U32 /*lsbs...*/
		qslen:   2  /*qs enable additive length*/,
		hole0:   2  /*reserved*/,
		qsltcy:  4  /*qs enable additive latency*/,
		dslen:   2  /*ds enable additive length*/,
		hole1:   2  /*reserved*/,
		dsltcy:  4  /*ds enable additive latency*/,
		dvlen:   2  /*d_valid additive length*/,
		hole2:   2  /*reserved*/,
		dvltcy:  4  /*d_valid additive latency*/,
		hole3:   8; /*... to msbs*/
	}bits;
	U32 val;
} PCTL_DQSECFGReg;

/*Protocol Control*/
struct umac_pctl{
	volatile PCTL_SCFGReg scfg;		/* +0x000 */
	volatile PCTL_SCTLReg sctl;		/* +0x004 */
	volatile PCTL_STATReg stat;		/* +0x008 */
	volatile U32 pad0[NW(0x00c,0x090)];	/* +0x00c */
	volatile PCTL_DQSECFGReg dqsecfg;	/* +0x090 */
	volatile U32 pad1[NW(0x094,0x0d0)];	/* +0x094 */
	volatile U32 trefi;			/* +0x0d0 */
	volatile U32 pad2[NW(0x0d4,0x318)];	/* +0x0d4 */
	volatile U32 phypvtupdi;		/* +0x318 */
	volatile U32 pad3[NW(0x31c,0x328)];	/* +0x31c */
	volatile U32 pvtupdi;			/* +0x328 */
	volatile U32 pad4[NW(0x32c,0x400)];	/* +0x32c */
};

/*Operational State Status Register*/
typedef union _tagPHY_PIRReg {
	struct { __extension__ U32 /*lsbs...*/
		phyinit: 1  /*PHY Initialization*/,
		initbyp: 1  /*PHY Initialization Bypass*/,
		cal:     1  /*Calibrate*/,
		calbyp:  1  /*Calibrate Bypass*/,
		hole0:   2  /*Reserved*/,
		phyhrst: 1  /*PHY High-Speed Reset*/,
		pllrst:  1  /*PLL Reset*/,
		pllpd:   1  /*PLL Power Down*/,
		mdlen:   1  /*Master Delay Line Enable*/,
		lpfen:   1  /*Low-Pass Filter Enable*/,
		lpfdepth:2  /*Low-Pass Filter Depth*/,
		fdepth:  2  /*Filter Depth*/,
		dldlmt:  8  /*Delay Line VT Drift Limit*/,
		zcksel:  2  /*Impedance Clock Divider Select*/,
		zcalbyp: 1  /*Impedance Calibration Bypass*/,
		zcal:    1  /*Impedance Calibrate*/,
		zcalupd: 1  /*Impedance Calibration on VT Update*/,
		inhvt:   1  /*VT Calculation Inhibit*/,
		hole1:   3; /*... to msbs*/
	}bits;
	U32 val;
} PHY_PIRReg;

/*PHY_ACIOCR Register*/
typedef union _tagPHY_ACIOCRReg {
	struct { __extension__ U32 /*lsbs...*/
		aciom:   1  /*Address/Command I/O Mode*/,
		acoe:    1  /*Address/Command Output Enable*/,
		acodt:   1  /*Address/Command On-Die Termination*/,
		acpdd:   1  /*AC Power Down Driver*/,
		acpdr:   1  /*AC Power Down Receiver*/,
		ckodt:   3  /*CK On-Die Termination*/,
		ckpdd:   3  /*CK Power Down Driver*/,
		ckpdr:   3  /*CK Power Down Receiver*/,
		rankodt: 4  /*Rank On-Die Termination*/,
		rankpdd: 4  /*Rank Power Down Driver*/,
		rankpdr: 4  /*Rank Power Down Receiver*/,
		rstodt:  1  /*SDRAM Reset On-Die Termination*/,
		rstpdd:  1  /*SDRAM Reset Power Down Driver*/,
		rstpdr:  1  /*SDRAM Reset Power Down Receiver*/,
		acsr:    2  /*Address/Command Slew Rate (IO Type D3F_IO only)*/,
		hole0:   1; /*... to msbs*/
	}bits;
	U32 val;
} PHY_ACIOCRReg;

/*DATX8 Common Configuration Register*/
typedef union _tagPHY_DXCCRReg {
	struct { __extension__ U32 /*lsbs...*/
		dxodt:   1  /*Data On-Die Termination*/,
		dxiom:   1  /*Data IDDQ Test Mode*/,
		mdlen:   1  /*Master Delay Line Enable*/,
		dxpdd:   1  /*Data Power Down Driver*/,
		dxpdr:   1  /*Data Power Down Receiver*/,
		dqsres:  4  /*DQS Resistor*/,
		dqsnres: 4  /*DQS# Resistor*/,
		dxsr:    2  /*Data Slew Rate (IO Type D3F_IO only)*/,
		hole0:   17;/*... to msbs*/
	}bits;
	U32 val;
} PHY_DXCCRReg;

/*DRLCFG Register*/
typedef union _tagPUB_DRLCFGReg {
	struct { __extension__ U32 /*lsbs...*/
		rslr0:   3  /*rslr0 value*/,
		rslr1:   3  /*rslr1 value*/,
		rslr2:   3  /*rslr2 value*/,
		rslr3:   3  /*rslr3 value*/,
		hole0:   20;/*... to msbs*/
	}bits;
	U32 val;
} PUB_DRLCFGReg;

/*DATX8 Local Calibrated Delay Line Register 1 (PHY_DXnLCDLR1)*/
typedef union _tagPHY_DXnLCDLR1Reg {
	struct { __extension__ U32 /*lsbs...*/
		wdqd:    8  /*Write Data Delay*/,
		rdqsd:   8  /*Read DQS Delay*/,
		rdqsnd:  8  /*Read DQSN Delay*/,
		hole0:   8; /*... to msbs*/
	}bits;
	U32 val;
} PHY_DXnLCDLR1Reg;

/*PHY Utility Block*/
struct umac_pub{
	volatile U32 phy_ridr;			/* +0x400 */
	volatile PHY_PIRReg phy_pir;		/* +0x404 */
	volatile U32 pad00[NW(0x408,0x430)];	/* +0x408 */
	volatile PHY_ACIOCRReg phy_aciocr;	/* +0x430 */
	volatile PHY_DXCCRReg phy_dxccr;	/* +0x434 */
	volatile U32 pad01[NW(0x438,0x494)];	/* +0x438 */
	volatile U32 pub_tr_addr0;		/* +0x494 */
	volatile U32 pub_tr_addr1;		/* +0x498 */
	volatile U32 pub_tr_addr2;		/* +0x49c */
	volatile U32 pub_tr_addr3;		/* +0x4a0 */
	volatile U32 pub_tr_rd_lcdl;		/* +0x4a4 */
	volatile U32 pad1[NW(0x4a8,0x4b4)];	/* +0x4a8 */
	volatile U32 pub_tr_wr_lcdl;		/* +0x4b4 */
	volatile U32 pad2[NW(0x4b8,0x518)];	/* +0x4b8 */
	volatile PUB_DRLCFGReg drlcfg_0;	/* +0x518 */
	volatile U32 pad3[NW(0x51c,0x5e4)];	/* +0x51c */
	volatile PHY_DXnLCDLR1Reg phy_dx0lcdlr1;/* +0x5e4 */
	volatile U32 phy_dx0lcdlr2;		/* +0x5e8 */
	volatile U32 pad4[NW(0x5ec,0x624)];	/* +0x5ec */
	volatile PHY_DXnLCDLR1Reg phy_dx1lcdlr1;/* +0x624 */
	volatile U32 phy_dx1lcdlr2;		/* +0x628 */
	volatile U32 pad5[NW(0x62c,0x664)];	/* +0x62c */
	volatile PHY_DXnLCDLR1Reg phy_dx2lcdlr1;/* +0x664 */
	volatile U32 phy_dx2lcdlr2;		/* +0x668 */
	volatile U32 pad6[NW(0x66c,0x6a4)];	/* +0x66c */
	volatile PHY_DXnLCDLR1Reg phy_dx3lcdlr1;/* +0x6a4 */
	volatile U32 phy_dx3lcdlr2;		/* +0x6a8 */
	volatile U32 pad7[NW(0x6ac,0xfc0)];	/* +0x6ac */
	volatile U32 ctl_dqsecfg_pe;		/* +0xfc0 */
	volatile U32 ctl_dqsecfg;		/* +0xfc4 */
	volatile U32 pad8[NW(0xfc8,0xfe4)];	/* +0xfc8 */
	volatile U32 pub_tr_rw_ofs;		/* +0xfe4, except uxlb */
	volatile U32 pub_tr_rw_ofs_sign;	/* +0xfe8, except uxlb */
	volatile U32 pad9[NW(0xfec,0x1000)];	/* +0xfec */
};

/*Arbiter, uxlb only*/
struct umac_abt{
	volatile U32 pad0[NW(0x000,0x034)];	/* +0x000 */
	volatile U32 pub_tr_rw_ofs;		/* +0x034 */
	volatile U32 pub_tr_rw_ofs_sign;	/* +0x038 */
	volatile U32 pad1[NW(0x03c,0x194)];	/* +0x03c */
};

/*UMAC_IP_2034, except uxlb*/
struct umac_mac{
	struct umac_mctl mctl;
};

/*UMAC_IP_2037*/
struct umac_phy{
	struct umac_pctl pctl;
	struct umac_pub  pub;
};

/*umac device structure*/
struct umac_device{
	struct umac_mac *mac;	/*MAC base*/
	struct umac_phy *phy;	/*PHY base*/
	struct umac_abt *abt;	/*ARBT base*/
	U32 width;		/*data width (number of bits)*/
	U32 quirks;
	U32 addr;		/*base addr*/
};

/**
 * umac device object initializer helper (c variant)
 *   mac - MAC base
 *   phy - PHY base
 *   abt - Arbiter base
 *   w   - data width, number of bits
 *   q   - quirks
 *   a   - base addr
 */
#define UMAC_DEVICE_INIT(mac,phy,abt,w,q,a) 			\
	{(struct umac_mac*)(mac),(struct umac_phy*)(phy),	\
	 (struct umac_abt*)(abt),(w),(q),(a)},

extern const struct umac_device trix_udev_tbl[CONFIG_SIGMA_NR_UMACS];

/**
 * @brief	get umac_device object for the specified umac id.
 * @fn		const struct umac_device* umac_get_dev_by_id(int uid);
 * @param[in]	uid	umac id (0,1,...)
 * @retval	object	on success
 * @retval	NULL	fail
 */
extern const struct umac_device* umac_get_dev_by_id(int uid);

/**
 * @brief	check if specified umac is activated or not.
 * @fn		int umac_is_activated(int uid);
 * @param[in]	uid	umac id (0,1,...)
 * @retval	TRUE	activated
 * @retval	FALSE	inactivated
 */
extern int umac_is_activated(int uid);

#define umac_get_element(T, E)					\
static inline T umac_get_##E(int uid)				\
{								\
	const struct umac_device* dev =	umac_get_dev_by_id(uid);\
	if (dev != NULL)					\
		return dev->E;					\
	else							\
		return (T)0;					\
}

umac_get_element(struct umac_mac*, mac)
umac_get_element(struct umac_phy*, phy)
umac_get_element(struct umac_abt*, abt)
umac_get_element(const U32, width)
umac_get_element(const U32, quirks)

/*PMAN Hub address Register*/
typedef union _tagPMAN_CON_HUB_ADDRReg {
	struct { __extension__ U32 /*lsbs...*/
		rank:    4  /*addr[31:28] value*/,
		hole0:   28;/*... to msbs*/
	}bits;
	U32 val;
} PMAN_CON_HUB_ADDRReg;

/*PMAN_IP_1820_CON*/
struct pman_con{
#if defined(CONFIG_SIGMA_SOC_SX7)
# define PMAN_HUB_REMAP
	volatile U32 pad0[NW(0x000,0x030)];	/* +0x000 */
	volatile PMAN_CON_HUB_ADDRReg hub0_start_addr;	/* +0x030 */
	volatile PMAN_CON_HUB_ADDRReg hub0_end_addr;	/* +0x034 */
	volatile PMAN_CON_HUB_ADDRReg hub1_start_addr;	/* +0x038 */
	volatile PMAN_CON_HUB_ADDRReg hub1_end_addr;	/* +0x03c */
	volatile PMAN_CON_HUB_ADDRReg hub2_start_addr;	/* +0x040 */
	volatile PMAN_CON_HUB_ADDRReg hub2_end_addr;	/* +0x044 */
	volatile U32 pad1[NW(0x048,0xffc)];	/* +0x048 */
	volatile U32 module_id;			/* +0xffc */
#elif defined (CONFIG_SIGMA_SOC_SX8) || defined(CONFIG_SIGMA_SOC_UNION) //TODO: union spg
# define PMAN_HUB_REMAP
	volatile U32 pad0[NW(0x000,0x03c)];	/* +0x000 */
	volatile PMAN_CON_HUB_ADDRReg hub0_start_addr;	/* +0x03c */
	volatile PMAN_CON_HUB_ADDRReg hub0_end_addr;	/* +0x040 */
	volatile PMAN_CON_HUB_ADDRReg hub1_start_addr;	/* +0x044 */
	volatile PMAN_CON_HUB_ADDRReg hub1_end_addr;	/* +0x048 */
	volatile PMAN_CON_HUB_ADDRReg hub2_start_addr;	/* +0x04c */
	volatile PMAN_CON_HUB_ADDRReg hub2_end_addr;	/* +0x050 */
	volatile U32 pad1[NW(0x054,0xffc)];	/* +0x054 */
	volatile U32 module_id;			/* +0xffc */
#else
# undef PMAN_HUB_REMAP
# if !defined(CONFIG_SIGMA_SOC_SX6) && !defined(CONFIG_SIGMA_SOC_UXLB)
	#warning "unknown chipset, pman_con is disabled"
# endif
	volatile U32 dummy;	/* dummy used to pass compiling */
#endif
};

/**
 * @brief	get the base address of specified umac.
 * @fn		const U32 umac_get_addr(int uid);
 * @param[in]	uid	umac id (0,1,...)
 * @retval	addr	base address
 */
static inline const U32 umac_get_addr(int uid)
{
#	ifndef CONFIG_REG_BASE_PMAN_CON
#	 define CONFIG_REG_BASE_PMAN_CON 0xf5004000
#	endif
	const struct umac_device* dev=umac_get_dev_by_id(uid);
	if (dev != NULL) {
# ifdef PMAN_HUB_REMAP
		if (dev->quirks & UMAC_QUIRK_VARIABLE_ADDR) {
			const volatile PMAN_CON_HUB_ADDRReg *hub_start;
			const struct pman_con *con =
				(struct pman_con*)CONFIG_REG_BASE_PMAN_CON;
			hub_start = &con->hub0_start_addr + uid * 2;
			return (hub_start->bits.rank << 28);
		} else {
			return dev->addr;
		}
# else /*PMAN_HUB_REMAP*/
		return dev->addr;
# endif /*PMAN_HUB_REMAP*/
	} else {
		return 0;	/*return zero though it's not good*/
	}
}

#else /*__ASSEMBLY__*/

/**
 * umac device object initializer helper (assembly variant)
 *   mac - MAC base
 *   phy - PHY base
 *   abt - Arbiter base
 *   w   - data width, number of bits
 *   q   - quirks
 *   a   - base addr
 */
#ifdef CONFIG_ARM64
#define UMAC_DEVICE_INIT(mac,phy,abt,w,q,a) 			\
	.quad mac,phy,abt;					\
	.long w,q,a;
#else
#define UMAC_DEVICE_INIT(mac,phy,abt,w,q,a) 			\
	.long mac,phy,abt,w,q,a;
#endif

.macro umac_get_dev rd, rtbl, rid
	mov	\rd, #SIZEOF_UMAC_DEV
	mul	\rd, \rd, \rid
	add	\rd, \rd, \rtbl
.endm

.macro umac_dev_mac rd, rdev
	ldr	\rd, [\rdev, #UDEV_OFS_MAC]
.endm

.macro umac_dev_phy rd, rdev
	ldr	\rd, [\rdev, #UDEV_OFS_PHY]
.endm

.macro umac_dev_abt rd, rdev
	ldr	\rd, [\rdev, #UDEV_OFS_ABT]
.endm

.macro umac_dev_width rd, rdev
	ldr	\rd, [\rdev, #UDEV_OFS_WIDTH]
.endm

.macro umac_dev_quirks rd, rdev
	ldr	\rd, [\rdev, #UDEV_OFS_QUIRKS]
.endm

.macro umac_dev_addr rd, rdev
	ldr	\rd, [\rdev, #UDEV_OFS_ADDR]
.endm

#endif /*__ASSEMBLY__*/

#endif /*__ASM_ARCH_TRIX_MACH_UMAC_H__*/
