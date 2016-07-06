#ifndef __SIGMA_DTV_COMMON_H__
#define __SIGMA_DTV_COMMON_H__

/*
 * board late init
 */
#define CONFIG_BOARD_LATE_INIT


/*
 * Flattened device tree support
 */
#define CONFIG_OF_LIBFDT


/*
 * Ethernet PHY support.
 */
#define CONFIG_PHY_SMSC			/* SMSC LAN8720 */
#define CONFIG_PHY_MICREL		/* Micrel PHY */
#define CONFIG_PHY_REALTEK		/* Realtek PHY */



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

#define CONFIG_SYS_MALLOC_LEN		(10*1024*1024)
#define CONFIG_SYS_BOOTPARAMS_LEN	(128*1024)

#define CONFIG_SYS_INIT_SP_ADDR		0x02E00000  /* 32M + 14M location */
#define	CONFIG_SYS_LOAD_ADDR		0x08000000	/* default load address	*/

/* #define CONFIG_SKIP_RELOCATE_UBOOT */
/* #define CONFIG_MISC_INIT_R */

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

/* related to setting up ATAG */
#define CONFIG_CMDLINE_TAG          
#define CONFIG_SETUP_MEMORY_TAGS  
#define CONFIG_INITRD_TAG
#define CONFIG_REVISION_TAG

/* Display CPU and Board Info */
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

/*kernel run address, used for start_kernel command*/
#define	CONFIG_SYS_START_KERNEL_ADDR	0x8000

/* monza specific, TODO: remove me, seems no one use this */
#define CONFIG_PLF_MMIO_BASE0       0xF5000000
#define CONFIG_PLF_MMIO_BASE1       0xF6000000
#define CONFIG_PLF_MMIO_BASE2       0xFB000000

/* Designware net MAC driver */
#if defined(CONFIG_ETH_DESIGNWARE)
#   define CONFIG_DW_ALTDESCRIPTOR
#   define CONFIG_DESIGNWARE_ETH_HW_BASE    (0x15030000 + 0xE0000000)
#   define CONFIG_DW_AUTONEG		        /* Using Auto Negotiation */
#   define CONFIG_DW_SEARCH_PHY	            /* Auto Detect Ethernet Phy Address */
#   define CONFIG_NET_MULTI                 // do we really need this???
#   define CONFIG_PHY_RESET_DELAY	        10000
#   define CONFIG_BRD_NET_INIT_DELAY        100    /* resolution of msec */
#endif

/* FTMAC net driver*/
#if defined(CONFIG_FTMAC110)
#   define CONFIG_FTMAC110_BASE		(0x1b004000 + 0xE0000000)
#   define CONFIG_RANDOM_MACADDR
#   define CONFIG_LIB_RAND
#endif

/*nand flash configuration */
#if defined(CONFIG_TRIX_NAND)
#   define CONFIG_SYS_NAND_SELF_INIT
#   define CONFIG_SYS_MAX_NAND_DEVICE	    1
#   define CONFIG_SYS_NAND_BASE		        0xFC000000
#   define CONFIG_NAND_MLC_BCH
/*#   define CONFIG_MTD_NAND_ECC_512*/
#   define CONFIG_NAND_MODE
/* #define CONFIG_MTD_NAND_DMA */
#   define CONFIG_SYS_NAND_ONFI_DETECTION
#   define MTDIDS_DEFAULT "nand0=nand0"
#   define MTDPARTS_DEFAULT "mtdparts=nand0:10M(boot),22m(system),20M(rootfs),-(app)"
#endif

/* usb */
#if defined (CONFIG_USB_EHCI_TRIX)
#   define CONFIG_EHCI_HCD_INIT_AFTER_RESET
#   define CONFIG_SYS_USB_EHCI_MAX_ROOT_PORTS	8
/* #define CONFIG_EHCI_DCACHE */
#   define CONFIG_CMD_USB
#   define CONFIG_USB_STORAGE
#   define CONFIG_USB_NETWORK
#   define CONFIG_USB_MAX_CONTROLLER_COUNT      2
#endif
/* usb3.0 */
#if defined (CONFIG_USB_XHCI_TRIX)
#   define CONFIG_CMD_USB
#   define CONFIG_USB_STORAGE
#   define CONFIG_USB_NETWORK
#   define CONFIG_SYS_USB_XHCI_MAX_ROOT_PORTS 2
#endif


