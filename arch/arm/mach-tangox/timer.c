#include <common.h>
#include <asm/io.h>
#include <asm/arch/emhwlib_registers_tango4.h>

#define TIMER_LOAD_VAL          0xFFFF
#define TIMER_OVERFLOW_VAL      TIMER_LOAD_VAL
#define CONFIG_SYS_27MHZ        27
#define TMR_PRESCALE16_TICK     32


DECLARE_GLOBAL_DATA_PTR;

int timer_init(void)
{
    //TODO: init code goes here
#if !defined(CONFIG_RTSM)
    /* start timer and reload upon overflow */
    writew( TIMER_LOAD_VAL, REG_BASE_cpu_block + CPU_time1_load );

    /* enable timer - pre-scaler set to 16 */
    writew( 0xc4, REG_BASE_cpu_block + CPU_time1_ctrl );
    
    /* reset time */
    gd->arch.lastinc = readw(REG_BASE_cpu_block + CPU_time1_value);

    gd->arch.tbl = 0;
#endif

    return 0;
}

unsigned long get_timer(unsigned long base)
{
    //TODO: implement this function
#if !defined(CONFIG_RTSM)
    if( base == 0 ) {
        gd->arch.lastinc = readw(REG_BASE_cpu_block + CPU_time1_value);
    }

    return get_timer_masked() - base;
#else
    return 0;
#endif
}

void __udelay(unsigned long usec)
{
#if !defined(CONFIG_RTSM)
	long tmo = usec * CONFIG_SYS_27MHZ / TMR_PRESCALE16_TICK;
    unsigned long now, last = readw( REG_BASE_cpu_block + CPU_time1_value );

    while (tmo > 0) {
        now = readw( REG_BASE_cpu_block + CPU_time1_value );
    
        if (last < now ) /* cont timer lap */
            tmo -= (last + (TIMER_OVERFLOW_VAL-now));
        else
            tmo -= (last - now);
        last = now;
    }
#endif
}

ulong get_timer_masked(void)
{
#if !defined(CONFIG_RTSM)
	/* current tick value */
	ulong now = readw( REG_BASE_cpu_block + CPU_time1_value );

    if (now < gd->arch.lastinc) /* normal mode (non roll) */
        /* move stamp forward with absolute diff ticks */
        gd->arch.tbl += (gd->arch.lastinc - now) * TMR_PRESCALE16_TICK / CONFIG_SYS_27MHZ;
    else    /* we have rollover of incrementer */
        gd->arch.tbl += ( gd->arch.lastinc + (TIMER_OVERFLOW_VAL-now)) * TMR_PRESCALE16_TICK / CONFIG_SYS_27MHZ;

    gd->arch.lastinc = now;

    return gd->arch.tbl / 1000;
#else
    return 0;
#endif
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


