
/*********************************************************************
 Copyright (C) 2001-2007
 Sigma Designs, Inc.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 as
 published by the Free Software Foundation.
 *********************************************************************/

#ifndef __DTV_SMC_H__
#define __DTV_SMC_H__

#ifdef CONFIG_CPU_V7
# include "armv7/armor_smc.h"
#elif defined(CONFIG_ARM64)
# include "armv8/armor_smc.h"
#else
# error "unknown ARCH!!"
#endif

#define ARMOR_SMCCALL_WRITE            2
#define ARMOR_SMCCALL_READ             3
#define ARMOR_SMCCALL_SHELL            4

/*
 * NOTE: these have been copied from
 * repo/hwdep_xos/xos/xboot4/armor/include/armor_smc_call.h
 * Report any modification over there!
 */
#define ARMOR_SMC_set_l2_control		0x102
#define ARMOR_SMC_get_aux_core_boot		0x103
#define ARMOR_SMC_set_aux_core_boot0		0x104
#define ARMOR_SMC_set_aux_core_boot_addr	0x105
#define ARMOR_SMC_set_l2_debug			0x106
#define ARMOR_SMC_set_l2_aux_control		0x109

#define ARMOR_SMC_set_l2_reg			0x119
#define ARMOR_SMC_alloc				0x11a
#define ARMOR_SMC_free				0x11b
#define ARMOR_SMC_read_reg			0x11d
#define ARMOR_SMC_write_reg			0x11e
#define ARMOR_SMC_otp_access			0x11f
#define ARMOR_SMC_set_mem_protection		0x121
#define ARMOR_SMC_scale_cpufreq			0x122

#ifndef __ASSEMBLY__
/*
 * you're supposed to land here after having #defined armor_smc_* to a macro that implements your
 * layer crossing facility. Note and observe the absence of semi-colon below.
 */
armor_smc_call1(uint32_t, set_l2_control, uint32_t, val)
armor_smc_call1(uint32_t, set_l2_debug, uint32_t, val)
armor_smc_call1(uint32_t, set_l2_aux_control, uint32_t, val)
armor_smc_call2(uint32_t, set_l2_reg, uint32_t, ofs, uint32_t, val)
armor_smc_call2(uint64_t, read_reg, uint32_t, mode, uint32_t, addr)
armor_smc_call2(uint64_t, set_mem_protection, uint32_t, pa, uint32_t, sz)
armor_smc_call4(uint32_t, write_reg, uint32_t, mode, uint32_t, addr, uint32_t, val, uint32_t, mask)
armor_smc_call4(uint64_t, otp_access, uint32_t, opc, uint32_t, arg0, uint32_t, arg1, uint32_t, arg2)
armor_smc_call1(uint32_t, scale_cpufreq, uint32_t, target)

#undef armor_smc_call0v
#undef armor_smc_call0
#undef uint64_armor_smc_call0
#undef armor_smc_call1v
#undef armor_smc_call1
#undef armor_smc_call2v
#undef armor_smc_call2
#undef armor_smc_call3
#undef armor_smc_call4

/* Keep legacy api for common/cmd_armor.c */
/* armor api */
uint32_t do_smc_call( int code, int arg1, void* arg2);
int secure_get_security_state(void);

/*
 * Return current security state
 * 0 - Non-secure, 1 - secure
 */
int get_security_state(void);

/* smc wrapper api */
void secure_l2x0_set_reg(void* base, uint32_t ofs, uint32_t val);

/*
 * @fn		int secure_set_mem_protection(uint32_t pa, uint32_t sz);
 * @brief	ask armor to help setup pman protection
 * @param[in]	<pa>  - pman table buffer address
 * @param[in]	<sz>    - table buffer size
 * @return	0 on success. Otherwise error code
 */
int secure_set_mem_protection(uint32_t pa, uint32_t sz);

/*
 * @fn		int secure_otp_get_fuse_mirror(const uint32_t offset, uint32_t *pval);
 * @brief	ask armor to read fuse mirror, must from NS world
 * @param[in]	<offset>  - fuse offset
 * @param[out]	<pval>    - buffer pointer
 * @return	0 on success. Otherwise error code
 */
int secure_otp_get_fuse_mirror(const uint32_t offset, uint32_t *pval);

/*
 * @fn		int secure_otp_get_fuse_array(const uint32_t offset, uint32_t *buf, uint32_t nbytes);
 * @brief	ask armor to read fuse array, must from NS world
 * @param[in]	<offset> - fuse offset
 * @param[out]	<buf>    - buffer pointer
 * @param[in]	<nbytes> - buffer length
 * @return	return 0 and fuse value filled in buf on success. Otherwise error code
 */
int secure_otp_get_fuse_array(const uint32_t offset, uint32_t *buf, uint32_t nbytes);


/*
 * @fn		int secure_read_reg(uint32_t mode, uint32_t pa, uint32_t *pval);
 * @brief	secure read one register
 * 		user shall rarely use this API, go with secure_read_uintx for instead.
 * @param[in]	<mode> - specifies access mode
 * 		0 - byte
 * 		1 - halfword
 * 		2 - word
 * 		others - reserved
 * @param[in]	<pa>   - specifies reg physical addr
 * @param[out]	<pval> - pointer of buffer
 * @return	0 on success. Otherwise error code
 */
int secure_read_reg(uint32_t mode, uint32_t pa, uint32_t *pval);

/*
 * @fn		int secure_write_reg(uint32_t mode, uint32_t pa, uint32_t val, uint32_t mask);
 * @brief	secure write one register, supports mask
 * 		user shall rarely use this API, go with secure_write_uintx for instead.
 * @param[in]	<mode> - specifies access mode
 * 		0 - byte
 * 		1 - halfword
 * 		2 - word
 * 		others - reserved
 * @param[in]	<pa>   - specifies reg physical addr
 * @param[in]	<val>  - value to write
 * @param[in]	<mask> - mask for write
 * @return	0 on success. Otherwise error code
 */
int secure_write_reg(uint32_t mode, uint32_t pa, uint32_t val, uint32_t mask);
#define secure_read_generic(pa, m, t) ({					\
				uint32_t _tmp;					\
				secure_read_reg(m, (uint32_t)pa, &_tmp);	\
				(t)_tmp;					\
})
#define secure_read_uint8(pa)	secure_read_generic(pa, 0, uint8_t)
#define secure_read_uint16(pa)	secure_read_generic(pa, 1, uint16_t)
#define secure_read_uint32(pa)	secure_read_generic(pa, 2, uint32_t)
#define secure_write_uint8(pa, val, m)	secure_write_reg(0, (uint32_t)pa, (uint32_t)v, (uint32_t)m)
#define secure_write_uint16(pa, val, m)	secure_write_reg(1, (uint32_t)pa, (uint32_t)v, (uint32_t)m)
#define secure_write_uint32(pa, val, m)	secure_write_reg(2, (uint32_t)pa, (uint32_t)v, (uint32_t)m)

#endif /*__ASSEMBLY__*/


#endif // __DTV_SMC_H__
