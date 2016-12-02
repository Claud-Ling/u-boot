#include <config.h>

#include "../macro.S"

/*
 * Save CPU state for a suspend.  This saves the CPU general purpose
 * registers, and allocates space on the kernel stack to save the CPU
 * specific registers and some other data for resume.
 *  r0 = suspend function arg0
 *  r1 = suspend function
 */
ENTRY(__cpu_suspend)
	ret
ENDPROC(__cpu_suspend)

cpu_suspend_abort:
	ret
ENDPROC(cpu_suspend_abort)

/*
 * r0 = control register value
 */
ENTRY(cpu_resume_nommu)
	b	cpu_resume_after_mmu
ENDPROC(cpu_resume_nommu)

cpu_resume_after_mmu:
	ret
ENDPROC(cpu_resume_after_mmu)

/*
 * resume entry
 */
ENTRY(cpu_resume)
	ret
ENDPROC(cpu_resume)

sleep_save_sp:
	.long	0			/* preserve stack phys ptr here */
