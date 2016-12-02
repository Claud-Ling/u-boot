#ifndef __ARMOR_SMC_H__
#define __ARMOR_SMC_H__

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


#endif /*__ASSEMBLY__*/

#endif /*__ARMOR_SMC_H__*/
