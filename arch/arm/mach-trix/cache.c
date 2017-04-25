#include <common.h>
#include <asm/arch/setup.h>

#include <asm/io.h>
#include <linux/compiler.h>


#define L2X0_CACHE_ID			0x000
#define L2X0_CACHE_TYPE			0x004
#define L2X0_CTRL			0x100
#define L2X0_AUX_CTRL			0x104
#define L2X0_TAG_LATENCY_CTRL		0x108
#define L2X0_DATA_LATENCY_CTRL		0x10C
#define L2X0_EVENT_CNT_CTRL		0x200
#define L2X0_EVENT_CNT1_CFG		0x204
#define L2X0_EVENT_CNT0_CFG		0x208
#define L2X0_EVENT_CNT1_VAL		0x20C
#define L2X0_EVENT_CNT0_VAL		0x210
#define L2X0_INTR_MASK			0x214
#define L2X0_MASKED_INTR_STAT		0x218
#define L2X0_RAW_INTR_STAT		0x21C
#define L2X0_INTR_CLEAR			0x220
#define L2X0_CACHE_SYNC			0x730
#define L2X0_DUMMY_REG			0x740
#define L2X0_INV_LINE_PA		0x770
#define L2X0_INV_WAY			0x77C
#define L2X0_CLEAN_LINE_PA		0x7B0
#define L2X0_CLEAN_LINE_IDX		0x7B8
#define L2X0_CLEAN_WAY			0x7BC
#define L2X0_CLEAN_INV_LINE_PA		0x7F0
#define L2X0_CLEAN_INV_LINE_IDX		0x7F8
#define L2X0_CLEAN_INV_WAY		0x7FC
/*
 * The lockdown registers repeat 8 times for L310, the L210 has only one
 * D and one I lockdown register at 0x0900 and 0x0904.
 */
#define L2X0_LOCKDOWN_WAY_D_BASE	0x900
#define L2X0_LOCKDOWN_WAY_I_BASE	0x904
#define L2X0_LOCKDOWN_STRIDE		0x08
#define L2X0_ADDR_FILTER_START		0xC00
#define L2X0_ADDR_FILTER_END		0xC04
#define L2X0_TEST_OPERATION		0xF00
#define L2X0_LINE_DATA			0xF10
#define L2X0_LINE_TAG			0xF30
#define L2X0_DEBUG_CTRL			0xF40
#define L2X0_PREFETCH_CTRL		0xF60
#define L2X0_POWER_CTRL			0xF80
#define   L2X0_DYNAMIC_CLK_GATING_EN	(1 << 1)
#define   L2X0_STNDBY_MODE_EN		(1 << 0)

/* Registers shifts and masks */
#define L2X0_CACHE_ID_REV_MASK		(0x3f)
#define L2X0_CACHE_ID_PART_MASK		(0xf << 6)
#define L2X0_CACHE_ID_PART_L210		(1 << 6)
#define L2X0_CACHE_ID_PART_L310		(3 << 6)
#define L2X0_CACHE_ID_RTL_MASK          0x3f
#define L2X0_CACHE_ID_RTL_R0P0          0x0
#define L2X0_CACHE_ID_RTL_R1P0          0x2
#define L2X0_CACHE_ID_RTL_R2P0          0x4
#define L2X0_CACHE_ID_RTL_R3P0          0x5
#define L2X0_CACHE_ID_RTL_R3P1          0x6
#define L2X0_CACHE_ID_RTL_R3P2          0x8

