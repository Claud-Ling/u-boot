
/*********************************************************************
 Copyright (C) 2001-2008
 Sigma Designs, Inc.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 as
 published by the Free Software Foundation.
 *********************************************************************/

/*
 * misc vars/func shared by platform setup code
 */

#ifndef __SETUP_H__
#define __SETUP_H__

#include <config.h>

#include <asm/arch/rmdefs.h>
#include <asm/arch/dtv_smc.h>
#ifdef CONFIG_DTV_BOOTPARAM
#include <asm/arch/bootparam.h>
#endif
#include <asm/arch/otp.h>
#ifdef CONFIG_SIGMA_DTV_SECURITY
# ifndef __ASSEMBLY__
  extern int is_secure_enable(void);
# endif
# define SIMGA_TRIX_SECURITY_STATE is_secure_enable()
#else
# define SIMGA_TRIX_SECURITY_STATE 0
#endif

#endif
