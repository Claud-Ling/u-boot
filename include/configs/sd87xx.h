/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_SD87XX_H
#define __CONFIG_SD87XX_H

/*
 * system top level configurations
 */
#undef CONFIG_RPP_25MHZ             /* RPP/RTSM/normal confs are mutuxally exclusive */					
#undef CONFIG_RPP_50MHZ            
#undef CONFIG_RTSM                 
#define CONFIG_MACH_TANGO_8734
#define CONFIG_TANGOX_SMC
#define CONFIG_8734_NAND_TIMING_WORKAROUND
#define CONFIG_BUG41394_WORKAROUND
#define CONFIG_BOARD_LATE_INIT
#define CONFIG_ANDROID_SUPPORT
/*#define CONFIG_OF_LIBFDT*/		/* device tree support */


/*
 * cache Configuration
 */
#define CONFIG_SYS_DCACHE_SIZE		32768
#define CONFIG_SYS_ICACHE_SIZE		32768
#define CONFIG_SYS_CACHELINE_SIZE	32
#define CONFIG_SYS_L2_PL310

/*
 * system speed and memory related 
 */

#define CONFIG_NR_DRAM_BANKS        1

#if defined(CONFIG_RPP_25MHZ) || defined(CONFIG_RPP_50MHZ)
#   define MEM_SIZE                 128 /* in MB */
#   ifdef CONFIG_RPP_50MHZ
#       define CONFIG_SYS_MHZ       25  /* system is running a 25 MHz */
#   else
#       define CONFIG_SYS_MHZ       12  /* system is running at 12.5MHz */
#   endif
#else
#   define MEM_SIZE                 32  /* in MB */
#   define CONFIG_SYS_MHZ           358
#endif
#define CONFIG_SYS_BOOTM_LEN        0x2000000 /* 32 MB */

#undef CONFIG_MEMSIZE_IN_BYTES
#define CONFIG_SYS_MALLOC_LEN       1024*1024
#define CONFIG_SYS_MIPS_TIMER_FREQ  (CONFIG_SYS_MHZ * 1000000)
#define CONFIG_SYS_HZ               1000

#define CONFIG_SYS_SDRAM_BASE       0x80000000	/* SDRAM base address */ 
#define CONFIG_SYS_LOAD_ADDR        0x84001000  /* default load address */

/* start and end address of mem test command */
#define CONFIG_SYS_MEMTEST_START    0x84000000
#define CONFIG_SYS_MEMTEST_END	     0x85000000 

/* The following #defines are needed to get flash environment right */
#define CONFIG_SYS_TEXT_BASE        0x81800000
#define CONFIG_SYS_MONITOR_BASE     CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_INIT_SP_OFFSET	 0x400000
#define CONFIG_SYS_INIT_SP_ADDR     0x81900000

#define CONFIG_IXKC_ADDR            0x87400000

#define CONFIG_ENV_SIZE             0x8000
#define CONFIG_ENV_OVERWRITE        1

#define CONFIG_SYS_PL310_BASE	     0x20100000


/*
 * system init/boot related
 */
#define CONFIG_ARCH_MISC_INIT
#undef CONFIG_MISC_INIT_R
#define CONFIG_BOARD_EARLY_INIT_R
#define CONFIG_SKIP_LOWLEVEL_INIT 

/*
 * sysetm device block level configurations
 */
#if defined(CONFIG_RPP_25MHZ) || defined(CONFIG_RPP_50MHZ) || defined(CONFIG_RTSM)
#	define CONFIG_SYS_NO_FLASH
#else
/* disable all phys */
#	define CONFIG_PHY_BROADCOM
#	define CONFIG_PHY_DAVICOM
#	define CONFIG_PHY_ET1011C
#	define CONFIG_PHY_ICPLUS
#	define CONFIG_PHY_LXT
#	define CONFIG_PHY_MARVELL
#	define CONFIG_PHY_MICREL
#	define CONFIG_PHY_NATSEMI
#	define CONFIG_PHY_REALTEK
#	define CONFIG_PHY_SMSC
#	undef CONFIG_PHY_TERANETICS
#	define CONFIG_PHY_VITESSE
#	define CONFIG_PHY_ATHEROS
/* tangox devices */
#	define CONFIG_TANGOX_SATA
#	define CONFIG_TANGOX_USB
#	define CONFIG_SYS_NO_FLASH
#	define CONFIG_MTD_DEVICE
#endif

/* phy dependent */
#if defined(CONFIG_PHY_VITESSE)
#   define CONFIG_PHY_RESET_DELAY   500
#   define CONFIG_PHY_VITESSE_EHN_LED
#endif

/* sata dependent */
#if defined(CONFIG_TANGOX_SATA)
#   define CONFIG_SYS_SATA_MAX_DEVICE 2
#   define CONFIG_LIBATA
#endif

/* usb stroage device related */
#if defined(CONFIG_TANGOX_USB)
#   define CONFIG_USB_EHCI_TANGOX
#   define CONFIG_SYS_USB_EHCI_MAX_ROOT_PORTS  2
#   define CONFIG_EHCI_HCD_INIT_AFTER_RESET
#   define CONFIG_SUPPORT_VFAT
#   define CONFIG_DOS_PARTITION
#endif

/* sd/mmc dependent */
#if defined(CONFIG_TANGOX_MMC)
#   define CONFIG_MMC
#   define CONFIG_GENERIC_MMC
#   define CONFIG_SDHCI
#   define CONFIG_MMC_SDMA
#   define CONFIG_MMC_SDHCI_IO_ACCESSORS
#   define CONFIG_FAT_WRITE
#endif


/*
 * general - console/env related
 */

