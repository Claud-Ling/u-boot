#include <common.h>
#include <errno.h>
#include <compiler.h>

int __weak secure_svc_probe(void)
{
	debug("In weak symbol %s\n", __func__);
	return 0;
}

/*
 * Weak symbol definition for smc
 */

int __weak secure_get_security_state(void)
{
	debug("In weak symbol %s, always indicate in secure status\n", __func__);
	return 1;
}

int get_security_state(void)
	__attribute__((weak,alias("secure_get_security_state")));

int __weak secure_set_mem_protection(const uintptr_t va, const uint32_t sz)
{
	/* Should never call this function, BUG here */
	BUG();
	return -ENODEV;
}

int __weak secure_l2x0_set_reg(const uint32_t ofs, const uint32_t val)
{
	/* Should never call this function, BUG here */
	BUG();
	return -ENODEV;
}

int __weak secure_read_reg(const uint32_t mode, const uint32_t pa, uint32_t *pval)
{
	/* Should never call this function, BUG here */
	BUG();
	return -ENODEV;
}

int __weak secure_write_reg(uint32_t mode, uint32_t pa, uint32_t val, uint32_t mask)
{
	/* Should never call this function, BUG here */
	BUG();
	return -ENODEV;
}

int __weak secure_otp_get_fuse_mirror(const uint32_t offset, uint32_t *pval, uint32_t *pprot)
{
	/* Should never call this function, BUG here */
	BUG();
	return -ENODEV;
}

int __weak secure_otp_get_fuse_array(const uint32_t offset, uint32_t *buf, uint32_t *size, uint32_t *pprot)
{
	/* Should never call this function, BUG here */
	BUG();
	return -ENODEV;
}

int __weak secure_get_rsa_pub_key(uint32_t *buf, const uint32_t nbytes)
{
	/* Should never call this function, BUG here */
	BUG();
	return -ENODEV;
}
