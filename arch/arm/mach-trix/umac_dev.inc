/*
 *  umac_dev.inc
 *
 *  Copyright (c) 2016 Sigma Designs Limited
 *  All Rights Reserved
 *
 *  This file contains initializer for UMAC devices. The code
 *  could be used by either c or assembly code. It supports
 *  up to 3 umacs at most by now.
 *
 *  Usages:
 *  1. machine macro shall be given in CONFIG_SIGMA_SOC_XX
 *     form.
 *  2. the host has to define CONFIG_SIGMA_NR_UMACS.
 *  3. the host could override UMAC reg base by defining
 *     CONFIG_REG_BASE_UMACx (except UXLB).
 *  4. the host could override UMAC data width by defining
 *     CONFIG_UMACx_WIDTH.
 *
 * Author: Tony He <tony_he@sigmadesigns.com>
 * Date:   05/17/2016
 */

#ifndef NULL
# ifndef __ASSEMBLY__
#  define NULL ((void *) 0)
# else
#  define NULL 0
# endif
#endif

/*Guess base addresses for umacs if necessary*/
#if !defined(CONFIG_REG_BASE_UMAC0)
# if defined(CONFIG_SIGMA_SOC_UXLB)
#  define CONFIG_REG_BASE_UMAC0 0xfb010000
# else
#  define CONFIG_REG_BASE_UMAC0 0xf5021000
# endif
#endif
#if !defined(CONFIG_REG_BASE_UMAC1) && (CONFIG_SIGMA_NR_UMACS > 1)
# define CONFIG_REG_BASE_UMAC1 0xf5023000
#endif
#if !defined(CONFIG_REG_BASE_UMAC2) && (CONFIG_SIGMA_NR_UMACS > 2)
# define CONFIG_REG_BASE_UMAC2 0xf503b000
#endif

/*Guess umac data width for umacs if necessary*/
#if !defined(CONFIG_UMAC0_WIDTH)
# define CONFIG_UMAC0_WIDTH UMAC_DATA_WIDTH32
#endif
#if !defined(CONFIG_UMAC1_WIDTH) && (CONFIG_SIGMA_NR_UMACS > 1)
# if defined(CONFIG_SIGMA_SOC_SX6)
#  define CONFIG_UMAC1_WIDTH UMAC_DATA_WIDTH16
# else
#  define CONFIG_UMAC1_WIDTH UMAC_DATA_WIDTH32	/*might change*/
# endif
#endif
#if !defined(CONFIG_UMAC2_WIDTH) && (CONFIG_SIGMA_NR_UMACS > 2)
# define CONFIG_UMAC2_WIDTH UMAC_DATA_WIDTH16	/*might change*/
#endif

/*
 * SX7 onwards, UMAC HUBn start/end address are configureable via MMIO:
 * hubn_start_addr/hubn_end_addr.
 * The current configuration have below result (new map):
 *  UMAC0: 0x00000000
 *  UMAC1: 0x80000000
 *  UMAC2: 0x40000000
 * Checkout PMAN_IP_1820_CON SPG for details.
 */
#if defined(CONFIG_SIGMA_SOC_SX6)
# define UMAC_DFT_QUIRKS	0
#else
# define UMAC_DFT_QUIRKS	UMAC_QUIRK_VARIABLE_ADDR
#endif

/* sanity check over CONFIG_SIGMA_NR_UMACS */
#ifndef CONFIG_SIGMA_NR_UMACS
# error "CONFIG_SIGMA_NR_UMACS is not defined!"
#endif

#if CONFIG_SIGMA_NR_UMACS < 1 || CONFIG_SIGMA_NR_UMACS > 3
# error "Misconfigured CONFIG_SIGMA_NR_UMACS value!"
#endif

/*
 * UMAC0 initializer
 */
#if defined(CONFIG_SIGMA_SOC_UXLB)
# define DEFINE_UMAC_DEV0						\
	UMAC_DEVICE_INIT(NULL,CONFIG_REG_BASE_UMAC0,0xf5022000,		\
			CONFIG_UMAC0_WIDTH,				\
			UMAC_QUIRK_LEGACY_TR_OFS|UMAC_QUIRK_BROKEN_MAC,	\
			0x00000000)
#else
# define DEFINE_UMAC_DEV0						\
	UMAC_DEVICE_INIT((CONFIG_REG_BASE_UMAC0-0x1000),		\
			CONFIG_REG_BASE_UMAC0,NULL,CONFIG_UMAC0_WIDTH,	\
			UMAC_DFT_QUIRKS,0x00000000)
#endif

/*
 * UMAC1 initializer
 */
#if CONFIG_SIGMA_NR_UMACS > 1
# define DEFINE_UMAC_DEV1						\
	UMAC_DEVICE_INIT((CONFIG_REG_BASE_UMAC1-0x1000),		\
			CONFIG_REG_BASE_UMAC1,NULL,CONFIG_UMAC1_WIDTH,	\
			UMAC_DFT_QUIRKS,0x80000000)
#else
# define DEFINE_UMAC_DEV1	/*nothing*/
#endif

/*
 * UMAC2 initializer
 */
#if CONFIG_SIGMA_NR_UMACS > 2
# define DEFINE_UMAC_DEV2						\
	UMAC_DEVICE_INIT((CONFIG_REG_BASE_UMAC2-0x1000),		\
			CONFIG_REG_BASE_UMAC2,NULL,CONFIG_UMAC2_WIDTH,	\
			UMAC_DFT_QUIRKS,0x40000000)
#else
# define DEFINE_UMAC_DEV2	/*nothing*/
#endif

/*
 * Umac devices initializer
 */
#define UMAC_DEV_INITIALIZER	\
	DEFINE_UMAC_DEV0	\
	DEFINE_UMAC_DEV1	\
	DEFINE_UMAC_DEV2
