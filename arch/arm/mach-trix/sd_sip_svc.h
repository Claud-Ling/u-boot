/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/**
  @file   sd_sip_svc.h
  @brief
	This file decribes Sigma Designs defines SiP service calls.
	Complies with SMC32 calling convention

  @author Tony He, tony_he@sigmadesigns.com
  @date   2017-04-16
  */

#ifndef __SD_SIP_SVC_H__
#define __SD_SIP_SVC_H__

/* SMC function IDs for SiP Service queries */
#define SIP_SVC_CALL_COUNT		0x8200ff00
#define SIP_SVC_UID			0x8200ff01
/*					0x8200ff02 is reserved */
#define SIP_SVC_VERSION			0x8200ff03

/*
 * Sigma Designs SiP Service UUID
 * 82af283f-67d7-49bb-8323-002a5d926486
 * Represented in 4 32-bit words in SD_SIP_UID_0, SD_SIP_UID_1,
 * SD_SIP_UID_2, SD_SIP_UID_3.
 * Note that SMC_UUID_RET represents UUID in little-endian.
 */
#define SD_SIP_UID_0			0x82af283f
#define SD_SIP_UID_1			0x49bb67d7
#define SD_SIP_UID_2			0x2a002383
#define SD_SIP_UID_3			0x8664925d

/* Sigma Designs SiP Service Calls version numbers */
#define SD_SIP_SVC_VERSION_MAJOR	0x1
#define SD_SIP_SVC_VERSION_MINOR	0x1

/* Number of Sigma Designs SiP Calls implemented */
#define SD_COMMON_SIP_NUM_CALLS		9

/*
 * Sigma Designs defined SiP SMC Calls
 *
--------------------------+---------------------
 SMC Function Identifier  |  SD SMC origine
--------------------------+---------------------
 0x82000000 - 0x820001FF  | SMC32: Common
--------------------------+---------------------
 0x82000200 - 0x820003FF  | SMC32: Secure world
--------------------------+---------------------
 0x82000400 - 0x820005FF  | SMC32: Normal world
--------------------------+---------------------
 0xC2000000 - 0xC20001FF  | SMC64: Common
--------------------------+---------------------
 0xC2000200 - 0xC20003FF  | SMC64: Secure world
--------------------------+---------------------
 0xC2000400 - 0xC20005FF  | SMC64: Normal world
--------------------------+---------------------
 *
 */

#define SD_SIP_FUNC_C_BASE		0x82000000
#define SD_SIP_FUNC_S_BASE		0x82000200
#define SD_SIP_FUNC_N_BASE		0x82000400

/**********************************************************************/
/*  SiP Common Services                                               */
/**********************************************************************/

/*
 * Check memory access state of the specified memory block.
 *
 * "Call" register usage:
 * a0	SD_SIP_FUNC_C_MEM_STATE
 * a1	Upper 32bits of 64-bit physical address to an memory block
 * a2	Lower 32bits of 64-bit physical address to an memory block
 * a3	Size in bytes of the memory block
 * a4-7	Not used
 *
 * "Return" register usage:
 * a0	SD_SIP_E_SUCCESS on success, or other error code otherwise
 * a1	32-bit bitwise access state on success
 *	bit[0]	- 1: secure accessible,   0: secure non-accessible
 *	bit[1]	- 1: non-secure readable, 0: non-secure non-readable
 *	bit[2]	- 1: non-secure writable, 0: non-secure non-writable
 *	bit[3]	- 1: secure executable,   0: secure non-executable
 *	bit[4]	- 1: ns executable,       0: non-secure non-executable
 * a2-3	Not used
 * a4-7	Preserved
 */
#define MEM_STATE_MASK		0x1f	/* mask */
#define MEM_STATE_S_RW		1	/* secure r/w mem */
#define MEM_STATE_NS_RD		(1<<1)	/* non-secure readable mem */
#define MEM_STATE_NS_WR		(1<<2)	/* non-secure writable mem */
#define MEM_STATE_S_EXEC	(1<<3)	/* secure executable */
#define MEM_STATE_NS_EXEC	(1<<4)	/* non-secure executable */

#define SD_SIP_FUNC_C_MEM_STATE		(SD_SIP_FUNC_C_BASE)

/*
 * Request to read OTP data from either secure or normal world.
 * All OTP is accessible to secure world.
 * Only a few OTP that's security irrelavant can be accessied to normal world.
 *
 * "Call" register usage:
 * a0	SD_SIP_FUNC_C_OTP_READ
 * a1	Fuse data offset value
 * a2	Upper 32bits of 64-bit physical address to an memory block
 * a3	Lower 32bits of 64-bit physical address to an memory block
 *	Or set address to 0 to read back protection value only
 * a4	Size in bytes of OTP data to read
 * a5-7	Not used
 *
 * "Return" register usage:
 * a0	SD_SIP_E_SUCCESS on success, or other error code otherwise
 *	OTP data will be filled into the input memory block on success
 * a1	Protection value
 * a2	OTP data size in bytes
 * a3-4	Not used
 * a5-7	Preserved
 */