#define L2X0_AUX_CTRL_MASK			0xc0000fff
#define L2X0_AUX_CTRL_DATA_RD_LATENCY_SHIFT	0
#define L2X0_AUX_CTRL_DATA_RD_LATENCY_MASK	0x7
#define L2X0_AUX_CTRL_DATA_WR_LATENCY_SHIFT	3
#define L2X0_AUX_CTRL_DATA_WR_LATENCY_MASK	(0x7 << 3)
#define L2X0_AUX_CTRL_TAG_LATENCY_SHIFT		6
#define L2X0_AUX_CTRL_TAG_LATENCY_MASK		(0x7 << 6)
#define L2X0_AUX_CTRL_DIRTY_LATENCY_SHIFT	9
#define L2X0_AUX_CTRL_DIRTY_LATENCY_MASK	(0x7 << 9)
#define L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT	16
#define L2X0_AUX_CTRL_WAY_SIZE_SHIFT		17
#define L2X0_AUX_CTRL_WAY_SIZE_MASK		(0x7 << 17)
#define L2X0_AUX_CTRL_SHARE_OVERRIDE_SHIFT	22
#define L2X0_AUX_CTRL_NS_LOCKDOWN_SHIFT		26
#define L2X0_AUX_CTRL_NS_INT_CTRL_SHIFT		27
#define L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT	28
#define L2X0_AUX_CTRL_INSTR_PREFETCH_SHIFT	29
#define L2X0_AUX_CTRL_EARLY_BRESP_SHIFT		30

#define L2X0_LATENCY_CTRL_SETUP_SHIFT	0
#define L2X0_LATENCY_CTRL_RD_SHIFT	4
#define L2X0_LATENCY_CTRL_WR_SHIFT	8

#define L2X0_ADDR_FILTER_EN		1

#define REV_PL310_R2P0				4

#define L2X0_REG_BASE		0xf2102000
#define CACHE_LINE_SIZE		32


#define CONFIG_ANDROID_PLATFORM
#define CONFIG_CACHE_PL310
//#define CONFIG_PL310_ERRATA_727915

#define readl_relaxed(reg) readl(reg)
#define writel_relaxed(val, reg) writel(val, reg)
#define cpu_relax() barrier()
#define l2cache_base ((void*)L2X0_REG_BASE)
#ifdef __iomem
#undef __iomem
#endif
#define __iomem
#define SZ_1K 0x00000400

#ifndef CONFIG_ARM64
# include <asm/armv7.h>
# define dsb() CP15DSB
#else
# define dsb()	\
	__asm__ volatile("dsb sy\n" : : : "memory")
#endif

#ifndef CONFIG_SYS_L2CACHE_OFF
/**************************************************************************/
/* sx6 l2x0 control wrapper						  */
/**************************************************************************/
#ifdef CONFIG_DTV_SMC
static void sx6_smc_l2x0_enable(void)
{
	/* Disable PL310 L2 Cache controller */
	if ( !secure_get_security_state() )
		secure_l2x0_set_reg(l2cache_base, L2X0_CTRL, 0x1);
	else
		writel_relaxed(1, l2cache_base + L2X0_CTRL);
}
static void sx6_smc_l2x0_disable(void)
{
	/* Disable PL310 L2 Cache controller */
	if ( !secure_get_security_state() )
		secure_l2x0_set_reg(l2cache_base, L2X0_CTRL, 0x0);
	else
		writel_relaxed(0, l2cache_base + L2X0_CTRL);
}

static void sx6_smc_l2x0_set_auxctrl(unsigned long val)
{
	/* Set L2 Cache auxiliary contry register */
	if ( !secure_get_security_state() )
		secure_l2x0_set_reg(l2cache_base, L2X0_AUX_CTRL, (uint32_t)val);
	else
		writel_relaxed(val, l2cache_base + L2X0_AUX_CTRL);
}

static void sx6_smc_l2x0_set_debug(unsigned long val)
{
	/* Program PL310 L2 Cache controller debug register */
	if ( !secure_get_security_state() )
		secure_l2x0_set_reg(l2cache_base, L2X0_DEBUG_CTRL, (uint32_t)val);
	else
		writel_relaxed(val, l2cache_base + L2X0_DEBUG_CTRL);
}

