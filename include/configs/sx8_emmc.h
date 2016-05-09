
#ifndef __CONFIG_H
#define __CONFIG_H

/*
 *  system top level configurations
 */

#define CONFIG_MACH_SIGMA_SX8
#define CONFIG_SIGMA_TRIX_NAME	"SX8"
#define CONFIG_SIGMA_NR_UMACS	3

/*
 *  cache configuration
 */

#define CONFIG_SYS_DCACHE_SIZE		(32<<10)
#define CONFIG_SYS_ICACHE_SIZE		(32<<10)
#define CONFIG_SYS_CACHELINE_SIZE	32
/* #define CONFIG_SYS_ICACHE_OFF */
/* #define CONFIG_SYS_DCACHE_OFF */
#define CONFIG_SYS_L2CACHE_OFF


/*
 *  memory usages
 */

#define CONFIG_SYS_TEXT_BASE        0x02000000  /* 32M location */

#define CONFIG_NR_DRAM_BANKS 		1
#define CONFIG_SYS_SDRAM_BASE	    0x00000000
#define CONFIG_SDRAM_SIZE		    0xbc00000	/* 188M, same as kernel low mem */

#define CONFIG_SYS_MEMTEST_START	CONFIG_SYS_SDRAM_BASE
#define CONFIG_SYS_MEMTEST_END		0x90000000

#define CONFIG_SYS_MALLOC_LEN		(2*1024*1024)
#define CONFIG_SYS_BOOTPARAMS_LEN	(128*1024)

#define CONFIG_SYS_INIT_SP_ADDR		0x02E00000  /* 32M + 14M location */
#define	CONFIG_SYS_LOAD_ADDR		0x08000000	/* default load address	*/

/* #define CONFIG_SKIP_RELOCATE_UBOOT */
/* #define CONFIG_MISC_INIT_R */

/*
 * board configuration option
 */
#define CONFIG_BOARD_CONFIGURATION

/*
 * boot mips option
 */
#define CONFIG_SYS_BOOTMIPS

/*
 * secure monitor option
 */
#define CONFIG_SECURE_MONITOR

/*
 * s2ram test option
 */
#define CONFIG_TEST_S2RAM

/*
 * bootparam option
 */
#define CONFIG_BOOTPARAM

/*
 * syslog option
 */
#define CONFIG_SYSLOG

/*
 * memory configure
 */
#define CONFIG_SIGMA_MCONFIG

/*
 * auto update option
 */
#define CONFIG_SIGMA_AUTOUPDATE

/*
 * board late init
 */
#define CONFIG_BOARD_LATE_INIT

/*
 * flash
 */
#define CONFIG_SYS_MAX_FLASH_BANKS	2	    /* max number of memory banks */
#define CONFIG_SYS_MAX_FLASH_SECT	(128)	/* max number of sectors on one chip */

/*
 * NOR flash configuration
 */
#define CONFIG_SYS_NO_FLASH
#define CONFIG_SYS_FLASH_BASE       0x1c000000



/*
 * sysetm device block level configurations
 */
#define CONFIG_MII
#define CONFIG_PHYLIB
#define CONFIG_PHY_SMSC			/* SMSC LAN8720 */
#define CONFIG_ETH_DESIGNWARE	/* Designware Ethernet Controller */

/* #define CONFIG_NAND_MONZA */
/* #define CONFIG_MTD_DEVICE */

#define CONFIG_MONZA_MMC


/*
 * Select USB controller in u-boot
 * Only one controller can be selected
 */
#define CONFIG_USB_MONZA_EHCI
/* #define CONFIG_USB_MONZA_XHCI */


#define	CONFIG_EXTRA_ENV_SETTINGS					\
    "bootcmd=run check_bmode\0"   \
    "verify=n\0"          \
    "boot_size=18000\0"   \
    "mem_config=mem=188M mem=167M@857M mem=410M@2150M vmalloc=512M\0"  \
    "mem_config488=mem=188M mem=167M@857M mem=410M@2150M vmalloc=512M\0"  /*for 512M(umac2)+1G(umac1)+1G(umac0)*/\
    "console_config=console=ttyS0,115200n8 earlyprintk=serial,uart0,115200\0" \
    "normal_rootfs=/dev/ram0 rw\0"  \
    "recovery_rootfs=/dev/ram0 rw\0"  \
    "nfs_rootfs=root=/dev/nfs rw nfsroot=server_ip:path rdinit=/none init=/init\0"  \
    "emmc_partitions=blkdevparts=mmcblk0:6M(reserved),1M(env),48M(recovery)"    \
        ",48M(boot),320M(system),128M(cache),1G(sdcard),-(data)"  \
        ";mmcblk0boot0:512K(uboot),64K(mcu),-(reserved)"  \
        ";mmcblk0boot1:512K(uboot),64K(mcu),-(reserved)\0"  \
    "mmc_normal=setenv bootargs ${normal_rootfs} ${mem_config} ${console_config}"    \
        " androidboot.hardware=sx6 ethaddr=${ethaddr} ${emmc_partitions} ${armor_config}\0" \
    "mmc_recovery=setenv bootargs ${recovery_rootfs} ${mem_config} ${console_config}"    \
        " androidboot.hardware=sx6 ethaddr=${ethaddr} ${emmc_partitions} ${armor_config}\0"    \
    "mmc_nfs=setenv bootargs ${nfs_rootfs} ${mem_config} ${console_config}"    \
        " androidboot.hardware=sx6 ethaddr=${ethaddr} ${emmc_partitions} ${armor_config}"	\
        " ip=:::::eth0:dhcp\0" \
    "start_normal=mmc read 8000000 1B800 ${boot_size};boota 8000000\0"    \
    "start_recovery=mmc read 8000000 3800 18000;boota 8000000\0"  \
    "check_bmode=run check_bmips;run set_memcfg;if itest.s ${bootmode} == normalmode; then "   \
        "run ${mmc_mode} start_normal; else "    \
        "run mmc_recovery start_recovery; fi\0"  \
    "bootmode=normalmode\0" \
    "mmc_mode=mmc_normal\0" \
    "android_update_file=/mnt/update.zip\0"   \
    "mconfig=n\0" /*triple state: 'y'-auto; 'm'-manual; 'n'-off*/\
    "msize=488\0" \
    "prepare_memcfg=set do_memcfg set mem_config $\"{mem_config${msize}}\"\0" \
    "set_memcfg=if itest.s ${mconfig} == y || itest.s ${mconfig} == m;then run prepare_memcfg do_memcfg;fi\0" \
    "bootmips=n\0" /*dual state: 'y'-enable; 'n'-disable*/\
    "start_bootmips=bootmips 700000:mmc\0" \
    "check_bmips=if itest.s ${bootmips} == y;then run start_bootmips;fi\0" \
    "usb_update=dcache off;armor cfg;set update_method USB;usb start;if fatload usb 0 4000000 fs.sys;then "	\
	"source 4000000;else dcache on;run bootcmd; fi;" \
        "if fatload usb 0 4000000 safe-kernel.img1; then crc_start_kernel 4000000 fscmdline;"	\
	"else dcache on;run bootcmd;fi\0"         \
    ""
#include <configs/sigma-dtv-common.h>

#endif	/* __CONFIG_H */

