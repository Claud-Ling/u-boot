#include <common.h>

#include <asm/arch/tango4.h>

void do_smc_call( unsigned int, unsigned int, void *);

#ifndef CONFIG_SYS_L2CACHE_OFF
void v7_outer_cache_enable(void)
{
#ifdef CONFIG_TANGOX_SMC
	do_smc_call(ARMOR_SMCCALL_SET_L2_CONTROL, 1, NULL);
#else
	*((volatile unsigned int*)(CONFIG_SYS_PL310_BASE+0x100)) = 0x01;
#endif
}

void v7_outer_cache_disable(void)
{
#ifdef  CONFIG_TANGOX_SMC
	do_smc_call(ARMOR_SMCCALL_SET_L2_CONTROL, 0, NULL);
#else
	*((volatile unsigned int*)(CONFIG_SYS_PL310_BASE+0x100)) = 0x00;
#endif
}
#endif

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
    /* Enable D-cache. I-cache is already enabled in start.S */
    dcache_enable();
}
#endif