#define CONFIG_ENV_IS_NOWHERE

#define CONFIG_BOOTDELAY            3   /* autoboot after this */

#if defined(CONFIG_RPP_50MHZ)
#   define CONFIG_BAUDRATE          115200
#elif defined(CONFIG_RPP_25MHZ) 
#   define CONFIG_BAUDRATE          57600
#else
#   define CONFIG_BAUDRATE          115200
#endif

#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }
#define CONFIG_TIMESTAMP            /* Print image info with timestamp */

#if defined(CONFIG_RPP_25MHZ) || defined(CONFIG_RPP_50MHZ)
#   define CONFIG_BOOTARGS "console=ttyS0,115200 mem=128M earlyprintk"
#else
#   define CONFIG_BOOTARGS "console=ttyS0,115200 mem=256M earlyprintk"
#endif


#define CONFIG_EXTRA_ENV_SETTINGS           \
	"autoload=no\0"                         \
	"bootfile=/tftpboot/vmlinux\0"          \
	"load=tftp 80500000 ${u-boot}\0"        \
    "bootcmd=run mmcboot\0"                 \
    "mmcboot=mmc part;fatload mmc 0:1 0x86000000 uimage;bootm 0x86000000\0"     \
	""

#define CONFIG_BOOTCOMMAND	        "bootm 0x86000000"

#define CONFIG_SYS_LONGHELP         /* undef to save memory */

#define CONFIG_AUTO_COMPLETE
#define CONFIG_CMDLINE_EDITING
#define CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "

#define CONFIG_SYS_CBSIZE           512	    /* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE+sizeof(CONFIG_SYS_PROMPT)+16)  /* Print Buffer Size */
#define CONFIG_SYS_MAXARGS          16      /* max number of command args */
#define CONFIG_SYS_BOOTPARAMS_LEN   128*1024

/* related to setting up ATAG */
#define CONFIG_CMDLINE_TAG          
#define CONFIG_SETUP_MEMORY_TAGS  
#define CONFIG_INITRD_TAG

/* network related default configurations */
#define CONFIG_IPADDR               20.20.1.102         /* Our IP address     */
#define CONFIG_NETMASK              255.255.255.0       /* Netmask            */
#define CONFIG_GATEWAYIP            20.20.1.1	        /* Gateway IP address */
#define CONFIG_SERVERIP             20.20.1.100         /* Server IP address  */
#define CONFIG_ETHADDR              00:00:00:00:00:00   /* default eth MAC    */
#define CONFIG_ETH1ADDR             00:00:00:00:00:00   /* default eth1 MAC   */

/*
 * BOOTP options
 */
#define CONFIG_BOOTP_BOOTFILESIZE
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME

/*
 * Command line configuration.
 */
#if defined(CONFIG_ANDROID_SUPPORT)
#   define CONFIG_CMD_BOOTA
#endif

#define CONFIG_CMD_MMC
#define CONFIG_CMD_BOOTX
#define CONFIG_CMD_FAT
 
/* supporing ARMOR shell */
/* #define CONFIG_CMD_ARMORSHELL */



/*
 * serial driver related
 */
#define CONFIG_SYS_NS16550_MEM32 
#define CONFIG_SYS_NS16550_SERIAL
#if !defined(CONFIG_OF_LIBFDT)
#	define CONFIG_SYS_NS16550_REG_SIZE	-4		/* TANGO3 has 4 byte reg width, little endian */
#endif

#if defined(CONFIG_RPP_50MHZ)
#   define CONFIG_SYS_NS16550_CLK       23961600 /* 50Mhz, 115200bps */
#elif defined(CONFIG_RPP_25MHZ)
#   define CONFIG_SYS_NS16550_CLK       11980800 /* for 25MHz */
#else
#   define CONFIG_SYS_NS16550_CLK       7372800	 /* this value comes from YAMON */
#endif

#define CONFIG_SYS_NS16550_COM1     0x10700
/*
#define CONFIG_SYS_NS16550_COM2     0x00//KSEG1ADDR(REG_BASE_cpu_block+CPU_UART1_base)
#define CONFIG_SYS_NS16550_COM3     0x00//KSEG1ADDR(REG_BASE_cpu_block+CPU_UART2_base)
*/

#define CONFIG_CONS_INDEX           1
#define CONFIG_SERIAL_MULTI
#define CONFIG_SYS_CONSOLE_IS_IN_ENV


/*
 * NAND releated configs
 */

#if defined(CONFIG_CMD_NAND)
#	define CONFIG_NAND_SMP8xxx
#   define CONFIG_SYS_MAX_NAND_DEVICE	    2
#   define CONFIG_SYS_NAND_ONFI_DETECTION
#endif

/* spi nor flash */
#if defined(CONFIG_TANGOX_SPI)
#   define CONFIG_SPI_FLASH
#   define CONFIG_TANGOX_SPI_BUS        0
#   define CONFIG_TANGOX_SPI_CS         0
#   define CONFIG_DEFAULT_SPI_BUS       CONFIG_TANGOX_SPI_BUS
#   define CONFIG_SF_DEFAULT_SPEED      25000000
#   define CONFIG_SF_DEFAULT_MODE       SPI_MODE_0

/* SPI flash manufacturers */
#   define CONFIG_SPI_FLASH_MACRONIX
/*
# define CONFIG_SPI_FLASH_ATMEL
# define CONFIG_SPI_FLASH_SPANSION
# define CONFIG_SPI_FLASH_SST
# define CONFIG_SPI_FLASH_STMICRO
# define CONFIG_SPI_FLASH_WINBOND
*/
#endif

/*
 * libraries
 */
#define CONFIG_LZMA


#endif /* __CONFIG_H */