# define sigma_sx6_l2x0_enable() sx6_smc_l2x0_enable()
# define sigma_sx6_l2x0_disable() sx6_smc_l2x0_disable()
# define sigma_sx6_l2x0_set_auxctrl(val) sx6_smc_l2x0_set_auxctrl(val)
# define sigma_sx6_l2x0_set_debug(val) sx6_smc_l2x0_set_debug(val)

#else /*CONFIG_DTV_SMC*/

# define sigma_sx6_l2x0_enable() writel_relaxed(1, l2cache_base + L2X0_CTRL)
# define sigma_sx6_l2x0_disable() writel_relaxed(0, l2cache_base + L2X0_CTRL)
# define sigma_sx6_l2x0_set_auxctrl(val) writel_relaxed(val, l2cache_base + L2X0_AUX_CTRL);
# define sigma_sx6_l2x0_set_debug(val) writel_relaxed(val, l2cache_base + L2X0_DEBUG_CTRL);

#endif /*CONFIG_DTV_SMC*/

/**************************************************************************/
/* l2x0 control interfaces						  */
/**************************************************************************/

static u32 l2x0_way_mask;	/* Bitmask of active ways */
static u32 l2x0_size;
#ifdef CONFIG_ANDROID_PLATFORM
static u32 l2x0_cache_id;
static unsigned int l2x0_sets;
static unsigned int l2x0_ways;
#endif
static unsigned long sync_reg_offset = L2X0_CACHE_SYNC;

struct l2x0_regs {
	unsigned long phy_base;
	unsigned long aux_ctrl;
	/*
	 * Whether the following registers need to be saved/restored
	 * depends on platform
	 */
	unsigned long tag_latency;
	unsigned long data_latency;
	unsigned long filter_start;
	unsigned long filter_end;
	unsigned long prefetch_ctrl;
	unsigned long pwr_ctrl;
};
static struct l2x0_regs sx6_l2x0_saved_regs;

struct outer_cache_fns {
	void (*inv_range)(unsigned long, unsigned long);
	void (*clean_range)(unsigned long, unsigned long);
	void (*flush_range)(unsigned long, unsigned long);
	void (*flush_all)(void);
	void (*inv_all)(void);
	void (*disable)(void);
	void (*sync)(void);
	void (*set_debug)(unsigned long);
	void (*resume)(void);
};

static struct outer_cache_fns outer_cache;

#ifdef CONFIG_ANDROID_PLATFORM
static inline int is_pl310_rev(int rev)
{
	return (l2x0_cache_id &
		(L2X0_CACHE_ID_PART_MASK | L2X0_CACHE_ID_REV_MASK)) ==
			(L2X0_CACHE_ID_PART_L310 | rev);
}
#endif

static inline void cache_wait_way(void __iomem *reg, unsigned long mask)
{
	/* wait for cache operation by line or way to complete */
	while (readl_relaxed(reg) & mask)
		cpu_relax();
}

#ifdef CONFIG_CACHE_PL310
static inline void cache_wait(void __iomem *reg, unsigned long mask)
{
	/* cache operations by line are atomic on PL310 */
}
#else
#define cache_wait	cache_wait_way
#endif

static inline void cache_sync(void)
{
	void __iomem *base = l2cache_base;

	writel_relaxed(0, base + sync_reg_offset);
	cache_wait(base + L2X0_CACHE_SYNC, 1);
}

static inline void l2x0_clean_line(unsigned long addr)
{
	void __iomem *base = l2cache_base;
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_CLEAN_LINE_PA);
}

static inline void l2x0_inv_line(unsigned long addr)
{
	void __iomem *base = l2cache_base;
	cache_wait(base + L2X0_INV_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_INV_LINE_PA);
}

#if defined(CONFIG_PL310_ERRATA_588369) || defined(CONFIG_PL310_ERRATA_727915)
static inline void debug_writel(unsigned long val)
{
	if (outer_cache.set_debug)
		outer_cache.set_debug(val);
}

