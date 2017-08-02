#include <common.h>
#include <command.h>
#include <asm/io.h>
#include "libfw/include/fw_core.h"
#include "libfw/include/libfw.h"

enum {
	PREBOOT_KEY=1,
	UBOOT_KEY,
	KERNEL_KEY,
	APP_KEY,
	INVALID_KEY
};
extern int signature_check(void *data, int len, int key_type, void *signature);

static struct fw_ctx *g_ctx = NULL;

static void fw_init(void)
{
	g_ctx = fw_get_board_ctx();
}

static void dep_vol_handler(struct fw_ctx *ctx, struct fw_vol *vol)
{
	char command_buff[128] = { 0 };
	int32_t flash_typ = fdetecter_main_storage_typ();
	struct fw_part *part = NULL;
	if (strncmp(&vol->info->fs[0], "mipsimg",
				strlen(&vol->info->fs[0]))) {
		printf("Depend volume(%s)"
			" operation Not support!!\n", vol->info->name);
	}

	char *s = getenv("bootmips");

	if (s && (*s!='y') && (*s!='Y')) {
		return;
	}

	/* Mips image case */
	part = fw_vol_get_active_part(vol);
	sprintf(command_buff, "bootmips %llx:", part->info->start);
	if (flash_typ == FW_FTYP_EMMC) {
		strcat(command_buff, "mmc");
	} else if (flash_typ == FW_FTYP_NAND) {
		strcat(command_buff, "nand");
	} else {
		printf("Flash type(%d) not support for bootmips\n", flash_typ);
		return;
	}
	run_command(&command_buff[0], 0);

}

static void volume_switch_part(struct fw_vol *vol)
{
	struct fw_part *act_part = fw_vol_get_active_part(vol);
	struct fw_part *inact_part = fw_vol_get_inactive_part(vol);

	/* Impossible */
	if (!act_part || !inact_part) {
		BUG();
		return;
	}

	printf("\r\n############switch part %s -> %s##########\r\n", act_part->info->name, inact_part->info->name);

	fw_vol_set_active_part(vol, inact_part);
	return;
}

static void fw_boot(void)
{
	int ret = -1, rd_sz;;
	struct fw_vol *boot = NULL;
	struct fw_part *part = NULL;
	struct fw_vol *dep_vol =  NULL;
	int len = 0;
	void *data = NULL;
	char *s = NULL;
	unsigned long load_addr = 0;
	uint64_t image_sz = 0;
	char cmd[64] = { 0 };

#if CONFIG_ARM64
	/*skip 2k Android bootimg header */
	load_addr = 0x80000 - (2<<10);
#else
	/* Load to 16M offset @DDR */
	load_addr = 0x1000000;
#endif

	s = getenv("kernel_load_addr");

	if (s) {
		load_addr = simple_strtoul(s, 0, 16);
	}

	boot = fw_get_vol_by_ability_enabled(g_ctx, FW_AB_BOOT, FW_BOOT_ACT);
	if (!boot) {
		printf("fw: No boot active volume found!\n");
		return;
	}

	dep_vol = fw_get_dep_vol(g_ctx, boot);
	if (dep_vol) {
		dep_vol_handler(g_ctx, dep_vol);
	}

	part = fw_vol_get_active_part(boot);

	data = &part->info->need_sig;
	len = (int)((unsigned long)&part->info->info_sig[0] -
				(unsigned long)&part->info->need_sig - 1); 

	ret = signature_check(data, len, PREBOOT_KEY, &part->info->info_sig[0]);
	if (ret) {
		printf("\r\n##########Firmware info  RSA verify fail!############\r\n");
		volume_switch_part(boot);
		goto failed;
	}

	ret = fw_open_volume(g_ctx, boot->info->name, FW_VOL_CONTENT_INUSE);
	if (ret < 0) {
		printf("Can't open volume(%s)!\n", boot->info->name);
		return;
	}


	/*
	 * valid_sz indicate the valid content size of this part, so here perferred
	 * load valid data can improve boot time.
	 */
	(part->info->valid_sz > 0) ? (image_sz = part->info->valid_sz):(image_sz = part->info->size);
	rd_sz = fw_read_volume(g_ctx, (void *)load_addr, image_sz);

	if (rd_sz < 0) {
		printf("fw: Load volume(%s) content failed!\n", boot->info->name);
		return;
	}

	if (!part->info->need_sig) {
		goto skip_sig;
	}

	ret = signature_check((void *)load_addr, part->info->valid_sz, PREBOOT_KEY, &part->info->part_sig[0]);
	if (ret) {
		printf("\r\n##########Volume(%s)  RSA verify fail!############\r\n", boot->info->name);
		volume_switch_part(boot);
		goto failed;
	}

skip_sig:
	snprintf(cmd, sizeof(cmd), "boota %lx", load_addr);
	run_command(cmd, 0);
	return;
failed:
	printf("\r\n############Signature check failed!, system halt##########\r\n");
	fw_ctx_save_to_flash(g_ctx);
	while(1);
}

