#include <common.h>
#include <errno.h>
#include <memalign.h>
#include <asm/arch/setup.h>
#include <asm/arch/reg_io.h>

/**************************************************************************/
/* cortex-a9 security state control					  */
/**************************************************************************/

static int security_state = 1;	/*0 - NS, 1 - Secure*/

#ifdef DEBUG
static const char* reg_access_mode(int mode)
{
	char *str = "unknown";
	switch(mode) {
	case 0: str = "byte"; break;
	case 1: str = "hword"; break;
	case 2: str = "word"; break;
	default: break;
	}
	return str;
}

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
#define armor2linuxret(ret)	((ret) == RM_OK ? 0 : 1)

/*
 * otp_access_code
 * otp access code definition
 * this enum is inherited from armor/include/otp.h
 */
enum otp_access_code{
	OTP_ACCESS_CODE_FUSE_MIRROR = 0,	/*read fuse mirror, arg0 - fuse offset, return fuse value on success*/
	OTP_ACCESS_CODE_FUSE_ARRAY,		/*read fuse array, arg0 - fuse offset, arg1 - phy addr of buf, arg2 - buf length, return RM_OK on success*/
};

struct secure_reg_access {
	uint32_t addr;	/* address */
	uint32_t amask;	/* address mask */
#define OP_MODE_MASK	0x3
#define OP_MODE_BYTE	(0 << 0)
#define OP_MODE_HWORD	(1 << 0)
#define OP_MODE_WORD	(2 << 0)
#define OP_ACCESS_MASK	(0x3 << 2)
#define OP_ACCESS_RD	(1 << 2)
#define OP_ACCESS_WR	(1 << 3)
	uint32_t op;	/* operation value*/
};

/*
 * secure register access table. All NS access to those regiters will be routed to armor.
 * it shall be an superset of reg_access_table in armor/flow/reg_access.c
 */
static struct secure_reg_access reg_access_tbl[] =
{
#if defined(CONFIG_SIGMA_SOC_SX6) || defined(CONFIG_SIGMA_SOC_SX7) || defined(CONFIG_SIGMA_SOC_SX8)
	{0xf5005000, 0xfffff000, OP_ACCESS_RD | OP_ACCESS_WR},	/* PMAN_SEC0 (4k) */
	{0xf5008000, 0xfffff000, OP_ACCESS_RD | OP_ACCESS_WR},	/* PMAN_SEC1 (4k) */
# ifndef CONFIG_SIGMA_SOC_SX6
	{0xf5036000, 0xfffff000, OP_ACCESS_RD | OP_ACCESS_WR},	/* PMAN_SEC2 (4k) */
# endif
	{0xf5002000, 0xfffff000, OP_ACCESS_RD},			/* PLF_MMIO_Security (4k) */
	{0xf5003000, 0xfffff000, OP_ACCESS_RD | OP_ACCESS_WR},	/* PLF_MMIO_Configure (4k) */
	{0xf0016000, 0xfffff000, OP_ACCESS_RD},			/* AV_MMIO_Security (4k) */
	{0xf0017000, 0xfffff000, OP_ACCESS_RD | OP_ACCESS_WR},	/* AV_MMIO_Configure (4k) */
	{0xfa000000, 0xfffff000, OP_ACCESS_RD},			/* DISP_MMIO_Security (4k) */
	{0xfa001000, 0xfffff000, OP_ACCESS_RD | OP_ACCESS_WR},	/* DISP_MMIO_Configure (4k) */
# ifdef CONFIG_SIGMA_SOC_SX6
	{0xf5100000, 0xfffe0000, OP_ACCESS_RD | OP_ACCESS_WR},	/* Turing (128k) */
# elif defined(CONFIG_SIGMA_SOC_SX7) || defined(CONFIG_SIGMA_SOC_SX8)
	{0xf1040000, 0xfffe0000, OP_ACCESS_RD | OP_ACCESS_WR},	/* Turing (128k) */
# endif
#endif
	{-1, -1,}	/* the end */
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

int secure_get_security_state(void)
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

int get_security_state(void)
	__attribute__((alias("secure_get_security_state")));

int secure_set_mem_protection(uint32_t pa, uint32_t sz)
{
	uint64_t ret = 0;
	ret = armor_call(set_mem_protection, pa, sz);
	return armor2linuxret(ret);
}

/**************************************************************************/
/* l2x0 control wrapper						  */
/**************************************************************************/
void secure_l2x0_set_reg(void* base, uint32_t ofs, uint32_t val)
{
	/*
	 * Program PL310 Secure R/W registers
	 * So far secure monitor only supports l2x0 regs
	 * L2X0_CTRL
	 * L2X0_AUX_CTRL
	 * L2X0_DEBUG_CTRL
	 * L310_PREFETCH_CTRL
	 */
	debug("program l2x0 regs (ofs %#x) -> %#x\n", ofs, val);
	if ( security_state ) {
		writel(val, base + ofs);
	} else {
		armor_call(set_l2_reg, ofs, val);
	}
}

#define is_secure_accessible(a, t) ({					\
	int _ret = false;						\
	struct secure_reg_access *_e;					\
	for (_e=&reg_access_tbl[0]; _e->addr != -1; _e++) {		\
		if (!(((a) ^ _e->addr) & _e->amask) &&			\
		((t) & _e->op)) {					\
			debug("hit entry: %08x %08x %08x, %08x:%x\n", _e->addr, _e->amask, _e->op, (a), (t));						\
			_ret = true;					\
			break;						\
		}							\
	}								\
	_ret;								\
})

