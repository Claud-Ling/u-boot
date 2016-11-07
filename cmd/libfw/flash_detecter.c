#include <fw_core.h>

static int main_storage = FW_FTYP_EMMC;
static int boot_storage = FW_FTYP_SPI;
static int detected = 0;

#if !defined(__KERNEL__)
//FIXME: 8192 bytes is large enough for OTP proc file size
#define PROC_FILE_LEN	(8192)

static uint32_t boot_from_rom(void)
{
	int fd = -1, ret;
	void *buff = NULL;
	char *p;
	uint32_t val = 0;

	fd = open("/proc/otp/FC_2", O_RDONLY);
	if (fd<0) {
		fw_debug("%s: Can't read otp!! Treat as boot from rom\n", __func__);
		goto boot_from_rom;
	}

	buff = (void *)malloc(PROC_FILE_LEN);
	if (!buff) {
		fw_debug("%s: Can't alloc buff for parser otp\n", __func__);
		close(fd);
		goto boot_from_rom;
	}
	memset(buff, 0, PROC_FILE_LEN);

	ret = read(fd, buff, PROC_FILE_LEN);
	if (ret <  0) {
		fw_debug("%s: Read otp failed\n", __func__);
		close(fd);
		free(buff);
		buff = NULL;
		goto boot_from_rom;
	}
	p = strstr(buff, OTP_STR);
	p = (char *)((unsigned long)p + strlen(OTP_STR));

	val = strtoull(p, NULL, 10);
	fw_debug("%s: %s %d\n", __func__, OTP_STR, val);

	close(fd);
	free(buff);
	buff = NULL;

	if (val) {
		goto boot_from_rom;
	} else {
		goto spi_boot;
	}

spi_boot:
	return 0;

boot_from_rom:
	return 1;
}
#endif

static int32_t detect_flash_type(void)
{
#if !defined(__KERNEL__)
	int fd;
	struct stat st;
	struct mtd_info_user mtd;

	/*
 	 * Try with NAND device first
 	 */
	if (stat(NAND_DEV, &st)) {
		goto try_emmc;
	}
	fd = open(NAND_DEV, O_RDONLY);
	if (fd == -1) {
		printf("Can't open device %s\n", NAND_DEV);
		goto try_emmc;
	}

	if (ioctl(fd, MEMGETINFO, &mtd)) {
		printf("Can't get device %s MTD info\n", NAND_DEV);
		close(fd);
		goto try_emmc;
	}

	/*so far not have uenv on NOR flash*/
	if (mtd.type == MTD_NORFLASH) {
		goto try_emmc;
	}

	main_storage = FW_FTYP_NAND;
	close(fd);
	goto end;

try_emmc:
	/*
	 * Try with eMMC device
	 */
	if (stat(EMMC_DEV, &st)) {
		printf("No ENV device found!\n");
		goto unknown;
	}

	main_storage = FW_FTYP_EMMC;
	goto end;

unknown:
	main_storage = FW_FTYP_UNKNOWN;

end:
	if (boot_from_rom()) {
		boot_storage = main_storage;
	} else {
		boot_storage = FW_FTYP_SPI;
	}

	detected = 1;
	return 0;
#else
	#if defined(CONFIG_TRIX_MMC)
	main_storage = FW_FTYP_EMMC;
	#else
	main_storage =  FW_FTYP_NAND;
	#endif

	boot_storage = main_storage;
	detected = 1;
	return 0;
#endif
}

int32_t fdetecter_main_storage_typ(void)
{
	if (!detected)
		detect_flash_type();

	return main_storage;
}

int32_t fdetecter_boot_storage_typ(void)
{
	if (!detected)
		detect_flash_type();

	return boot_storage;
}

