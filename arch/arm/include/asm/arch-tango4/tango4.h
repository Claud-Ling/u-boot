
/*********************************************************************
 Copyright (C) 2001-2007
 Sigma Designs, Inc. 

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 as
 published by the Free Software Foundation.
 *********************************************************************/

#ifndef __TANGO4_H__
#define __TANGO4_H__

#include "rmdefs.h"
#include <asm/arch/addrspace.h>

#define CPU_TMP_REMAP               CPU_remap7
#define CPU_TMP_REMAP_address       CPU_remap7_address
#define CPU_PFLASH_REMAP            CPU_remap6
#define CPU_PFLASH_REMAP_address    CPU_remap6_address

/* xenv related defines */
#define MAX_XENV_SIZE               16384
#define MAX_LR_XENV2_RO             628
#define MAX_LR_XENV2_RW             628

#define LR_ETH_MAC_LO 0xc0
#define LR_ETH_MAC_HI 0xc4


// Host Memories
#define MEMORY_BASE_HOST_PB0        0x40000000  // Peripheral Bus CS #0 Memory 
#define MEMORY_BASE_HOST_PB_CS(n)   (MEMORY_BASE_HOST_PB0 + (0x04000000 * (n)))


#define RM_OK 0
#define RM_ERROR 1
#define RM_NOT_FOUND 2
#define RM_INSUFFICIENT_SIZE 3
#define RM_INVALIDMODE 4
#define RM_NOT_SUPPORTED 5
#define RM_PENDING 7

/*
#define RMmustBeEqual(x,y,seed)                 \
typedef RMascii XXX ## seed ## LeftIsBiggerNow[(y)-(x)];           \
typedef RMascii XXX ## seed ## LeftIsSmallerNow[(x)-(y)];
*/
/*
#define RMleftMustBeSmaller(x,y,seed)                 \
typedef RMascii XXX ## seed ## LeftIsBiggerNow[(y)-(x)];           
*/

#define RMunshiftBitsVar(data, bits, shift) ( \
        (((RMuint32)(data)) >> (shift)) & ((1 << (bits)) - 1) )
#define RMunshiftBits RMunshiftBitsVar


/* armor api */
#define ARMOR_SMCCALL_WRITE            2
#define ARMOR_SMCCALL_READ             3
#define ARMOR_SMCCALL_SHELL            4
#define ARMOR_SMCCALL_SET_L2_CONTROL    0x102
#define ARMOR_SMCCALL_GET_AUX_CORE_BOOT 0x103
#define ARMOR_SMCCALL_SET_AUX_CORE_BOOT0 0x104
#define ARMOR_SMCCALL_SET_AUX_CORE_BOOT_ADDR 0x105

#endif // __TANGO3_H__

