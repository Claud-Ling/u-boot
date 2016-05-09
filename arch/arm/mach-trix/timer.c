
#include <common.h>
#include <asm/io.h>

struct pritimer {
	u32 tldr;	/* load*/
	u32 tcrr;	/* counter*/
	u32 tclr;	/* control*/
	u32 tisr;	/* interrupt status*/
};
//#define TIMER_CLOCK		(200 << 20)
#define TIMER_CLOCK		(200 * 1000 * 1000)

#define TIMER_OVERFLOW_VAL	0xffffffff
#define TIMER_LOAD_VAL		0xffffffff

/* GP Timer */
#define TCLR_ST			(0x3 << 0)

DECLARE_GLOBAL_DATA_PTR;

static struct pritimer *timer_base = (struct pritimer *)CONFIG_SYS_TIMERBASE;

int timer_init(void)
{
	/* start the counter ticking up, reload value on overflow */
	writel(TIMER_LOAD_VAL, &timer_base->tldr);

	/* enable timer */
	writel(TCLR_ST, &timer_base->tclr);

	/* reset time, capture current incrementer value time */
	gd->arch.lastinc = readl(&timer_base->tcrr) / (TIMER_CLOCK / CONFIG_SYS_HZ);
	gd->arch.tbl = 0;		/* start "advancing" time stamp from 0 */

	return 0;
}

/*
 * timer without interrupts
 */
ulong get_timer(ulong base)
{
	return get_timer_masked() - base;
}

/* delay x useconds */
void __udelay(unsigned long usec)
{
	long tmo = usec * (TIMER_CLOCK / 1000) / 1000;
	unsigned long now, last = readl(&timer_base->tcrr);

	while (tmo > 0) {
		now = readl(&timer_base->tcrr);
#if 0
		if (last > now) /* count up timer overflow */
			tmo -= TIMER_OVERFLOW_VAL - last + now + 1;
		else
			tmo -= now - last;
#else
		if (last >= now) /* count up timer overflow */
			tmo -= last - now ;
		else
			tmo -= TIMER_OVERFLOW_VAL + last - now;
#endif
		last = now;
	}
}

ulong get_timer_masked(void)
{
	/* current tick value */
	ulong now = readl(&timer_base->tcrr) / (TIMER_CLOCK / CONFIG_SYS_HZ);

	//if (now >= gd->lastinc)	/* normal mode (non roll) */
	if (now <= gd->arch.lastinc)	/* normal mode (non roll) */
	{
		/* move stamp fordward with absoulte diff ticks */
		gd->arch.tbl +=  gd->arch.lastinc - now;
	}
	else	/* we have rollover of incrementer */
	{
		gd->arch.tbl += ((TIMER_LOAD_VAL / (TIMER_CLOCK / CONFIG_SYS_HZ))) +gd->arch.lastinc - now;
	}
	gd->arch.lastinc = now;
	return gd->arch.tbl;
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On ARM it just returns the timer value.
 */
unsigned long long get_ticks(void)
{
	return get_timer(0);
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	return CONFIG_SYS_HZ;
}
