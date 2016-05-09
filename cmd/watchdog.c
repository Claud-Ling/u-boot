#include <common.h>
#include <command.h>
#include <asm/io.h>

static void disable_mcu_watchdog(void)
{
	char val = 0;
	writeb(0x11, 0xf5000010);
	writeb(0x08, 0xf5000011);
	writeb(0xff, 0xf5000012);
	writeb(0x15, 0xf5000013);
	writeb(0x02, 0xf5000014);
	writeb(0x02, 0xf5000015);
	writeb(0x00, 0xf5000016);
	writeb(0x00, 0xf5000017);
	writeb(0xe7, 0xf5000018);
	writeb(0xfe, 0xf5000019);

	val = readb(0xf5000000);
	val = (val & (1<<3)) ? (val & (~(1<<3))) : (val | (1<<3));
	writeb(val, 0xf5000000);
	printf("Disable MCU watchdog!\n");
}
static void enable_mcu_watchdog(void)
{	
	char val = 0;
	writeb(0x11, 0xf5000010);
	writeb(0x08, 0xf5000011);
	writeb(0xff, 0xf5000012);
	writeb(0x15, 0xf5000013);
	writeb(0x03, 0xf5000014);
	writeb(0x02, 0xf5000015);
	writeb(0x00, 0xf5000016);
	writeb(0x00, 0xf5000017);
	writeb(0xe6, 0xf5000018);
	writeb(0xfe, 0xf5000019);

	val = readb(0xf5000000);
	val = (val & (1<<3)) ? (val & (~(1<<3))) : (val | (1<<3));
	writeb(val, 0xf5000000);
	printf("Enable MCU watchdog!\n");
	
}

static int do_watchdog(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	//int ret = -1;

	if (argc < 2)
		goto usage;

	if (strcmp(argv[1], "disable") == 0) {
		disable_mcu_watchdog();
		return 0;
	} else if (strcmp(argv[1], "enable") == 0) {
		enable_mcu_watchdog();
		return 0;
	}

usage:
	return CMD_RET_USAGE;
}


U_BOOT_CMD(
	watchdog,	2,	0,	do_watchdog,
	"Disable/Enable MCU watch dog",
	"watchdog disable/enable\n"
	"    -Disable/Enable MCU watchdog"
);
