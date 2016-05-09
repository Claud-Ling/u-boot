
#include <common.h>
#include <asm/arch/setup.h>

/*
 * This is used to verify if the configuration header
 * was executed by rom code prior to control of transfer
 * to the bootloader. boot1 is responsible for saving and
 * passing the boot_params pointer to the u-boot.
 */
struct bootparam_frame boot_params __attribute__ ((section(".data")));

#define DFT_BOOT_MSG	""
#define DFT_BOOT_DEVICE DEV_UNKNOWN
#define DFT_BOOT_REASON	REASON_UNKNOWN
#define DFT_BOOT_FLAGS	0

#define CHECK_BOOTPARAM ((boot_params.BOOTPARAM_HEAD == BOOTPARAM_HEAD_TAG) \
			&& (boot_params.BOOTPARAM_TAIL == BOOTPARAM_TAIL_TAG))

long sx6_boot_device(void)
{
	long dev = DFT_BOOT_DEVICE;
	if (CHECK_BOOTPARAM)
	{
		dev = boot_params.BOOTPARAM_DEV;
		debug("boot device: %ld\n", dev);
	}
	return dev;
}

char* sx6_boot_message(void)
{
	char *str = DFT_BOOT_MSG;
	if (CHECK_BOOTPARAM)
	{
		str = (char*) (boot_params.BOOTPARAM_MSG);
		debug("boot msg: %s\n", str);
	}
	return str;
}

long sx6_boot_reason(void)
{
	long reason = DFT_BOOT_REASON;
	if (CHECK_BOOTPARAM)
	{
		reason = boot_params.BOOTPARAM_REASON;
		debug("boot reason: %ld\n", reason);
	}
	return reason;
}

long sx6_boot_flags(void)
{
	long flags = DFT_BOOT_FLAGS;
	if (CHECK_BOOTPARAM)
	{
		flags = boot_params.BOOTPARAM_FLAGS;
		debug("boot flags: 0x%08lx\n", flags);
	}
	return flags;
}
