#ifndef __ARMOR_SMC_H__
#define __ARMOR_SMC_H__

#ifdef __ASSEMBLY__

	/*
	 * Helper macro to generate the best mov/movk combinations according
	 * the value to be moved. The 16 bits from '_shift' are tested and
	 * if not zero, they are moved into '_reg' without affecting
	 * other bits.
	 */
	.macro _mov_imm16 _reg, _val, _shift
		.if (\_val >> \_shift) & 0xffff
			.if (\_val & (1 << \_shift - 1))
				movk	\_reg, (\_val >> \_shift) & 0xffff, LSL \_shift
			.else
				mov	\_reg, \_val & (0xffff << \_shift)
			.endif
		.endif
	.endm

	/*
	 * Helper macro to load arbitrary values into 32 or 64-bit registers
	 * which generates the best mov/movk combinations. Many base addresses
	 * are 64KB aligned the macro will eliminate updating bits 15:0 in
	 * that case
	 */
	.macro mov_imm _reg, _val
		.if (\_val) == 0
			mov	\_reg, #0
		.else
			_mov_imm16	\_reg, (\_val), 0
			_mov_imm16	\_reg, (\_val), 16
			_mov_imm16	\_reg, (\_val), 32
			_mov_imm16	\_reg, (\_val), 48
		.endif
	.endm


#define armor_smc(call_name, val)            \
	mov_imm	x12, =ARMOR_SMC_ ##call_name;\
	ldr	x0, =val;                    \
	dsb	#1;                          \
	smc	#1;

#else /*__ASSEMBLY__*/

#define armor_smc_call0v(type, name)                              \
static inline type armor_smc_##name(void)                         \
{                                                                 \
	register uint32_t code asm("w12") = ARMOR_SMC_ ##name;     \
								  \
	__asm__ __volatile__( 					  \
				"dsb #1\n"                        \
				"smc #1\n"                        \
				:                                 \
				: "r" (code)                      \
				: "r0", "r1", "r2", "r3", "memory"\
				);                                \
}

#define armor_smc_call0(type, name)                               \
static inline type armor_smc_##name(void)                         \
{                                                                 \
	register uint32_t code asm("w12") = ARMOR_SMC_ ##name;     \
	register uint32_t v0 asm("w0");                           \
	register uint32_t v1 asm("w1");                           \
								  \
	__asm__ __volatile__(				             \
				"dsb #1\n"                           \
				"smc #1\n"                        \
				: "=r" (v0), "=r" (v1)            \
				: "r" (v0), "r" (code)            \
				: "x2", "x3", "memory"            \
				);                                \
	return (type)((((uint64_t)v1) << 32) + (uint64_t)v0);     \
}

/* can't use RMuint64from2RMuint32 below, cpp doesn't seem to replace it
 * inside the other macro definition... */
#define uint64_armor_smc_call0(name)                              \
static inline uint64_t armor_smc_##name(void)                     \
{                                                                 \
	register uint32_t code asm("w12") = ARMOR_SMC_ ##name;     \
	register uint32_t v0 asm("w0");                           \
	register uint32_t v1 asm("w1");                           \
								  \
	__asm__ __volatile__( 					  \
				"dsb #1\n"                           \
				"smc #1\n"                        \
				: "=r" (v0), "=r" (v1)            \
				: "r" (v0), "r" (code)            \
				: "x2", "x3", "memory"            \
				);                                \
	return (uint64_t)((((uint64_t)v1)<<32) + ((uint64_t)v0)); \
}

#define armor_smc_call1v(type, name, type1, arg1)                       \
static inline type armor_smc_##name(type1 arg1)                         \
{									\
	register uint32_t v0 asm("w0") = (uint32_t)arg1;		\
	register uint32_t code asm("w12") = ARMOR_SMC_ ##name;           \
									\
	__asm__ __volatile__( 						\
				"dsb #1\n"                                 \
				"smc #1\n"                              \
			     :                                          \
			     : "r" (v0), "r" (code)                     \
			     : "x1", "x2", "x3", "memory"               \
			     );                                         \
}

