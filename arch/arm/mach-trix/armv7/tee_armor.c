/**
  @file   tee_armor.c
  @brief
	This file decribes ree glue codes for sigma designs defined TOS - armor.
	This is for legacy A9 projects only.

  @author Tony He, tony_he@sigmadesigns.com
  @date   2017-04-25
  */

#include <common.h>
#include <errno.h>
#include <memalign.h>
#include <asm/arch/setup.h>
#include <asm/arch/reg_io.h>
#include "../tee_service.h"

#define PHYADDR(a) ((uintptr_t)(a))	/* u-boot utilizes idmap */

/**************************************************************************/
/* cortex-a9 security state control					  */
/**************************************************************************/

static int security_state = EXEC_STATE_SECURE;	/*0 - NS, 1 - Secure*/

#ifdef DEBUG
# define SMC_DEBUG(fmt...) do{debug(fmt);}while(0)
#else
# define SMC_DEBUG(fmt...) do{}while(0)
#endif

#define RM_OK 0x6	/*check armor code*/

#define armor_call_body(dbg, nm, args...)	({		\
	volatile uint64_t _ret;					\
	_ret = armor_smc_##nm(args);				\
	if (dbg)						\
	  SMC_DEBUG("armor_call_%s result: %llx\n", #nm, _ret);	\
	_ret;							\
})

#define armor_call(nm, args...)	armor_call_body(1, nm, args)
#define armor2linuxret(ret)	((ret) == RM_OK ? 0 : -EIO)

/*
 * otp_access_code
 * otp access code definition
 * this enum is inherited from armor/include/otp.h
 */
enum otp_access_code{
	OTP_ACCESS_CODE_FUSE_MIRROR = 0,	/*read fuse mirror, arg0 - fuse offset, return fuse value on success*/
	OTP_ACCESS_CODE_FUSE_ARRAY,		/*read fuse array, arg0 - fuse offset, arg1 - phy addr of buf, arg2 - buf length, return RM_OK on success*/
};

/*
 * probe core executing state
 * return value
 *   0 in case of non-secure
 *   1 in case of secure
 *
 */
static int is_execute_in_secure_state(void)
{
	#define MTESTREG 0xf2101080	/*GICDISR0*/
	#define MTESTBIT (1 << 27)	/*Global Timer Interrupt*/
	volatile unsigned long *addr = (volatile unsigned long*)MTESTREG;
	unsigned long bak = 0, val = 0;

	/*
	 * GICDISR0
	 *  r/w in secure state
	 *  RAZ/WI in non-secure state
	 */
	bak = *addr;
	*addr = (bak | MTESTBIT);
	val = *addr;

	if (val != 0) {
		*addr = bak;	/*recover*/
		return 1;
	} else {
		return 0;
	}
}

static int armor_get_security_state(void)
{
#if defined(CONFIG_SIGMA_SOC_SX6) || defined(CONFIG_SIGMA_SOC_SX7) || defined(CONFIG_SIGMA_SOC_SX8)
	security_state = is_execute_in_secure_state();
#else
# ifdef CONFIG_DTV_BOOTPARAM
	security_state =
		(sx6_boot_flags() & FLAGS_SMC) ? 0 : 1;
# endif
#endif
	debug("security state: %d\n", security_state);
	return security_state;
}

static int armor_set_mem_protection(unsigned long va, unsigned long sz)
{
	uint32_t ret = 0;
	ret = armor_call(set_mem_protection, PHYADDR(va), sz);
	return armor2linuxret(ret);
}

/**************************************************************************/
/* l2x0 control wrapper						  */
/**************************************************************************/
static int armor_set_l2x0_reg(unsigned long ofs, unsigned long val)
{
	/*
	 * Program PL310 Secure R/W registers
	 * So far secure monitor only supports l2x0 regs
	 * L2X0_CTRL
	 * L2X0_AUX_CTRL
	 * L2X0_DEBUG_CTRL
	 * L310_PREFETCH_CTRL
	 */
	uint32_t ret = 0;
	debug("program l2x0 regs (ofs %#lx) -> %#lx\n", ofs, val);
	ret = armor_call(set_l2_reg, ofs, val);
	return armor2linuxret(ret);
}

