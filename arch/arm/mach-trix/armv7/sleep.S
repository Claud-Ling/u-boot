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
	stmfd	sp!, {r4 - r11, lr}
	ldr	r4, =cpu_v7_suspend_size
	mov	r5, sp			@ current virtual SP
	add	r4, r4, #12		@ Space for pgd, virt sp, phys resume fn
	sub	sp, sp, r4		@ allocate CPU state on stack
	stmfd	sp!, {r0, r1}		@ save suspend func arg and pointer
	add	r0, sp, #8		@ save pointer to save block
	mov	r1, r4			@ size of save block
	mov	r2, r5			@ virtual SP
	ldr	r3, =sleep_save_sp

	bl	__cpu_suspend_save
	adr	lr, cpu_suspend_abort
	ldmfd	sp!, {r0, pc}		@ call suspend fn
ENDPROC(__cpu_suspend)

cpu_suspend_abort:
	ldmia	sp!, {r1 - r3}		@ pop phys pgd, virt SP, phys resume fn
	teq	r0, #0
	moveq	r0, #1			@ force non-zero value
	mov	sp, r2
	ldmfd	sp!, {r4 - r11, pc}
ENDPROC(cpu_suspend_abort)

/*
 * r0 = control register value
 */
ENTRY(cpu_resume_nommu)
	b	cpu_resume_after_mmu
ENDPROC(cpu_resume_nommu)

cpu_resume_after_mmu:
	@bl	cpu_init		@ restore the und/abt/irq banked regs
	mov	r0, #0			@ return zero on success
	ldmfd	sp!, {r4 - r11, pc}
ENDPROC(cpu_resume_after_mmu)

/*
 * resume entry
 */
ENTRY(cpu_resume)
	ldr	r0, sleep_save_sp	@ stack phys addr
	@setmode	PSR_I_BIT | PSR_F_BIT | SVC_MODE, r1  @ set SVC, irqs off
	@ load phys pgd, stack, resume fn
	ldmia	r0!, {r1, sp, pc}
ENDPROC(cpu_resume)

sleep_save_sp:
	.long	0			@ preserve stack phys ptr here
