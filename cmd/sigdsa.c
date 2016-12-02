#include <common.h>
#include <command.h>
#include <asm/io.h>

#ifdef CONFIG_SIGMA_DTV_SECURITY
/*defined in trix_emmc/security/security_boot.c*/
extern int authenticate_image(void *img, int len, void* key, void* sig);
static int do_signature_chk(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	int ret;
	void *img, *key, *sig;
	unsigned int len;

	if (argc < 5)
	{
		printf("too few parameters! argc=%d\n", argc);
		return CMD_RET_USAGE;
	}

	img = (void*)simple_strtoul(argv[1], NULL, 0);
	len = simple_strtoul(argv[2], NULL, 0);
	key = (void*)simple_strtoul(argv[3], NULL, 0);
	sig = (void*)simple_strtoul(argv[4], NULL, 0);
	ret = authenticate_image(img, len, key, sig);
	debug("[img:%p, len:%x, key:%p, sig:%p]\n", img, len, key, sig);
	printf("result: %s(%d)\n", (ret==0)?"OK":"FAILED", ret);
	return 0;
}

U_BOOT_CMD(
	sigchk, 5,  1, do_signature_chk,
	"check signature",
	"<img> <nbytes> <key> <sig> \n"
	"    - verify signature 'sig' for 'nbytes' payload of image 'img',\n"
	"      by using public key 'key'"
);
#endif
