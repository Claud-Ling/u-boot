#include <common.h>
#include <command.h>
#include <asm/arch/setup.h>

#define TOTEXT(val) #val
static void print_boot_device(long val)
{
	const char* str = NULL;
	switch(val){
	case DEV_SPI:
		str = TOTEXT(DEV_SPI); break;
	case DEV_NAND:
		str = TOTEXT(DEV_NAND); break;
	case DEV_EMMC:
		str = TOTEXT(DEV_EMMC); break;
	default:
		str = TOTEXT(DEV_UNKNOWN); break;
	}
	printf("%-14s: %s\n", "[boot_device]", str);
}

static void print_boot_reason(long val)
{
	const char* str = NULL;
	switch(val) {
	case REASON_AC_OFF:
		str = TOTEXT(REASON_AC_OFF); break;
	case REASON_DC_OFF:
		str = TOTEXT(REASON_DC_OFF); break;
	default:
		str = TOTEXT(REASON_UNKNOWN); break;
	}
	printf("%-14s: %s\n", "[boot_reason]", str);
}

static void print_boot_flags(long val)
{
	printf("%-14s: ", "[boot_flags]");
	if (FLAGS_SECURITY & val)
		printf("%s|", TOTEXT(FLAGS_SECURITY));
	if (FLAGS_MCU & val)
		printf("%s|", TOTEXT(FLAGS_MCU));
	if (FLAGS_CFG & val)
		printf("%s|", TOTEXT(FLAGS_CFG));
	if (FLAGS_SSC & val)
		printf("%s|", TOTEXT(FLAGS_SSC));
	if (FLAGS_SMC & val)
		printf("%s|", TOTEXT(FLAGS_SMC));
	if (FLAGS_S2RAM & val)
		printf("%s|", TOTEXT(FLAGS_S2RAM));
	printf("\b \n");
}

static int do_bootparam ( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i = 0;
	printf("%-14s:\n", "[param data]");
	for(i=0; i<BOOTPARAM_FRM_SIZE; i+=4)
	{
		if (!(i & 0xf))
		{
			if( i ) printf("\n");
			printf("%08x: ", i);
		}
		printf("%08lx ", boot_params.data[i >> 2]);
	}
	printf("\n");
	printf("%-14s: %s\n", "[boot_message]", sx6_boot_message());
	print_boot_device(sx6_boot_device());
	print_boot_reason(sx6_boot_reason());
	print_boot_flags(sx6_boot_flags());
	return 0;
}

/* ====================================================================== */
U_BOOT_CMD(
	bootparam,	1,	1,	do_bootparam,
	"Show boot params that are given by preboot",
	""
);
