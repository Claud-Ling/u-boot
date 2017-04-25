/**
  @file   tee.c
  @brief
	This file decribes test code for sigma designs defined secure services

  @author Tony He, tony_he@sigmadesigns.com
  @date   2017-04-25
  */

#include <common.h>
#include <command.h>
#include <asm/arch/setup.h>	/*secure_xxx*/

static int do_tee_test (int argc, char * const argv[])
{
	int i, ret;
	uint32_t rsa_key[64];
	printf("************probe sip svc*****************\n");
	ret = secure_svc_probe();
	printf("result: %d\n", ret);
	printf("************test secure mmio**************\n");
	ret = secure_read_uint32(0xf500300c);
	printf("dcsn bc_addr: %08x\n", ret);
	ret = secure_read_uint32(0xf5003010);
	printf("dcsn bc_stat: %08x\n", ret);
	ret = secure_read_uint32(0xf5005000);
	printf("pman_sec0 addr_mask: %08x\n", ret);
	ret = secure_read_uint32(0xf5005fe0);
	printf("pman_sec0 int_status: %08x\n", ret);
	printf("************test access otp***************\n");
	ret = otp_get_security_boot_state();
	printf("secure_boot state: %d\n", ret);
	ret = otp_get_rsa_key_index();
	printf("rsa key index: %d\n", ret);
	ret = secure_get_rsa_pub_key(rsa_key, sizeof(rsa_key));
	printf("get rsa pub key: %d\n", ret);
	for (i = 0; i < ARRAY_SIZE(rsa_key); i++) {
		if (i && !(i & 3))
			printf("\n");
		printf("%08x ", rsa_key[i]);
	}
	printf("\n");
	return 0;
}

static int do_tee ( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const char* subcmd = NULL;

	argc--;
	argv++;
	if (argc > 0) {
		subcmd = argv[0];
		argc--;
		argv++;
	}

	if (strncmp(subcmd, "test", 4) == 0) {
		do_tee_test(argc, argv);
		return 0;
	} else {
		return CMD_RET_USAGE;
	}
}

/* ====================================================================== */
U_BOOT_CMD(
	tee,	2,	1,	do_tee,
	"tee sub-system",
	"test - do quick test on tee\n"
	""
);

