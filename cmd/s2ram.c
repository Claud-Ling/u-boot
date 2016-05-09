
#include <common.h>
#include <command.h>

extern void s2ramtest_do_s2ram(int state);
extern int s2ramtest_do_s2ram_sz;
extern void s2ramtest_do_resume(void);
extern int s2ramtest_set_entry(void* entry, void* mem0, unsigned len0, void* mem1, unsigned len1);
extern int s2ramtest_finish_suspend(unsigned long stack);
extern void cpu_resume(void);
extern int cpu_suspend(unsigned long arg, int (*fn)(unsigned long));
#ifdef CONFIG_S2RAM_CHECKSUM
extern int s2ram_check_crc(void* frame);
extern void* my_resume_frame;
#endif

void (*do_s2ram_sram)(int state);

#define SRAM_BASE	0xfffe0000
#define SRAM_TEXT_MAX	0x3000	//12k
#define SRAM_SP_START	(SRAM_BASE + 0x3ffc)
/* Macro to push a function to the internal SRAM, using the fncpy API */
#define monza_sram_push(funcp, size) ({					\
	typeof(&(funcp)) _res = NULL;					\
	if (size < SRAM_TEXT_MAX)					\
		_res = memcpy((void*)SRAM_BASE, (void*)&(funcp), size);	\
	_res;								\
})

static void push_sram_idle(void)
{
	do_s2ram_sram = monza_sram_push(s2ramtest_do_s2ram, s2ramtest_do_s2ram_sz);
	if (!do_s2ram_sram) {
		printf("push sram error!\n");
	}
}

static int do_s2ram ( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;
	void *mem0=NULL, *mem1=NULL;
	unsigned int len0=0, len1=0;

	printf("\n");
	printf(" ***** start test sram ****\n");
	printf("\n");
	if (argc > 2) {
		mem0 = (void*)simple_strtoul( argv[1], NULL, 16 );
		len0 = simple_strtoul( argv[2], NULL, 16 );
	}
	if (argc > 4){
		mem1 = (void*)simple_strtoul( argv[3], NULL, 16 );
		len1 = simple_strtoul( argv[4], NULL, 16 );
	}

	printf(" ***** resume entry: 0x%08x ****\n", (int)cpu_resume);
	s2ramtest_set_entry(cpu_resume, mem0, len0, mem1, len1);
	push_sram_idle();
	ret = cpu_suspend(SRAM_SP_START, s2ramtest_finish_suspend);

	if (ret == 0) {
		/*success*/
#ifdef CONFIG_S2RAM_CHECKSUM
		s2ram_check_crc(&my_resume_frame);
#endif
		printf("warning: uboot is recovered, but not drivers\n");
	} else {
		/*fail*/
		printf(" ***** failed to do s2ram ****\n");
	}

	return 0;
}

/* ====================================================================== */
U_BOOT_CMD(
	s2ram,	CONFIG_SYS_MAXARGS,	1,	do_s2ram,
	"Test suspend to RAM",
	"[mem0] [len0] [mem1] [len1]\n"
	"    suspend system to ram and optionally support checksum on two memory blocks\n"
	"    which are specified by 'mem0'/'mem1' and 'len0'/'len1', where\n"
	"      'mem0' specifies start addr for memory#0, in hex\n"
	"      'len0' specifies byte length for memory#0, in hex\n"
	"      'mem1' specifies start addr for memory#1, in hex\n"
	"      'len1' specifies byte length for memory#1, in hex\n"
);