/* sd/mmc dependent */
#if defined(CONFIG_TRIX_MMC)
#   define MMC_BASE_ADDR                        (0xfb00a000)
#   define CONFIG_MMC
#   define CONFIG_GENERIC_MMC
#   define CONFIG_SDHCI
#   define CONFIG_MMC_SDMA
/* #   define CONFIG_MMC_TRACE */
/* #   define CONFIG_MMC_SDHCI_IO_ACCESSORS */
#endif

/*clock */

/* Use General purpose timer 1 */
#define V_OSCK                      38400000    /* pheri clock*/
#define V_SCLK                      V_OSCK
#define CONFIG_SYS_PTV			    2	        /* Divisor: 2^(PTV+1) => 8 */
#define CONFIG_SYS_HZ			    1000
#define CONFIG_SYS_TIMERBASE 	    0xF5027000


/*
 * console
 */
#define CONFIG_BOOTDELAY	        0	/* autoboot after 5 seconds	*/
#define	CONFIG_TIMESTAMP		        /* Print image info with timestamp */
/*#define CONFIG_SYS_HUSH_PARSER*/
#define CONFIG_SYS_PROMPT_HUSH_PS2  "> "
/* #define CONFIG_AUTOBOOT_KEYED */
#define CONFIG_ZERO_BOOTDELAY_CHECK
/* #define CONFIG_AUTOBOOT_STOP_STR "s" */
/* #define CONFIG_SILENT_CONSOLE */
/* #define CONFIG_DISABLE_CONSOLE */

/*
 * serial driver related
 */
#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE -1
#define CONFIG_CONS_INDEX           1		
#define CONFIG_SYS_NS16550_COM1     0xFB005100
#define CONFIG_SYS_NS16550_CLK      1843200 /* (115200 * 16) */

/* 
 * baudrates 
 */
#define CONFIG_BAUDRATE		        115200
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

/* saving env */
#if defined(CONFIG_TRIX_NAND)

#   define CONFIG_ENV_IS_IN_NAND
#   define CONFIG_ENV_OFFSET            0x00600000     /* 6 MB   */
#   define CONFIG_ENV_RANGE             0x00001000     /* 4 KB */
/*#   define CONFIG_ENV_OFFSET_REDUND	    0x00020000 */    /* 128  KB */
#   define CONFIG_ENV_SIZE              0x00001000     /* 4  KB */
/* #   define CONFIG_CMD_SAVEENV */	/* 2016 u-boot default enable*/ 
#   define CONFIG_ENV_OVERWRITE
/* redundant env in nand */
#   define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#   define CONFIG_ENV_OFFSET_REDUND     (CONFIG_ENV_OFFSET + (0x00300000))
#   define CONFIG_ENV_SIZE_REDUND       CONFIG_ENV_SIZE

#elif defined (CONFIG_TRIX_MMC)

#   define CONFIG_ENV_IS_IN_MMC
#   define CONFIG_SYS_MMC_ENV_DEV       0
#   define CONFIG_ENV_OFFSET            0x00600000     /* 6 MB */
#   define CONFIG_ENV_SIZE              0x00001000     /* 4 KB */
/*#   define CONFIG_CMD_SAVEENV */	/* 2016 u-boot default enable*/
#   define CONFIG_ENV_OVERWRITE
/* redundant env in mmc */
#   define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#   define CONFIG_ENV_OFFSET_REDUND     (CONFIG_ENV_OFFSET + CONFIG_ENV_SIZE)
#   define CONFIG_ENV_SIZE_REDUND       CONFIG_ENV_SIZE
#if defined(CONFIG_SYS_REDUNDAND_ENVIRONMENT) && defined(CONFIG_ENV_SIZE_REDUND)
#   define CONFIG_ENV_AUTHENTICATION
#endif
#ifdef CONFIG_ENV_AUTHENTICATION
#   define CONFIG_ENV_GROUP1_DYN1_OFFSET	(CONFIG_ENV_OFFSET)
#   define CONFIG_ENV_GROUP1_DYN2_OFFSET	((CONFIG_ENV_OFFSET) + 1*(CONFIG_ENV_SIZE))
#   define CONFIG_ENV_GROUP1_STATIC_OFFSET	((CONFIG_ENV_OFFSET) + 2*(CONFIG_ENV_SIZE))

