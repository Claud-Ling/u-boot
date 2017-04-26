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
	ret = secure_read_uint32(0xf5005fe2);
	printf("misaligned access: %08x\n", ret);
	printf("attempt to write RO reg...\n");
	ret = secure_write_uint32(0xf5005fe0, 0x1234, 0xffff);
	printf("write result: %d\n", ret);
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

static int do_tee_mstate (int argc, char * const argv[])
{
	int ret;
	uint32_t state = 0;
	uintptr_t addr;
	size_t len;

	if (argc < 2)
		return CMD_RET_USAGE;

	addr = simple_strtoul(argv[0], NULL, 16);
	len  = simple_strtoul(argv[1], NULL, 16);
	printf("***** test mem access state (0x%lx+0x%zx) *****\n", addr, len);
	ret = secure_get_mem_state(addr, len, &state);
	printf("result: %d state: %x\n", ret, state);
	if (ret == 0) {
		int i;
		const char* attrs[] = {
			"secure accessiable",
			"non-secure read",
			"non-secure write",
			"secure executable",
			"non-secure executable"
		};
		for (i = 0; i < ARRAY_SIZE(attrs); i++) {
			printf("\t%-32s: %c\n", attrs[i], (state & (1 << i)) ? 'Y' : 'N');
		}
	}
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
		return do_tee_test(argc, argv);
	} else if (strncmp(subcmd, "mstate", 6) == 0) {
		return do_tee_mstate(argc, argv);
	} else {
		return CMD_RET_USAGE;
	}
}

/* ====================================================================== */
U_BOOT_CMD(
	tee,	CONFIG_SYS_MAXARGS,	1,	do_tee,
	"tee sub-system",
	"test - do quick test on tee\n"
	"tee mstate addr len - check memory access state\n"
	""
);