static int armor_read_reg(uint32_t mode, unsigned long pa, uint32_t *pval)
{
	union {
		uint64_t data;
		struct {
			uint32_t val;	/* [31:0] */
			uint32_t ret;	/* [63:32] */
		};
	} tmp;

	BUG_ON(pval == NULL);
	tmp.data = armor_call(read_reg, mode, pa);
	if (RM_OK == tmp.ret) {
		*pval = tmp.val;
		return 0;
	} else {
		debug("read_reg_uint%d(0x%08lx) failed!\n", 8 << mode, pa);
		*pval = 0;	/*fill with 0 whatever*/
		return -EACCES;
	}
}

static int armor_write_reg(uint32_t mode, unsigned long pa, uint32_t val, uint32_t mask)
{
	uint32_t ret = armor_call(write_reg, mode, pa, val, mask);
	if (ret != RM_OK) {
		debug("write_reg_uint%d(0x%08lx, 0x%08x, 0x%08x) failed!\n", 8 << mode, pa, val, mask);
		return -EACCES;
	}
	return 0;
}

static int armor_mmio(unsigned long mode, unsigned long pa, unsigned long a2, unsigned long a3, unsigned int wnr)
{
	if (wnr) {
		return armor_write_reg(mode, pa, a2, a3);
	} else {
		return armor_read_reg(mode, pa, (uint32_t*)a2);
	}
}

static int armor_otp_get_fuse_mirror(const uint32_t offset, uint32_t *pval)
{
	union {
		uint64_t data;
		struct {
			uint32_t val;	/* [31:0] */
			uint32_t ret;	/* [63:32] */
		};
	} tmp;

	tmp.data = armor_call(otp_access, OTP_ACCESS_CODE_FUSE_MIRROR, offset, 0, 0);
	BUG_ON(pval == NULL);
	if (RM_OK == tmp.ret) {
		*pval = tmp.val;
		return 0;
	} else {
		*pval = 0;
		return -EIO;
	}
}

static int armor_otp_get_fuse_array(const uint32_t offset, uintptr_t buf, uint32_t nbytes)
{
	uint32_t ret;
	ALLOC_CACHE_ALIGN_BUFFER(void, tmp, nbytes);
	flush_dcache_range((uintptr_t)tmp, ALIGN((uintptr_t)tmp + nbytes, ARCH_DMA_MINALIGN));
	ret = (uint32_t)armor_call(otp_access, OTP_ACCESS_CODE_FUSE_ARRAY,
					offset, PHYADDR(tmp), nbytes);
	if (RM_OK == ret) {
		BUG_ON(buf == 0);
		memcpy((void*)buf, tmp, nbytes);
		ret = 0;
	} else {
		ret = -EIO;
	}
	return ret;
}

static int armor_fuse_read(unsigned long ofs, unsigned long va, unsigned int *size, unsigned int *pprot)
{
	uint32_t len;
	(void)pprot;
	if (NULL == size)
		return -EINVAL;
	len = *size;
	if (len == 4) {
		return armor_otp_get_fuse_mirror(ofs, (uint32_t*)va);
	} else {
		return armor_otp_get_fuse_array(ofs, (uintptr_t)va, len);
	}
}

static int armor_get_rsa_key(unsigned long va, unsigned long len)
{
	/*fallback to load key from OTP*/
	return armor_otp_get_fuse_array(CONFIG_FUSE_OFS_RSA_PUB_KEY, (uintptr_t)va, len);
}

TEE_OPS("armor",
	NULL,
	armor_get_security_state,
	armor_set_mem_protection,
	armor_set_l2x0_reg,
	armor_mmio,
	armor_fuse_read,
	armor_get_rsa_key,
	NULL)