static void pl310_set_debug(unsigned long val)
{
	sigma_sx6_l2x0_set_debug(val);
}
#else
/* Optimised out for non-errata case */
static inline void debug_writel(unsigned long val)
{
}

#define pl310_set_debug	NULL
#endif

#ifdef CONFIG_PL310_ERRATA_588369
static inline void l2x0_flush_line(unsigned long addr)
{
	void __iomem *base = l2cache_base;

	/* Clean by PA followed by Invalidate by PA */
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_CLEAN_LINE_PA);
	cache_wait(base + L2X0_INV_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_INV_LINE_PA);
}
#else

static inline void l2x0_flush_line(unsigned long addr)
{
	void __iomem *base = l2cache_base;
	cache_wait(base + L2X0_CLEAN_INV_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_CLEAN_INV_LINE_PA);
}
#endif

static void l2x0_cache_sync(void)
{
	cache_sync();
}

#if defined(CONFIG_ANDROID_PLATFORM) && defined(CONFIG_PL310_ERRATA_727915)
static void l2x0_for_each_set_way(void __iomem *reg)
{
	int set;
	int way;

	for (way = 0; way < l2x0_ways; way++) {
		for (set = 0; set < l2x0_sets; set++)
			writel_relaxed((way << 28) | (set << 5), reg);
		cache_sync();
	}
}
#endif

static void __l2x0_flush_all(void)
{
	void __iomem *base = l2cache_base;
	debug_writel(0x03);
	writel_relaxed(l2x0_way_mask, base + L2X0_CLEAN_INV_WAY);
	cache_wait_way(base + L2X0_CLEAN_INV_WAY, l2x0_way_mask);
	cache_sync();
	debug_writel(0x00);
}

static void l2x0_flush_all(void)
{
	void __iomem *base = l2cache_base;

#if defined(CONFIG_ANDROID_PLATFORM) && defined(CONFIG_PL310_ERRATA_727915)
	if (is_pl310_rev(REV_PL310_R2P0)) {
		l2x0_for_each_set_way(base + L2X0_CLEAN_INV_LINE_IDX);
		return;
	}
#endif

	/* clean all ways */
	__l2x0_flush_all();
}

static void l2x0_clean_all(void)
{
	void __iomem *base = l2cache_base;

#if defined(CONFIG_ANDROID_PLATFORM) && defined(CONFIG_PL310_ERRATA_727915)
	if (is_pl310_rev(REV_PL310_R2P0)) {
		l2x0_for_each_set_way(base + L2X0_CLEAN_LINE_IDX);
		return;
	}
#endif

	/* clean all ways */
#ifdef CONFIG_ANDROID_PLATFORM
	debug_writel(0x03);
#endif
	writel_relaxed(l2x0_way_mask, base + L2X0_CLEAN_WAY);
	cache_wait_way(base + L2X0_CLEAN_WAY, l2x0_way_mask);
	cache_sync();
#ifdef CONFIG_ANDROID_PLATFORM
	debug_writel(0x00);
#endif
}

static void l2x0_inv_all(void)
{
	void __iomem *base = l2cache_base;

	/* invalidate all ways */
	/* Invalidating when L2 is enabled is a nono */
	//BUG_ON(readl(base + L2X0_CTRL) & 1);
	writel_relaxed(l2x0_way_mask, base + L2X0_INV_WAY);
	cache_wait_way(base + L2X0_INV_WAY, l2x0_way_mask);
	cache_sync();
}

