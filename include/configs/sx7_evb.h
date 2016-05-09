
#ifndef __CONFIG_H
#define __CONFIG_H

/*
 *  system top level configurations
 */

#define CONFIG_MACH_SIGMA_SX7
#define CONFIG_SIGMA_TRIX_NAME	"SX7"
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

#define CONFIG_SYS_MALLOC_LEN		(4*1024*1024)
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
/* #define CONFIG_SECURE_MONITOR */

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
/* #define CONFIG_SIGMA_MCONFIG */

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

#define CONFIG_NAND_MONZA
#define CONFIG_MTD_DEVICE

/*#define CONFIG_MONZA_MMC*/


/*
 * Select USB controller in u-boot
 * Only one controller can be selected
 */
#define CONFIG_USB_MONZA_EHCI
/* #define CONFIG_USB_MONZA_XHCI */


#define	CONFIG_EXTRA_ENV_SETTINGS					\
	"nfsargs=setenv bootargs root=/dev/nfs rw "			\
		"nfsroot=${serverip}:${rootpath}\0"			\
	"ramargs=setenv bootargs root=/dev/ram rw\0"			\
	"addip=setenv bootargs ${bootargs} "				\
		"ip=${ipaddr}:${serverip}:${gatewayip}:${netmask}"	\
		":${hostname}:${netdev}:off\0"				\
	"addmisc=setenv bootargs ${bootargs} "				\
		"console=ttyS0,${baudrate} "				\
		"ethaddr=${ethaddr} "					\
		"panic=1\0"						\
	"flash_nfs=run nfsargs addip addmisc;"				\
		"bootm ${kernel_addr}\0"				\
	"flash_self=run ramargs addip addmisc;"				\
		"bootm ${kernel_addr} ${ramdisk_addr}\0"		\
	"net_nfs=tftp 80500000 ${bootfile};"				\
		"run nfsargs addip addmisc;bootm\0"			\
	"rootpath=/opt/eldk/mips_4KC\0"					\
	"bootfile=/tftpboot/INCA/uImage\0"				\
	"kernel_addr=B0040000\0"					\
	"ramdisk_addr=B0100000\0"					\
	"u-boot=/tftpboot/INCA/u-boot.bin\0"				\
	"load=tftp 80500000 ${u-boot}\0"				\
	"load_kernel=usb start;fatload usb 0 8000 vmlinux.bin; start_kernel\0"				\
	"update=protect off 1:0-2;era 1:0-2;"				\
		"cp.b 80500000 B0000000 ${filesize}\0"			\
    "mtd_partitions=512k(env0),512k(env1),32M(recovery),"   \
        "8M(kernel),32M(rootfs),192M(system),128M(cache),-(data)\0" \
    "mtdparts=mtdparts=nand0:$(mtd_partitions)\0"    \
    "bootargs=root=/dev/ram0 rw mem=64M "   \
        "mtdparts=sx6-nand:$(mtd_partitions) "   \
        "console=ttyS0,115200n8 "   \
        "earlyprintk=serial,uart0,115200\0"   \
    "usb_update=dcache off; set update_method USB;usb start;if fatload usb 0 4000000 fs.sys;"	\
	"then source 4000000;else dcache on;run bootcmd; fi;"	\
        "if fatload usb 0 4000000 safe-kernel.img1; then "	\
	"crc_start_kernel 4000000 fscmdline;else dcache on;run bootcmd;fi\0"         \
	""


#include <configs/sigma-dtv-common.h>

#endif	/* __CONFIG_H */

