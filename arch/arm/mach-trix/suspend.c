#include <common.h>

extern int __cpu_suspend(unsigned long, int (*)(unsigned long));
extern void cpu_v7_do_resume(void *sp);
extern int cpu_v7_do_suspend(void *sp);

#define cpu_do_resume cpu_v7_do_resume
#define cpu_do_suspend cpu_v7_do_suspend

/*
 * This is called by __cpu_suspend() to save the state, and do whatever
 * flushing is required to ensure that when the CPU goes to sleep we have
 * the necessary data available when the caches are not searched.
 */
void __cpu_suspend_save(u32 *ptr, u32 ptrsz, u32 sp, u32 *save_ptr)
{
	u32 *ctx = ptr;

	*save_ptr = (u32)ptr;

	/* This must correspond to the LDM in cpu_resume() assembly */
	*ptr++ = 0;	/*N/A*/
	*ptr++ = sp;
	*ptr++ = (u32)cpu_do_resume;

	cpu_do_suspend(ptr);

	flush_cache((u32)save_ptr, sizeof(*save_ptr));
	flush_cache((u32)ctx, ptrsz);
}

/*
 * Hide the first two arguments to __cpu_suspend - these are an implementation
 * detail which platform code shouldn't have to know about.
 */
int cpu_suspend(unsigned long arg, int (*fn)(unsigned long))
{
	int ret = __cpu_suspend(arg, fn);
	return ret;
}