static void l2x0_inv_range(unsigned long start, unsigned long end)
{
	void __iomem *base = l2cache_base;

	if (start & (CACHE_LINE_SIZE - 1)) {
		start &= ~(CACHE_LINE_SIZE - 1);
		debug_writel(0x03);
		l2x0_flush_line(start);
		debug_writel(0x00);
		start += CACHE_LINE_SIZE;
	}

	if (end & (CACHE_LINE_SIZE - 1)) {
		end &= ~(CACHE_LINE_SIZE - 1);
		debug_writel(0x03);
		l2x0_flush_line(end);
		debug_writel(0x00);
	}

	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		while (start < blk_end) {
			l2x0_inv_line(start);
			start += CACHE_LINE_SIZE;
		}

		if (blk_end < end) {
			//raw_spin_unlock_irqrestore(&l2x0_lock, flags);
			//raw_spin_lock_irqsave(&l2x0_lock, flags);
		}
	}
	cache_wait(base + L2X0_INV_LINE_PA, 1);
	cache_sync();
}

static void l2x0_clean_range(unsigned long start, unsigned long end)
{
	void __iomem *base = l2cache_base;

	if ((end - start) >= l2x0_size) {
		l2x0_clean_all();
		return;
	}

	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		while (start < blk_end) {
			l2x0_clean_line(start);
			start += CACHE_LINE_SIZE;
		}

		if (blk_end < end) {
			//raw_spin_unlock_irqrestore(&l2x0_lock, flags);
			//raw_spin_lock_irqsave(&l2x0_lock, flags);
		}
	}
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	cache_sync();
}

static void l2x0_flush_range(unsigned long start, unsigned long end)
{
	void __iomem *base = l2cache_base;

	if ((end - start) >= l2x0_size) {
		l2x0_flush_all();
		return;
	}

	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		debug_writel(0x03);
		while (start < blk_end) {
			l2x0_flush_line(start);
			start += CACHE_LINE_SIZE;
		}
		debug_writel(0x00);

		if (blk_end < end) {
			//raw_spin_unlock_irqrestore(&l2x0_lock, flags);
			//raw_spin_lock_irqsave(&l2x0_lock, flags);
		}
	}
	cache_wait(base + L2X0_CLEAN_INV_LINE_PA, 1);
	cache_sync();
}

static void l2x0_disable(void)
{
	debug("disable l2 cache\n");
	__l2x0_flush_all();
	sigma_sx6_l2x0_disable();
	dsb();
}

static void l2x0_unlock(u32 cache_id)
{
	void __iomem *base = l2cache_base;
	int lockregs;
	int i;

	if (cache_id == L2X0_CACHE_ID_PART_L310)
		lockregs = 8;
	else
		/* L210 and unknown types */
		lockregs = 1;

	for (i = 0; i < lockregs; i++) {
		writel_relaxed(0x0, base + L2X0_LOCKDOWN_WAY_D_BASE +
			       i * L2X0_LOCKDOWN_STRIDE);
		writel_relaxed(0x0, base + L2X0_LOCKDOWN_WAY_I_BASE +
			       i * L2X0_LOCKDOWN_STRIDE);
	}
}

