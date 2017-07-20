
#include <common.h>
#include <command.h>
#include <asm/arch/umac.h>

#define MAX_MEM_BANK_SIZE	0x40000000ul

#ifdef DEBUG
# define trace(fmt...) do{printf(fmt);}while(0)
#else
# define trace(fmt...) do{}while(0)
#endif

static u32 memtell (int id)
{
	u32 bs = 0;
	/* only probe activated umac */
	if (umac_is_activated(id)) {
		unsigned long base = umac_get_addr(id);
		bs = (u32)get_ram_size((long*)base, MAX_MEM_BANK_SIZE);
		trace("find 0x%08x bytes memory at addr 0x%lx\n", bs, base);
	}
	return bs;
}

static int do_mprobe ( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u32 msz = 0;

	argc--;
	argv++;

	if (argc > 1) {
		long *base;
		u32 max;
		base = (void*)simple_strtoul(argv[0], NULL, 16);
		max = simple_strtoul(argv[1], NULL, 16);
		msz = get_ram_size(base, max);
		printf("find 0x%08x bytes memory at addr %p\n", msz, base);
	} else {
		int i;
		u32 sz[CONFIG_SIGMA_NR_UMACS];
#if CONFIG_SIGMA_NR_UMACS > 0
		sz[0] = memtell(0);
#endif
#if CONFIG_SIGMA_NR_UMACS > 1
		sz[1] = memtell(1);
#endif
#if CONFIG_SIGMA_NR_UMACS > 2
		sz[2] = memtell(2);
#endif
#if CONFIG_SIGMA_NR_UMACS > 3
# error "too much UMACs, max 3 by now!"
#endif

		for (i = 0, msz = 0; i < CONFIG_SIGMA_NR_UMACS; i++) {
			msz |= (sz[i] / (CONFIG_SIGMA_MSIZE_GRANULE<<20)) << (4 * i);
		}

		trace("msize %x\n", msz);
		setenv_hex("msize", msz);
	}
	return 0;
}

/* ====================================================================== */
U_BOOT_CMD(
	mprobe,	CONFIG_SYS_MAXARGS,	1,	do_mprobe,
	"memory probe",
	"[base] [max] - probe memory info\n"
	"    Probe memory info on specified range [base, base + max], where\n"
	"      'base' specifies base address, in hex\n"
	"      'max'  specifies maximum memory size starting from 'base', in hex\n"
	"    Or probe memory info on all UMACs by default, and output memory size to env 'msize' in form of \"zyx\",\n"
	"    where x/y/z indicates memory size of UMAC0/1/2 in 128MB granule\n"
	""
);