#   define CONFIG_ENV_GROUP2_BASE		(CONFIG_ENV_OFFSET + 512*1024)
#   define CONFIG_ENV_GROUP2_DYN1_OFFSET	(CONFIG_ENV_GROUP2_BASE)
#   define CONFIG_ENV_GROUP2_DYN2_OFFSET	(CONFIG_ENV_GROUP2_BASE + CONFIG_ENV_SIZE)
#   define CONFIG_ENV_GROUP2_STATIC_OFFSET	(CONFIG_ENV_GROUP2_BASE + 2*CONFIG_ENV_SIZE)
#endif
#else
#   define CONFIG_ENV_IS_NOWHERE
#   define CONFIG_ENV_SIZE              0x00001000     /* 4 KB */
#endif

/* support long help */
#define CONFIG_SYS_LONGHELP

/* The following macros are needed to get flash environment right */
#define	CONFIG_SYS_MONITOR_BASE	    CONFIG_SYS_TEXT_BASE
#define	CONFIG_SYS_MONITOR_LEN		(192 << 10)

/*console info*/
#define CONFIG_SYS_CBSIZE           1024         /* Console I/O Buffer Size   */
#define CONFIG_SYS_PBSIZE           (CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)    /* Print Buffer Size */
#define CONFIG_SYS_MAXARGS          32          /* max number of command args*/
#define CONFIG_CMDLINE_EDITING

/* HIVE CONFIGURATION */
#define CONFIG_IPADDR		        192.168.1.224
#define CONFIG_NETMASK              255.255.255.0
#define CONFIG_GATEWAYIP	        192.168.1.1
#define CONFIG_SERVERIP		        192.168.1.100
#define CONFIG_ETHADDR		        00:25:8F:00:10:30

/*tftp config*/
#define CONFIG_TFTP_BLOCKSIZE 	    16384
#define CONFIG_IP_DEFRAG

/*kernel start*/
#define CONFIG_CMD_KERNEL
#define CONFIG_CMD_BOOTA


#define CONFIG_CMD_CACHE
#define CONFIG_CMD_REGINFO
#undef  CONFIG_CMD_IMLS
#define CONFIG_CMD_SCRIPT
#define CONFIG_CMD_WATCHDOG

#if defined(CONFIG_TRIX_NAND)
#   define CONFIG_CMD_NAND
#   define CONFIG_SYS_NAND_MAX_ECCPOS  360
#   define CONFIG_SYS_NAND_MAX_OOBFREE 8
#   define CONFIG_MTD_DEVICE
#endif

/*mtd*/
#if defined(CONFIG_MTD_DEVICE)
#   define CONFIG_CMD_UBI
#   define CONFIG_CMD_UBIFS
#   define CONFIG_CMD_MTDPARTS
#   define CONFIG_MTD_PARTITIONS  1
#   define CONFIG_LZO             1
#endif

#if defined(CONFIG_TRIX_MMC)
#   define CONFIG_CMD_MMC
#endif

/*filesystem*/
#define CONFIG_CMD_FAT
#define CONFIG_CMD_CRAMFS
#define CONFIG_DOS_PARTITION
#define CONFIG_CRAMFS_CMDLINE

/*net*/
#define CONFIG_CMD_MII          /* MII support                  */
#define CONFIG_CMD_PING         /* ping support                 */
#define CONFIG_CMD_DHCP         /* DHCP Support                 */


/*general module*/
#define CONFIG_RBTREE
#define CONFIG_CMD_AUTOSCRIPT

/* signature */
#define CONFIG_CMD_SIGDSA

