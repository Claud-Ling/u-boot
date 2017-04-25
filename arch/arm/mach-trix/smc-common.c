#include <common.h>
#include <errno.h>
#include <memalign.h>
#include <asm/arch/setup.h>
#include <asm/arch/reg_io.h>
#include "tee_service.h"

/**************************************************************************/
/* tee operations 							  */
/**************************************************************************/
static struct tee_operations *ops = &tee_ops;

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
#if CONFIG_SIGMA_NR_UMACS > 0
	{0xf5005000, 0xfffff000, OP_ACCESS_RD | OP_ACCESS_WR},	/* PMAN_SEC0 (4k) */
#endif
#if CONFIG_SIGMA_NR_UMACS > 1
	{0xf5008000, 0xfffff000, OP_ACCESS_RD | OP_ACCESS_WR},	/* PMAN_SEC1 (4k) */
#endif
#if CONFIG_SIGMA_NR_UMACS > 2
	{0xf5036000, 0xfffff000, OP_ACCESS_RD | OP_ACCESS_WR},	/* PMAN_SEC2 (4k) */
#endif
	{0xf5002000, 0xfffff000, OP_ACCESS_RD},			/* PLF_MMIO_Security (4k) */
	{0xf5003000, 0xfffff000, OP_ACCESS_RD | OP_ACCESS_WR},	/* PLF_MMIO_Configure (4k) */
	{0xf0016000, 0xfffff000, OP_ACCESS_RD},			/* AV_MMIO_Security (4k) */
	{0xf0017000, 0xfffff000, OP_ACCESS_RD | OP_ACCESS_WR},	/* AV_MMIO_Configure (4k) */
	{0xfa000000, 0xfffff000, OP_ACCESS_RD},			/* DISP_MMIO_Security (4k) */
	{0xfa001000, 0xfffff000, OP_ACCESS_RD | OP_ACCESS_WR},	/* DISP_MMIO_Configure (4k) */
#ifdef CONFIG_SIGMA_TURING_IN_PLF_DCS
	{0xf5100000, 0xfffe0000, OP_ACCESS_RD | OP_ACCESS_WR},	/* Turing (128k) */
#else
	{0xf1040000, 0xfffe0000, OP_ACCESS_RD | OP_ACCESS_WR},	/* Turing (128k) */
#endif
	{-1, -1,}	/* the end */
};

int secure_svc_probe(void)
{
	BUG_ON(ops == NULL);
	if (ops->probe)
		return ops->probe();
	else
		return 0;
}

int secure_get_security_state(void)
{
	BUG_ON(ops == NULL || ops->get_secure_state == NULL);
	return ops->get_secure_state();
}

int secure_set_mem_protection(const uintptr_t va, const uint32_t sz)
{
	int ret;
	BUG_ON(ops == NULL || ops->set_mem_protection == NULL);
	ret = ops->set_mem_protection(va, sz);
	if (ret != TEE_SVC_E_OK) {
		printf("%s failed, error %d\n", __func__, ret);
	}
	return (ret == TEE_SVC_E_OK) ? 0 : -EACCES;
}

/**************************************************************************/
/* l2x0 control wrapper						  */
/**************************************************************************/
int secure_l2x0_set_reg(const uint32_t ofs, const uint32_t val)
{
	int ret = TEE_SVC_E_ERROR;
	BUG_ON(ops == NULL);
	if (ops->set_l2x_reg)
		ret = ops->set_l2x_reg(ofs, val);
	return (ret == TEE_SVC_E_OK) ? 0 : -EACCES;
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

int secure_read_reg(const uint32_t mode, const uint32_t pa, uint32_t *pval)
{
	int ret = 0;
	if (is_secure_accessible(pa, OP_ACCESS_RD)) {
		BUG_ON(ops == NULL || ops->secure_mmio == NULL);
		ret = ops->secure_mmio(mode, pa, (unsigned long)pval, 0, 0);
		return (ret == TEE_SVC_E_OK) ? 0 : -EIO;
	} else {
		uintptr_t addr = pa;
		if (0 == mode)
			*pval = __raw_readb((void*)addr);
		else if (1 == mode)
			*pval = __raw_readw((void*)addr);
		else if (2 == mode)
			*pval = __raw_readl((void*)addr);
		else {
			error("unkown access mode %d\n", mode);
			*pval = 0;
			ret = -EACCES;
		}
		return ret;
	}
}

int secure_write_reg(uint32_t mode, uint32_t pa, uint32_t val, uint32_t mask)
{
	int ret = 0;
	if (is_secure_accessible(pa, OP_ACCESS_WR)) {
		int ret = 0;
		BUG_ON(ops == NULL || ops->secure_mmio == NULL);
		ret = ops->secure_mmio(mode, pa, val, mask, 1);
		return (ret == TEE_SVC_E_OK) ? 0 : -EIO;
	} else {
		uintptr_t addr = pa;
		if (0 == mode) {
			if (mask != 0xff)
				MWriteReg(Byte, pa, val, mask);
			else
				__raw_writeb(val, (void*)addr);
		} else if (1 == mode) {
			if (mask != 0xffff)
				MWriteReg(HWord, pa, val, mask);
			else
				__raw_writew(val, (void*)addr);
		} else if (2 == mode) {
			if (mask != 0xffffffff)
				MWriteReg(Word, pa, val, mask);
			else
				__raw_writel(val, (void*)addr);
		} else {
			error("unknown access mode %d\n", mode);
			ret = -EACCES;
		}
		return ret;
	}
}

int secure_otp_get_fuse_mirror(const uint32_t offset, uint32_t *pval, uint32_t *pprot)
{
	return secure_otp_get_fuse_array(offset, pval, 4, pprot);
}

int secure_otp_get_fuse_array(const uint32_t offset, uint32_t *buf, const uint32_t nbytes, uint32_t *pprot)
{
	int ret;
	BUG_ON(secure_get_security_state() == EXEC_STATE_SECURE);
	BUG_ON(ops == NULL || ops->fuse_read == NULL);
	ret = ops->fuse_read(offset, (uintptr_t)buf, nbytes, pprot);
	if (TEE_SVC_E_OK == ret) {
		return 0;
	} else {
		return -EIO;
	}
}

int secure_get_rsa_pub_key(uint32_t *buf, const uint32_t nbytes)
{
	int ret;
	BUG_ON(secure_get_security_state() == EXEC_STATE_SECURE);
	BUG_ON(ops == NULL || ops->get_rsa_key == NULL);
	ret = ops->get_rsa_key((uintptr_t)buf, nbytes);
	if (TEE_SVC_E_OK == ret) {
		return 0;
	} else {
		return -EIO;
	}
}