static void sx6_l2x0_init(void)
{
	u32 aux, aux_val, aux_mask;
	u32 cache_id;
	u32 way_size = 0;
	int ways;
	const char *type;
	void __iomem *base = l2cache_base;

#ifdef CONFIG_SIGMA_SOC_SX6
	/*
	* SX6 L2CC: PL310@r3p2, 512K, 16ways, 32K per ways
	* Tag laterncy: 0, Data latency: 0
	*/

	aux_val = ((1 << L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT) |		//bit 16: 1 for 16 ways
			(0x2 << L2X0_AUX_CTRL_WAY_SIZE_SHIFT) |		//bit 19-17: 011b 64KB, 010b 32KB
			(1 << L2X0_AUX_CTRL_SHARE_OVERRIDE_SHIFT) |     //bit 22: 1 shared attribute internally ignored.
			(1 << 25) |					//bit 25: SBO/RAO
			(1 << L2X0_AUX_CTRL_NS_LOCKDOWN_SHIFT) |	//bit 26: 1 NS can write to the lockdown register
			(1 << L2X0_AUX_CTRL_NS_INT_CTRL_SHIFT) |	//bit 27: 1 Int Clear and Mask can be NS accessed
			(1 << L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT) |	//bit 28: 1 data prefetching enabled
			(1 << L2X0_AUX_CTRL_INSTR_PREFETCH_SHIFT) |	//bit 29: 1 instruction prefetch enabled
			(1 << L2X0_AUX_CTRL_EARLY_BRESP_SHIFT));	//bit 30: 1 early BRESP eanbled
#elif defined (CONFIG_SIGMA_SOC_SX7)
	/*
	* SX7 L2CC: PL310@r3p2, 1024K, 16ways, 64K per ways
	* Tag laterncy: 0, Data latency: 0
	*/

	aux_val = ((1 << L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT) |		//bit 16: 1 for 16 ways
			(0x3 << L2X0_AUX_CTRL_WAY_SIZE_SHIFT) |		//bit 19-17: 011b 64KB, 010b 32KB
			(1 << L2X0_AUX_CTRL_SHARE_OVERRIDE_SHIFT) |     //bit 22: 1 shared attribute internally ignored.
			(1 << 25) |					//bit 25: SBO/RAO
			(1 << L2X0_AUX_CTRL_NS_LOCKDOWN_SHIFT) |	//bit 26: 1 NS can write to the lockdown register
			(1 << L2X0_AUX_CTRL_NS_INT_CTRL_SHIFT) |	//bit 27: 1 Int Clear and Mask can be NS accessed
			(1 << L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT) |	//bit 28: 1 data prefetching enabled
			(1 << L2X0_AUX_CTRL_INSTR_PREFETCH_SHIFT) |	//bit 29: 1 instruction prefetch enabled
			(1 << L2X0_AUX_CTRL_EARLY_BRESP_SHIFT));	//bit 30: 1 early BRESP eanbled


#elif defined (CONFIG_SIGMA_SOC_SX8)
	/*
	* SX8 L2CC: PL310@r3p2, 1024K, 16ways, 64K per ways
	* Tag laterncy: 0, Data latency: 0
	*/
	aux_val = ((1 << L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT) |		//bit 16: 1 for 16 ways
			(0x3 << L2X0_AUX_CTRL_WAY_SIZE_SHIFT) |		//bit 19-17: 011b 64KB, 010b 32KB
			(1 << L2X0_AUX_CTRL_SHARE_OVERRIDE_SHIFT) |     //bit 22: 1 shared attribute internally ignored.
			(1 << 25) |					//bit 25: SBO/RAO
			(1 << L2X0_AUX_CTRL_NS_LOCKDOWN_SHIFT) |	//bit 26: 1 NS can write to the lockdown register
			(1 << L2X0_AUX_CTRL_NS_INT_CTRL_SHIFT) |	//bit 27: 1 Int Clear and Mask can be NS accessed
			(1 << L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT) |	//bit 28: 1 data prefetching enabled
			(1 << L2X0_AUX_CTRL_INSTR_PREFETCH_SHIFT) |	//bit 29: 1 instruction prefetch enabled
			(1 << L2X0_AUX_CTRL_EARLY_BRESP_SHIFT));	//bit 30: 1 early BRESP eanbled


#else
	#error "unknown chip!"
#endif

	aux_mask = L2X0_AUX_CTRL_MASK;

	cache_id = readl_relaxed(base + L2X0_CACHE_ID);
	aux = readl_relaxed(base + L2X0_AUX_CTRL);

	aux &= aux_mask;
	aux |= aux_val;

	/* Determine the number of ways */
	switch (cache_id & L2X0_CACHE_ID_PART_MASK) {
	case L2X0_CACHE_ID_PART_L310:
		if (aux & (1 << 16))
			ways = 16;
		else
			ways = 8;
		type = "L310";
#ifdef CONFIG_PL310_ERRATA_753970
		/* Unmapped register. */
		sync_reg_offset = L2X0_DUMMY_REG;
#endif
		outer_cache.set_debug = pl310_set_debug;
		break;
	case L2X0_CACHE_ID_PART_L210:
		ways = (aux >> 13) & 0xf;
		type = "L210";
		break;
	default:
		/* Assume unknown chips have 8 ways */
		ways = 8;
		type = "L2x0 series";
		break;
	}

	l2x0_way_mask = (1 << ways) - 1;

	/*
	 * L2 cache Size =  Way size * Number of ways
	 */
	way_size = (aux & L2X0_AUX_CTRL_WAY_SIZE_MASK) >> 17;
	way_size = 1 << (way_size + 3);
	l2x0_size = ways * way_size * SZ_1K;

#ifdef CONFIG_ANDROID_PLATFORM
	l2x0_cache_id = cache_id;
	l2x0_ways = ways;
	l2x0_sets = way_size * SZ_1K / CACHE_LINE_SIZE;
#endif
	/*
	 * Check if l2x0 controller is already enabled.
	 * If you are booting from non-secure mode
	 * accessing the below registers will fault.
	 */
	if (!(readl_relaxed(base + L2X0_CTRL) & 1)) {
		/* Make sure that I&D is not locked down when starting */
		//l2x0_unlock(cache_id);

		/* l2x0 controller is disabled */
		sigma_sx6_l2x0_set_auxctrl(aux);
		sx6_l2x0_saved_regs.aux_ctrl = aux;

		l2x0_inv_all();

		/* enable L2X0 */
		sigma_sx6_l2x0_enable();
	}

	outer_cache.inv_range = l2x0_inv_range;
	outer_cache.clean_range = l2x0_clean_range;
	outer_cache.flush_range = l2x0_flush_range;
	outer_cache.sync = l2x0_cache_sync;
	outer_cache.flush_all = l2x0_flush_all;
	outer_cache.inv_all = l2x0_inv_all;
	outer_cache.disable = l2x0_disable;

	debug("%s cache controller enabled\n", type);
	debug("l2x0: %d ways, CACHE_ID 0x%08x, AUX_CTRL 0x%08x, Cache size: %d B\n",
			ways, cache_id, aux, l2x0_size);

}

