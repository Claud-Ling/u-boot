
/*********************************************************************
 Copyright (C) 2013-2017
 Sigma Designs, Inc.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 as
 published by the Free Software Foundation.
 *********************************************************************/

#ifndef __DTV_SMC_H__
#define __DTV_SMC_H__

#ifdef CONFIG_SIGMA_TEE_ARMOR
# include "armv7/armor_smc.h"
#elif defined (CONFIG_SIGMA_TEE_OPTEE)
# include "arm-smccc.h"
#else
# error "unknown TEE definition"
#endif

#ifndef __ASSEMBLY__

/*
 * @fn		int secure_svc_probe(void);
 * @brief	probe secure service
 * @return	0 on success. Otherwise error code
 */
int secure_svc_probe(void);

/*
 * @fn		int secure_get_security_state(void);
 * @brief	get execution secure state
 * @return	0 - non-secure, 1 - secure
 */
int secure_get_security_state(void);
#define get_security_state secure_get_security_state

/*
 * @fn		int secure_l2x0_set_reg(const uint32_t ofs, const uint32_t val);
 * @brief	request tee to set L2 cache registers (A9 only, deprecated)
 * @param[in]	<ofs>  - register offset offset
 * @param[in]	<val>  - value to set with
 * @return	0 on success. Otherwise error code
 */
int secure_l2x0_set_reg(const uint32_t ofs, const uint32_t val);

/*
 * @fn		int secure_set_mem_protection(const uintptr_t va, const uint32_t sz);
 * @brief	request tee to setup pman protection
 * @param[in]	<va>   - pman table buffer address
 * @param[in]	<sz>   - table buffer size
 * @return	0 on success. Otherwise error code
 */
int secure_set_mem_protection(const uintptr_t va, const uint32_t sz);

/*
 * @fn		int secure_otp_get_fuse_mirror(const uint32_t offset, uint32_t *pval, uint32_t *pprot);
 * @brief	request tee to read fuse mirror, must from NS world
 * @param[in]	<offset> - fuse offset
 * @param[out]	<pval>   - buffer pointer
 * @param[out]	<pprot>  - pointer of integar to load protection value on return, or NULL to ignore
 * @return	0 on success. Otherwise error code
 */
int secure_otp_get_fuse_mirror(const uint32_t offset, uint32_t *pval, uint32_t *pprot);

/*
 * @fn		int secure_otp_get_fuse_array(const uint32_t offset, uint32_t *buf, const uint32_t nbytes, uint32_t *pprot);
 * @brief	request tee to read fuse array, must from NS world
 * @param[in]	<offset> - fuse offset
 * @param[out]	<buf>    - buffer pointer, or NULL to read protection only
 * @param[in]	<nbytes> - buffer length
 * @param[out]	<pprot>  - pointer of integar to load protection value on return, or NULL to ignore
 * @return	return 0 and fuse value filled in buf on success. Otherwise error code
 */
int secure_otp_get_fuse_array(const uint32_t offset, uint32_t *buf, const uint32_t nbytes, uint32_t *pprot);


/*
 * @fn		int secure_read_reg(const uint32_t mode, const uint32_t pa, uint32_t *pval);
 * @brief	secure read register
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
int secure_read_reg(const uint32_t mode, const uint32_t pa, uint32_t *pval);

/*
 * @fn		int secure_write_reg(uint32_t mode, uint32_t pa, uint32_t val, uint32_t mask);
 * @brief	secure write register, supports mask
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

/*
 * @fn		int secure_get_rsa_pub_key(uint32_t *buf, const uint32_t nbytes);
 * @brief	request tee to load the selected rsa public key, must from NS world
 * @param[out]	<buf>    - buffer pointer
 * @param[in]	<nbytes> - buffer length
 * @return	return 0 and rsa public key filled in buf on success. Otherwise error code
 */
int secure_get_rsa_pub_key(uint32_t *buf, const uint32_t nbytes);

#endif /* !__ASSEMBLY__ */
#endif /* __DTV_SMC_H__ */