static int do_fw(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	//int ret = -1;

	if (argc < 2)
		goto usage;

	if (strcmp(argv[1], "init") == 0) {
		if (g_ctx == NULL)
			fw_init();
		return 0;
	} else if (strcmp(argv[1], "boot") == 0) {
		if (g_ctx == NULL) {
			fw_init();
		}
		if (g_ctx == NULL) {
			printf("No vaild firmware info on board!\n");
			return 0;
		}
		fw_boot();
		return 0;

	} else if (strcmp(argv[1], "dump") == 0) {
		if (g_ctx == NULL) {
			fw_init();
		}
		if (g_ctx == NULL) {
			printf("No vaild firmware info on board!\n");
			return 0;
		}
		fw_dump(g_ctx);
		return 0;
	} else if (strcmp(argv[1], "get_part_list") == 0) {
		char *part_list = NULL;
		part_list = libfw_board_get_part_list();
		if (!part_list) {
			return -1;
		}
		printf("%s\n", part_list);
		free(part_list);
		part_list = NULL;
		return 0;
	} else if (strcmp(argv[1],"set_boot_from") == 0) {
		char *vol_name = argv[2];
		int ret = -1;

		if (!vol_name) {
			printf("volume name is needed!\n");
			return -1;
		}

		ret = libfw_board_set_boot_from(vol_name);
		if (ret<0) {
			printf("Failed: please make sure this volume exist and have boot ability\n");
		}
		fw_init();

		return ret;
	} else if (strcmp(argv[1], "get_boot_volume") == 0) {

		char  *boot_vol = NULL;

		boot_vol = libfw_board_get_boot_volume();
		if (!boot_vol) {
			printf("fw: No boot active volume found!\n");
			return -1;
		}
		printf("boot volume is: %s\n", boot_vol);
		free(boot_vol);
		boot_vol = NULL;
		return 0;
	} else if (strcmp(argv[1], "switch_part") == 0) {
		char *volume_name = argv[2];
		struct fw_vol *vol = NULL;
		if (!volume_name) {
			printf("volume name be needed!\n");
			return CMD_RET_USAGE;
		}

		fw_init();
		if(g_ctx == NULL) {
			printf("No vaild firmware info on board!\n");
			return 0;
		}

		vol = fw_get_vol_by_name(g_ctx, volume_name);
		if (!vol) {
			printf("No such volume(%s) in firmware info\n", volume_name);
			return 0;
		}

		volume_switch_part(vol);
		fw_ctx_save_to_flash(g_ctx);
		return 0;
	} else if (strcmp(argv[1], "leave_message") == 0) {
		char *message_name = argv[2];
		char *message = argv[3];
		int ret = 0;

		if (!message_name) {
			printf("message name must be specified!\n");
			return CMD_RET_USAGE;
		}

		fw_init();
		if(g_ctx == NULL) {
			printf("No vaild firmware info on board!\n");
			return 0;
		}

		ret = libfw_board_leave_message(message_name, message);
		if (ret) {
			printf("Leave message failed!\n");
			return -1;
		}

		return 0;

	} else if (strcmp(argv[1], "get_message") == 0) {
		char *message_name = argv[2];
		char *message = NULL;

		if (!message_name) {
			printf("message name must be specified!\n");
			return CMD_RET_USAGE;
		}

		fw_init();
		if(g_ctx == NULL) {
			printf("No vaild firmware info on board!\n");
			return 0;
		}

		message = libfw_board_get_message(message_name);
		if (!message) {
			printf("Get message failed!\n");
			return -1;
		}
		printf("message:%s\n", message);
		return 0;
	}


usage:
	return CMD_RET_USAGE;
}


U_BOOT_CMD(
	fw,	5,	0,	do_fw,
	"Firmware info command",
	"init\n"
	"    -init fw_ctx\n\n"
	"boot\n"
	"    -Load boot active volume and boot\n\n"
	"dump\n"
	"    -Output all content of firmware info\n\n"
	"get_part_list\n"
	"    -Output partition layout\n\n"
	"set_boot_from <volume name>\n"
	"    -Set which volume will be booted with 'fw boot' command\n"
	"         'volume name' specify volume want to set as primary boot volume\n\n"
	"get_boot_volume\n"
	"    -Output primary boot volume name\n\n"
	"swtich_part <volume name>\n"
	"    -Switch specified volume active partition\n\n"
	"leave_message <message_name> [message]\n"
	"    -Store a message into firmware info\n"
	"    -[message] is optional, if NULL means delete the message\n"
	"    -specified by <message_name>\n\n"
	"get_message <message_name>\n"
	"    -Get a message from firmware info\n\n"
);