/* board configuration dependent */
#ifdef CONFIG_BOARD_CONFIGURATION
#   define CONFIG_SPEC_DDR_ENTRY  0x8000
#   define CONFIG_CMD_LOADCFG
/* #   define CONFIG_WAIT_MCU_READY */
#   ifdef CONFIG_WAIT_MCU_READY
#      define CONFIG_MCU_DELAY	5
#   endif
#endif

/* bootmips dependent */
#if defined(CONFIG_SYS_BOOTMIPS)
#   define CONFIG_DEBUG_MIPS
#   define CONFIG_CMD_BOOTMIPS
#   define CONFIG_CMD_LOADSETTINGS
#   define CONFIG_PMAN_ENTRY_SUPPORT
#endif

/* secure monitor dependent */
#if defined(CONFIG_SECURE_MONITOR)
#   define CONFIG_DTV_SMC
#   define CONFIG_CMD_ARMORSHELL	/* armor		*/
#endif

/* test s2ram */
#ifdef CONFIG_TEST_S2RAM
#   define CONFIG_CMD_S2RAM
#   define CONFIG_S2RAM_CHECKSUM	/* checksum support (for debug) */
#   define CONFIG_SIGMA_PM_UMAC_PHY    /* UMAC PHY PM support */
#endif

/* bootparam */
#ifdef CONFIG_BOOTPARAM
#   define CONFIG_DTV_BOOTPARAM
#   define CONFIG_CMD_BOOTPARAM
#endif

/* syslog */
#ifdef CONFIG_SYSLOG
#   define CONFIG_CMD_SYSLOG
#endif

/* mconfig */
#ifdef CONFIG_SIGMA_MCONFIG
#   define CONFIG_CMD_MPROBE
#endif

/* Use timer0 get high percision time stamp */
#ifdef CONFIG_TRIX_TIMESTAMP
#      define CONFIG_CMD_TIMESTAMP
#endif

#ifdef CONFIG_TRIX_NAND
#define	CONFIG_EXTRA_ENV_SETTINGS					\
    "bootcmd=run check_bmode\0"   \
    "verify=n\0"          \
    "boot_size=1400000\0"   \
    "console_config=console=ttyS0,115200n8 earlyprintk=serial,uart0,115200\0" \
    "ramdisk_rootfs=/dev/ram0 rw\0"  \
    "nfs_rootfs=root=/dev/nfs rw nfsroot=server_ip:path rdinit=/none init=/init ip=:::::eth0:dhcp\0"  \
    "partitions=mtdparts=trix-nand:27M(Uboot),50M(reserved_data),20M(systemA),20M(systemB),30M(etc),150M(APP_A),150M(APP_B),50M(rw_data),-(data)\0"  \
    "set_bootargs=setenv bootargs ${rootfs} ${mem_config} ${console_config}"    \
        " ethaddr=${ethaddr} ${partitions} ${armor_config} OPT=${opt_partition} bootmode=${bootmode}\0" \
    "check_bmode=run check_bmips;run set_memconfig setup_rootfs set_bootargs;if itest.s ${bootmode} == systemA; then "   \
        "run start_systemA; else "    \
        "run start_systemB; fi\0"  \
    "bootmode=systemA\0" \
    "rootfs_type=ramdisk\0" \
    "setup_rootfs=if itest.s ${rootfs_type} == nfs; then setenv rootfs ${nfs_rootfs}; else setenv rootfs ${ramdisk_rootfs}; fi\0" \
    "mconfig=n\0" /*triple state: 'y'-auto; 'm'-manual; 'n'-off*/\
    "msize=488\0" \
    "bootmips=n\0" /*dual state: 'y'-enable; 'n'-disable*/\
    "start_bootmips=bootmips 700000:nand\0" \
    "check_bmips=if itest.s ${bootmips} == y;then run start_bootmips;fi\0" \
    "start_systemA=nand read 8000000 4d00000 ${bootsize};boota 8000000\0" \
    "start_systemB=nand read 8000000 6100000 ${bootsize};boota 8000000\0" \
    "mem_configA=mem=188m  redund_env\0" \
    "mem_configB=mem=188m  redund_env\0" \
    "set_memconfig=if itest.s ${opt_partition} == mtdblock.APP_A; then set mem_config ${mem_configA}; else set mem_config ${mem_configB}; fi\0" \
    "opt_partition=mtdblock.APP_A\0" \
    "usbport=1\0" \
    "usb_update=dcache off;armor cfg;set update_method USB;usb reset;if fatload usb 0 4000000 fs.sys;then "	\
	"source 4000000;else dcache on;run bootcmd; fi;" \
        "if fatload usb 0 4000000 safe-kernel.img1; then crc_start_kernel 4000000 fscmdline;"	\
	"else dcache on;run bootcmd;fi\0"         \
    ""