#define SD_SIP_FUNC_C_OTP_READ		(SD_SIP_FUNC_C_BASE + 1)

/**********************************************************************/
/*  SiP Secure Services                                               */
/**********************************************************************/

/*
 * Request to write OTP data loaded via memory block from secure normal world.
 *
 * "Call" register usage:
 * a0	SD_SIP_FUNC_S_OTP_WRITE
 * a1	Fuse data offset value
 * a2	Upper 32bits of 64-bit physical address to an memory block
 * a3	Lower 32bits of 64-bit physical address to an memory block
 *	Or set address to 0 to write protection value only
 * a4	Size in bytes of OTP data to write
 * a5	Protection value
 * a6-7	Not used
 *
 * "Return" register usage:
 * a0	SD_SIP_E_SUCCESS on success, or other error code otherwise
 * a1	OTP data size in bytes
 * a2-5	Not used
 * a6-7	Preserved
 */
#define SD_SIP_FUNC_S_OTP_WRITE		(SD_SIP_FUNC_S_BASE)


/**********************************************************************/
/*  SiP Normal Services                                               */
/**********************************************************************/

/*
 * Set PMAN settingns via PMAN Secure Table (PST v1.0 or above).
 *
 * "Call" register usage:
 * a0	SD_SIP_FUNC_N_SET_PST
 * a1	Upper 32bits of 64-bit physical address to an memory block
 * a2	Lower 32bits of 64-bit physical address to an memory block
 * a3	Size of data in bytes in the memory block
 * a4-7	Not used
 *
 * "Return" register usage:
 * a0	SD_SIP_E_SUCCESS on success, or other error code otherwise
 * a1-3	Not used
 * a4-7	Preserved
 */
#define SD_SIP_FUNC_N_SET_PST		(SD_SIP_FUNC_N_BASE)

/*
 * Request secure mmio to access certain harmless secure registers
 * from normal world.
 *
 * "Call" register usage:
 * a0	SD_SIP_FUNC_N_MMIO
 * a1	32-bit operation code
 *	31                                         8      4     1 0
 *	 +-----------------------------------------+------------+-+
 *	 |                 RAZ/WI                  | mode |RAZWI| |
 *	 +-----------------------------------------+------------+-+
 *	                                                         |
 *	                                                        WnR
 *
 * a2	Upper 32bits of 64-bit physical address to a register
 * a3	Lower 32bits of 64-bit physical address to a register
 * a4	32-bit value to set with on write. Otherwise not used
 * a5	32-bit mask value on write. Otherwise not used
 * a6-7	Not used
 *
 * "Return" register usage:
 * a0	SD_SIP_E_SUCCESS on success, or other error code otherwise
 * a1	32-bit register value on read succeeds. Otherwise not used.
 * a2-5	Not used
 * a6-7	Preserved
 */
#define SEC_MMIO_MODE_SHIFT		4
#define SEC_MMIO_MODE_MASK		0xF
#define SEC_MMIO_MODE_BYTE		0	/* byte */
#define SEC_MMIO_MODE_HWORD		1	/* halfword */
#define SEC_MMIO_MODE_WORD		2	/* word */

#define SEC_MMIO_MODE(x)		(((x) >> SEC_MMIO_MODE_SHIFT) & SEC_MMIO_MODE_MASK)

#define SEC_MMIO_CMD_MASK		0xF
#define SEC_MMIO_CMD_WNR		1	/* WnR */

#define SD_SIP_FUNC_N_MMIO		(SD_SIP_FUNC_N_BASE + 1)

/*
 * Request to load the selected rsa public key from normal world.
 * Note that the selected key can be from either OTP or ROM, depending on
 * boot_val_user_id fuse bits.
 *
 * "Call" register usage:
 * a0	SD_SIP_FUNC_N_RSA_KEY
 * a1	Upper 32bits of 64-bit physical address to an memory block
 * a2	Lower 32bits of 64-bit physical address to an memory block
 * a3	Size of memory block in bytes, no less than 256
 * a4-7	Not used
 *
 * "Return" register usage:
 * a0	SD_SIP_E_SUCCESS on success, or other error code otherwise
 *	RSA public key will be filled into the input memory block on success
 * a1-3	Not used
 * a4-7	Preserved
 */
#define SD_SIP_FUNC_N_RSA_KEY		(SD_SIP_FUNC_N_BASE + 2)

/**********************************************************************/
/*  SiP Others                                                        */
/**********************************************************************/

/* Sigma Designs SiP Calls error code */
enum {
	SD_SIP_E_SUCCESS = 0,
	SD_SIP_E_INVALID_PARAM = -1,
	SD_SIP_E_NOT_SUPPORTED = -2,
	SD_SIP_E_INVALID_RANGE = -3,
	SD_SIP_E_PERMISSION_DENY = -4,
	SD_SIP_E_LOCK_FAIL = -5,
	SD_SIP_E_SMALL_BUFFER = -6,
	SD_SIP_E_FAIL = -7,
};

#endif /* __SD_SIP_SVC_H__ */