void v7_outer_cache_enable(void)
{
	sx6_l2x0_init();
}

void v7_outer_cache_disable(void)
{
	if (outer_cache.disable)
		outer_cache.disable();

	outer_cache.inv_range = NULL;
	outer_cache.clean_range = NULL;
	outer_cache.flush_range = NULL;
	outer_cache.sync = NULL;
	outer_cache.flush_all = NULL;
	outer_cache.inv_all = NULL;
	outer_cache.disable = NULL;
}

void v7_outer_cache_flush_all(void)
{
	if (outer_cache.flush_all)
		outer_cache.flush_all();
}

void v7_outer_cache_inval_all(void)
{
	if (outer_cache.inv_all)
		outer_cache.inv_all();
}

void v7_outer_cache_flush_range(u32 start, u32 end)
{
	if (outer_cache.clean_range)
		outer_cache.clean_range(start, end);
}

void v7_outer_cache_inval_range(u32 start, u32 end)
{
	if (outer_cache.inv_range)
		outer_cache.inv_range(start, end);
}
#elif defined(CONFIG_DTV_SMC) //ifndef CONFIG_SYS_L2CACHE_OFF
void v7_outer_cache_enable(void)
{
	if ( !secure_get_security_state() )
		secure_l2x0_set_reg(L2X0_CTRL, 0x0); //disable l2cache anyway
}
void v7_outer_cache_disable(void)
{
	if ( !secure_get_security_state() )
		secure_l2x0_set_reg(L2X0_CTRL, 0x0);
}

#endif //elif defined(CONFIG_DTV_SMC)

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	icache_enable();
	dcache_enable();
}
#endif
