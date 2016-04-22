
#include <common.h>
#include <command.h>

#ifdef CONFIG_TANGOX_SMC
#include <asm/arch/setup.h>	/*ARMOR_SMCCALL_SHELL*/
extern void do_smc_call( int, int, void*);
#elif defined(CONFIG_DTV_SMC)
#include <asm/arch/setup.h>	/*ARMOR_SMCCALL_SHELL*/
#endif


static int do_armorshell (int argc, char * const argv[])
{
#if defined(CONFIG_TANGOX_SMC) || defined(CONFIG_DTV_SMC)
	printf("\n");
	printf(" ***** Switching to ARMOR shell ****\n");
	printf("\n");
	do_smc_call(ARMOR_SMCCALL_SHELL, 0, NULL);
	printf("\n");
	printf(" ***** Return back from ARMOR shell ****\n");
	printf("\n");
#else
	printf("SMC is not available!!\n");
#endif
    
    return 0;
}

#ifdef CONFIG_DTV_SMC
static int do_armorcfg (int argc, char * const argv[])
{
	if (!secure_get_security_state())
		setenv ("armor_config", "with_armor");
	else
		setenv ("armor_config", "");
	return 0;
}
#endif

static int do_armor ( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const char* subcmd = NULL;

	argc--;
	argv++;
	if (argc > 0) {
		subcmd = argv[0];
		argc--;
		argv++;
	}else{
		subcmd = "shell";	/*default shell*/
	}

	if ((strncmp(subcmd, "shell", 5) == 0) ||
		(strncmp(subcmd, "sh", 2) == 0)) {
		do_armorshell(argc, argv);
#ifdef CONFIG_DTV_SMC
	} else if ((strncmp(subcmd, "config", 6) == 0) ||
		(strncmp(subcmd, "cfg", 3) == 0)) {
		do_armorcfg(argc, argv);
#endif
	}
	return 0;
}

/* ====================================================================== */
U_BOOT_CMD(
	armor,	2,	1,	do_armor,
	"armor sub-system",
	"[shell](sh) - get into armor shell\n"
#ifdef CONFIG_DTV_SMC
	"armor config(cfg) - config in uboot for armor\n"
#endif
	""
);

