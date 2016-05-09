#include <common.h>
#include <errno.h>
#include <compiler.h>

/*
 * Weak symbol definition for smc
 */

int __weak secure_get_security_state(void)
{
	debug("In weak symbol %s, always indicate in sceure status\n", __func__);
	return 1;
}

int get_security_state(void)
	__attribute__((weak,alias("secure_get_security_state")));

int __weak secure_set_mem_protection(uint32_t pa, uint32_t sz)
{
	/* Should never call this function, BUG here */
	BUG();
	return -ENODEV;
}

int __weak secure_otp_get_fuse_mirror(const uint32_t offset, uint32_t *pval)
{
	/* Should never call this function, BUG here */
	BUG();
	return -ENODEV;
}

int __weak secure_otp_get_fuse_array(const uint32_t offset, uint32_t *buf, uint32_t nbytes)
{
	/* Should never call this function, BUG here */
	BUG();
	return -ENODEV;
}

void __weak secure_l2x0_set_reg(void* base, uint32_t ofs, uint32_t val)
{
	/* Should never call this function, BUG here */
	BUG();
	return;
}
