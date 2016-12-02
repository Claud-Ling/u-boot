#ifndef __TRIX_PROC_OPS_H__
#define __TRIX_PROC_OPS_H__

#ifndef __ASSEMBLY__
extern void cpu_v8_do_resume(void *sp);
extern int cpu_v8_do_suspend(void *sp);

#define cpu_do_suspend cpu_v8_do_suspend
#define cpu_do_resume cpu_v8_do_resume
#endif /*__ASSEMBLY__*/
#endif /*__TRIX_PROC_OPS_H__*/
