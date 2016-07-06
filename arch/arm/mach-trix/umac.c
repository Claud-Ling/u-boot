/*
 *  umac.c
 *
 *  Copyright (c) 2016 Sigma Designs Limited
 *  All Rights Reserved
 *
 *  This file describes the top level table for UMACs on
 *  system. The code is shared by preboot, uboot and linux.
 *  It supports up to 3 umacs at most by now.
 *
 * Author: Tony He <tony_he@sigmadesigns.com>
 * Date:   05/17/2016
 */

#if defined(__PREBOOT__)
#include <config.h>
#include <umac.h>
#elif defined(__UBOOT__)
#include <common.h>
#include <asm/arch/umac.h>
#elif defined(__LINUX__)
#include <linux/kernel.h>
#include <linux/bug.h>
#include "umac.h"
#endif

#include "umac_dev.inc"

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#endif

/*
 * stub trib_udev_tbl
 */
const __attribute__((weak)) \
struct umac_device trix_udev_tbl[CONFIG_SIGMA_NR_UMACS] = {
	UMAC_DEV_INITIALIZER
};

const struct umac_device* umac_get_dev_by_id(int uid)
{
	if (uid >= 0 && uid < ARRAY_SIZE(trix_udev_tbl))
		return &trix_udev_tbl[uid];
	else
		return NULL;
}

/*
 * stub implemenation of umac_is_activated
 */
__attribute__((weak)) int umac_is_activated(int uid)
{
	const struct umac_device* dev = umac_get_dev_by_id(uid);
	if (dev) {
		if (dev->quirks & UMAC_QUIRK_BROKEN_MAC)
			return 1;
		else
			return !!dev->mac->mctl.if_enable;
	} else {
		return 0;
	}
}