#endif

#ifdef CONFIG_TRIX_MMC
#define	CONFIG_EXTRA_ENV_SETTINGS					\
    "bootcmd=run check_bmode\0"   \
    "verify=n\0"          \
    "boot_size=18000\0"   \
    "mem_config=mem=188M mem=167M@857M mem=410M@2150M vmalloc=512M\0"  \
    "mem_config488=mem=188M mem=167M@857M mem=410M@2150M vmalloc=512M\0"  /*for 512M(umac2)+1G(umac1)+1G(umac0)*/\
    "console_config=console=ttyS0,115200n8 earlyprintk=serial,uart0,115200\0" \
    "ramdisk_rootfs=/dev/ram0 rw\0"  \
    "nfs_rootfs=root=/dev/nfs rw nfsroot=server_ip:path rdinit=/none init=/init ip=:::::eth0:dhcp\0"  \
    "emmc_partitions=blkdevparts=mmcblk0:6M(reserved),1M(env),48M(recovery)"    \
        ",48M(boot),320M(system),128M(cache),1G(sdcard),-(data)"  \
        ";mmcblk0boot0:512K(uboot),64K(mcu),-(reserved)"  \
        ";mmcblk0boot1:512K(uboot),64K(mcu),-(reserved)\0"  \
    "set_bootargs=setenv bootargs ${rootfs} ${mem_config} ${console_config}"    \
        " androidboot.hardware=sx6 ethaddr=${ethaddr} ${emmc_partitions} ${armor_config}\0" \
    "start_normal=mmc read 8000000 1B800 ${boot_size};boota 8000000\0"    \
    "start_recovery=mmc read 8000000 3800 18000;boota 8000000\0"  \
    "check_bmode=run check_bmips;run set_memcfg check_mmc_mode set_bootargs;if itest.s ${bootmode} == normalmode; then "   \
        "run start_normal; else "    \
        "run start_recovery; fi\0"  \
    "bootmode=normalmode\0" \
    "rootfs_type=ramdisk\0" \
    "setup_rootfs=if itest.s ${rootfs_type} == nfs; then setenv rootfs ${nfs_rootfs}; else setenv rootfs ${ramdisk_rootfs}; fi\0" \
    "android_update_file=/mnt/update.zip\0"   \
    "mconfig=y\0" /*triple state: 'y'-auto; 'm'-manual; 'n'-off*/\
    "msize=488\0" \
    "prepare_memcfg=set do_memcfg set mem_config $\"{mem_config${msize}}\" msize=${msize}\0" \
    "set_memcfg=if itest.s ${mconfig} == y || itest.s ${mconfig} == m;then run prepare_memcfg do_memcfg;fi\0" \
    "bootmips=n\0" /*dual state: 'y'-enable; 'n'-disable*/\
    "start_bootmips=bootmips 700000:mmc\0" \
    "check_bmips=if itest.s ${bootmips} == y;then run start_bootmips;fi\0" \
    "usbport=1\0" \
    "usb_update=dcache off;armor cfg;set update_method USB;usb reset;if fatload usb 0 4000000 fs.sys;then "	\
	"source 4000000;else dcache on;run bootcmd; fi;" \
        "if fatload usb 0 4000000 safe-kernel.img1; then crc_start_kernel 4000000 fscmdline;"	\
	"else dcache on;run bootcmd;fi\0"         \
    ""
#endif
#endif /*__SIGMA_DTV_COMMON_H__*/