int secure_read_reg(uint32_t mode, uint32_t pa, uint32_t *pval)
{
	//SMC_DEBUG("read_reg_%s(%#x)\n", reg_access_mode(mode), pa);
	BUG_ON(pval == NULL);
	if ( !security_state && is_secure_accessible(pa, OP_ACCESS_RD)) {
		union {
			uint64_t data;
			struct {
				uint32_t val;	/* [31:0] */
				uint32_t ret;	/* [63:32] */
			};
		} tmp;

		tmp.data = armor_call(read_reg, mode, pa);
		if (RM_OK == tmp.ret) {
			*pval = tmp.val;
		} else {
			debug_cond(1, "read_reg_uint%d(0x%08x) failed!\n", 8 << mode, pa);
			*pval = 0;	/*fill with 0 whatever*/
			return -EACCES;
		}
	} else {
		uintptr_t addr = pa;
		if (mode == 0)
			*pval = __raw_readb((void*)addr);
		else if (mode == 1)
			*pval = __raw_readw((void*)addr);
		else if (mode == 2)
			*pval = __raw_readl((void*)addr);
		else
			*pval = 0;
	}
	return 0;
}

int secure_write_reg(uint32_t mode, uint32_t pa, uint32_t val, uint32_t mask)
{
	//SMC_DEBUG("write_reg_%s(%#x, %#x, %#x)\n", reg_access_mode(mode), pa, val, mask);
	if ( !security_state && is_secure_accessible(pa, OP_ACCESS_WR)) {
		uint32_t ret = armor_call(write_reg, mode, pa, val, mask);
		if (ret != RM_OK) {
			debug_cond(1, "write_reg_uint%d(0x%08x, 0x%08x, 0x%08x) failed!\n", 8 << mode, pa, val, mask);
			return -EACCES;
		}
	} else {
		uintptr_t addr = pa;
		if (mode == 0) {
			if (mask != 0xff)
				MWriteReg(Byte, pa, val, mask);
			else
				__raw_writeb(val, (void*)addr);
		} else if (mode == 1) {
			if (mask != 0xffff)
				MWriteReg(HWord, pa, val, mask);
			else
				__raw_writew(val, (void*)addr);
		} else if (mode == 2) {
			if (mask != 0xffffffff)
				MWriteReg(Word, pa, val, mask);
			else
				__raw_writel(val, (void*)addr);
		} else {
			error("unknown access mode value (%d)\n", mode);
		}
	}
	return 0;
}

int secure_otp_get_fuse_mirror(const uint32_t offset, uint32_t *pval)
{
	union {
		uint64_t data;
		struct {
			uint32_t val;	/* [31:0] */
			uint32_t ret;	/* [63:32] */
		};
	} tmp;

	if ( security_state ) {
		error("error: call to %s in Secure world!\n", __func__);
		return -EAGAIN;
	}

	tmp.data = armor_call(otp_access, OTP_ACCESS_CODE_FUSE_MIRROR, offset, 0, 0);
	BUG_ON(pval == NULL);
	if (RM_OK == tmp.ret) {
		*pval = tmp.val;
		return 0;
	} else {
		*pval = 0;
		return -EACCES;
	}
}

int secure_otp_get_fuse_array(const uint32_t offset, uint32_t *buf, uint32_t nbytes)
{
	if ( security_state ) {
		error("error: call to %s in Secure world!\n", __func__);
		return -EAGAIN;
	} else {
		uint32_t ret;
		uintptr_t addr;
		ALLOC_CACHE_ALIGN_BUFFER(void, tmp, nbytes);
		addr = (uintptr_t)tmp;
		if (tmp != NULL) {
			flush_dcache_range(addr, ALIGN(addr + nbytes, ARCH_DMA_MINALIGN));
			ret = (uint32_t)armor_call(otp_access, OTP_ACCESS_CODE_FUSE_ARRAY,
							offset, addr, nbytes);
			if (RM_OK == ret) {
				BUG_ON(buf == NULL);
				memcpy(buf, tmp, nbytes);
				ret = 0;
			} else {
				ret = -EACCES;
			}
		} else {
			ret = -ENOMEM;
		}
		return ret;
	}
}

