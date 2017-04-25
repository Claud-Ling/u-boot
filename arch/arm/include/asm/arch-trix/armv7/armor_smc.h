#ifndef __ARMOR_SMC_H__
#define __ARMOR_SMC_H__

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

#ifdef __ASSEMBLY__

#define armor_smc(call_name, val)            \
	ldr	ip, =ARMOR_SMC_ ##call_name; \
	ldr	r0, =val;                    \
	dsb;                                 \
	smc	#1;

#else /*__ASSEMBLY__*/

#define armor_smc_call0v(type, name)                              \
static inline type armor_smc_##name(void)                         \
{                                                                 \
	register uint32_t code asm("ip") = ARMOR_SMC_ ##name;     \
								  \
	__asm__ __volatile__( ".arch_extension sec\n"             \
				"dsb\n"                           \
				"smc #1\n"                        \
				:                                 \
				: "r" (code)                      \
				: "r0", "r1", "r2", "r3", "memory"\
				);                                \
}

#define armor_smc_call0(type, name)                               \
static inline type armor_smc_##name(void)                         \
{                                                                 \
	register uint32_t code asm("ip") = ARMOR_SMC_ ##name;     \
	register uint32_t v0 asm("r0");                           \
	register uint32_t v1 asm("r1");                           \
								  \
	__asm__ __volatile__( ".arch_extension sec\n"             \
				"dsb\n"                           \
				"smc #1\n"                        \
				: "=r" (v0), "=r" (v1)            \
				: "r" (v0), "r" (code)            \
				: "r2", "r3", "memory"            \
				);                                \
	return (type)((((uint64_t)v1) << 32) + (uint64_t)v0);     \
}

/* can't use RMuint64from2RMuint32 below, cpp doesn't seem to replace it
 * inside the other macro definition... */
#define uint64_armor_smc_call0(name)                              \
static inline uint64_t armor_smc_##name(void)                     \
{                                                                 \
	register uint32_t code asm("ip") = ARMOR_SMC_ ##name;     \
	register uint32_t v0 asm("r0");                           \
	register uint32_t v1 asm("r1");                           \
								  \
	__asm__ __volatile__( ".arch_extension sec\n"             \
				"dsb\n"                           \
				"smc #1\n"                        \
				: "=r" (v0), "=r" (v1)            \
				: "r" (v0), "r" (code)            \
				: "r2", "r3", "memory"            \
				);                                \
	return (uint64_t)((((uint64_t)v1)<<32) + ((uint64_t)v0)); \
}

#define armor_smc_call1v(type, name, type1, arg1)                       \
static inline type armor_smc_##name(type1 arg1)                         \
{									\
	register uint32_t v0 asm("r0") = (uint32_t)arg1;		\
	register uint32_t code asm("ip") = ARMOR_SMC_ ##name;           \
									\
	__asm__ __volatile__( ".arch_extension sec\n"                   \
				"dsb\n"                                 \
				"smc #1\n"                              \
			     :                                          \
			     : "r" (v0), "r" (code)                     \
			     : "r1", "r2", "r3", "memory"               \
			     );                                         \
}

#define armor_smc_call1(type, name, type1, arg1)                        \
static inline type armor_smc_##name(type1 arg1)                         \
{									\
	register uint32_t v0 asm("r0") = (uint32_t)arg1;		\
	register uint32_t v1 asm("r1");					\
	register uint32_t code asm("ip") = ARMOR_SMC_ ##name;           \
									\
	__asm__ __volatile__( ".arch_extension sec\n"                   \
				"dsb\n"                                 \
				"smc #1\n"                              \
			     : "=r" (v0), "=r" (v1)                     \
			     : "r" (v0), "r" (code)                     \
			     : "r2", "r3", "memory"                     \
			     );                                         \
									\
	return (type)((((uint64_t)v1) << 32) + (uint64_t)v0);           \
}

#define armor_smc_call2v(type, name, type1, arg1, type2, arg2)          \
static inline type armor_smc_##name(type1 arg1, type2 arg2)             \
{									\
	register uint32_t v0 asm("r0") = (uint32_t)arg1;		\
	register uint32_t a1 asm("r1") = (uint32_t)arg2;		\
	register uint32_t code asm("ip") = ARMOR_SMC_ ##name;           \
									\
	__asm__ __volatile__( ".arch_extension sec\n"                   \
				"dsb\n"                                 \
				"smc #1\n"                              \
			     :                                          \
			     : "r" (v0), "r" (a1), "r" (code)           \
			     : "r2", "r3", "memory"                     \
			     );                                         \
}

#define armor_smc_call2(type, name, type1, arg1, type2, arg2)           \
static inline type armor_smc_##name(type1 arg1, type2 arg2)             \
{									\
	register uint32_t v0 asm("r0") = (uint32_t)arg1;		\
	register uint32_t a1 asm("r1") = (uint32_t)arg2;		\
	register uint32_t code asm("ip") = ARMOR_SMC_ ##name;           \
									\
	__asm__ __volatile__( ".arch_extension sec\n"                   \
				"dsb\n"                                 \
				"smc #1\n"                              \
			     : "=r" (v0), "=r" (a1)                     \
			     : "r" (v0), "r" (a1), "r" (code)           \
			     : "r2", "r3", "memory"                     \
			     );                                         \
									\
	return (type)((((uint64_t)a1) << 32) + (uint64_t)v0);           \
}

#define armor_smc_call3(type, name, type1, arg1, type2, arg2, type3, arg3) \
static inline type armor_smc_##name(type1 arg1, type2 arg2, type3 arg3) \
{									\
	register uint32_t v0 asm("r0") = (uint32_t)arg1;		\
	register uint32_t a1 asm("r1") = (uint32_t)arg2;		\
	register uint32_t a2 asm("r2") = (uint32_t)arg3;		\
	register uint32_t code asm("ip") = ARMOR_SMC_ ##name;           \
									\
	__asm__ __volatile__( ".arch_extension sec\n"                   \
				"dsb\n"                                 \
				"smc #1\n"                              \
			     : "=r" (v0), "=r" (a1)                     \
			     : "r" (v0), "r" (a1), "r" (a2), "r" (code) \
			     : "r3", "memory"                           \
			     );                                         \
									\
	return (type)((((uint64_t)a1) << 32) + (uint64_t)v0);           \
}

#define armor_smc_call4(type, name, type1, arg1, type2, arg2, type3, arg3, type4, arg4)	\
static inline type armor_smc_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4)	\
{									\
	register uint32_t v0 asm("r0") = (uint32_t)arg1;		\
	register uint32_t a1 asm("r1") = (uint32_t)arg2;		\
	register uint32_t a2 asm("r2") = (uint32_t)arg3;		\
	register uint32_t a3 asm("r3") = (uint32_t)arg4;		\
	register uint32_t code asm("ip") = ARMOR_SMC_ ##name;           \
									\
	__asm__ __volatile__( ".arch_extension sec\n"                   \
				"dsb\n"                                 \
				"smc #1\n"                              \
			     : "=r" (v0), "=r" (a1)                     \
			     : "r" (v0), "r" (a1), "r" (a2), "r" (a3), "r" (code) \
			     : "memory"                                 \
			     );                                         \
									\
	return (type)((((uint64_t)a1) << 32) + (uint64_t)v0);           \
}

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

#endif /*__ASSEMBLY__*/

#endif /*__ARMOR_SMC_H__*/