#define armor_smc_call1(type, name, type1, arg1)                        \
static inline type armor_smc_##name(type1 arg1)                         \
{									\
	register uint32_t v0 asm("w0") = (uint32_t)arg1;		\
	register uint32_t v1 asm("w1");					\
	register uint32_t code asm("w12") = ARMOR_SMC_ ##name;           \
									\
	__asm__ __volatile__( 						\
				"dsb #1\n"                                 \
				"smc #1\n"                              \
			     : "=r" (v0), "=r" (v1)                     \
			     : "r" (v0), "r" (code)                     \
			     : "x2", "x3", "memory"                     \
			     );                                         \
									\
	return (type)((((uint64_t)v1) << 32) + (uint64_t)v0);           \
}

#define armor_smc_call2v(type, name, type1, arg1, type2, arg2)          \
static inline type armor_smc_##name(type1 arg1, type2 arg2)             \
{									\
	register uint32_t v0 asm("w0") = (uint32_t)arg1;		\
	register uint32_t a1 asm("w1") = (uint32_t)arg2;		\
	register uint32_t code asm("w12") = ARMOR_SMC_ ##name;           \
									\
	__asm__ __volatile__( 						\
				"dsb #1\n"                                 \
				"smc #1\n"                              \
			     :                                          \
			     : "r" (v0), "r" (a1), "r" (code)           \
			     : "x2", "x3", "memory"                     \
			     );                                         \
}

#define armor_smc_call2(type, name, type1, arg1, type2, arg2)           \
static inline type armor_smc_##name(type1 arg1, type2 arg2)             \
{									\
	register uint32_t v0 asm("w0") = (uint32_t)arg1;		\
	register uint32_t a1 asm("w1") = (uint32_t)arg2;		\
	register uint32_t code asm("w12") = ARMOR_SMC_ ##name;           \
									\
	__asm__ __volatile__( 						\
				"dsb #1\n"                                 \
				"smc #1\n"                              \
			     : "=r" (v0), "=r" (a1)                     \
			     : "r" (v0), "r" (a1), "r" (code)           \
			     : "x2", "x3", "memory"                     \
			     );                                         \
									\
	return (type)((((uint64_t)a1) << 32) + (uint64_t)v0);           \
}

#define armor_smc_call3(type, name, type1, arg1, type2, arg2, type3, arg3) \
static inline type armor_smc_##name(type1 arg1, type2 arg2, type3 arg3) \
{									\
	register uint32_t v0 asm("w0") = (uint32_t)arg1;		\
	register uint32_t a1 asm("w1") = (uint32_t)arg2;		\
	register uint32_t a2 asm("w2") = (uint32_t)arg3;		\
	register uint32_t code asm("w12") = ARMOR_SMC_ ##name;           \
									\
	__asm__ __volatile__( 						\
				"dsb #1\n"                                 \
				"smc #1\n"                              \
			     : "=r" (v0), "=r" (a1)                     \
			     : "r" (v0), "r" (a1), "r" (a2), "r" (code) \
			     : "x3", "memory"                           \
			     );                                         \
									\
	return (type)((((uint64_t)a1) << 32) + (uint64_t)v0);           \
}

#define armor_smc_call4(type, name, type1, arg1, type2, arg2, type3, arg3, type4, arg4)	\
static inline type armor_smc_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4)	\
{									\
	register uint32_t v0 asm("w0") = (uint32_t)arg1;		\
	register uint32_t a1 asm("w1") = (uint32_t)arg2;		\
	register uint32_t a2 asm("w2") = (uint32_t)arg3;		\
	register uint32_t a3 asm("w3") = (uint32_t)arg4;		\
	register uint32_t code asm("w12") = ARMOR_SMC_ ##name;           \
									\
	__asm__ __volatile__( 						\
				"dsb #1\n"                                 \
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
