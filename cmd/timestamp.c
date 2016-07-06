#include <common.h>
#include <command.h> 
#include <asm/arch/setup.h>

static int  do_showtimestamp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	show_arch_timestamp();
	return 0;
}

U_BOOT_CMD(
	showts,  1,  0,  do_showtimestamp,
	"show soc timer value elapsed\n"
	"clock @200MHz in ascending order",
	"- show time elapsed from power on\n"
);
