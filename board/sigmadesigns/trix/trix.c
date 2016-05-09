
#include <asm/arch/reg_io.h>

#include <common.h>
#include <command.h>
#include <netdev.h>
#include <phy.h>

#ifdef CONFIG_PHY_MICREL
#       include <micrel.h>
#endif

#ifdef CONFIG_GENERIC_MMC
#include <sdhci.h>
#endif
#include <asm/arch/reg_io.h>
#include <asm/arch/setup.h>

DECLARE_GLOBAL_DATA_PTR;

extern int monza_sdhi_init(u32 regbase,  u32 quirks);
extern int is_vizio_products(void);

void _machine_restart(void)
{

}

phys_size_t initdram(int board_type)
{
	return (64 << 20);
}

int checkboard (void)
{
	return 0;
}

extern void usb_power_init(void);
int board_init(void)
{
    /* use hard coded value */
    gd->bd->bi_arch_number = MACH_TYPE_SIGMA_TRIX;
    gd->bd->bi_boot_params = (0x00000000 + 0x100); /* boot param addr */
    /* ease usb power */
    usb_power_init();
	return 0;
}

unsigned long flash_init(void)
{
	return 0;
}

#if 0
void reset_cpu(ulong addr)
{

	return;
}
#endif

int dram_init(void)
{
	gd->ram_size = CONFIG_SDRAM_SIZE;
	return 0;
}

int print_cpuinfo(void)
{
	puts("CPU  : Sigma Design "CONFIG_SIGMA_TRIX_NAME"\n");

	return 0;
}


#ifdef CONFIG_CMD_NET
int board_eth_init(bd_t *bis)
{
#if defined (CONFIG_ETH_DESIGNWARE)
	unsigned char tmp;

	tmp = ReadRegByte(0xf500ec17);
	tmp |= 0x8; //bit3 = 1 enable REFCLK for sx6 revb,50MHz clock output
	WriteRegByte(0xf500ec17, tmp);
#ifdef CONFIG_MACH_SIGMA_SX7
	WriteRegByte(0xf500ea2c, 0x7f);	//GBE_TXEN
	WriteRegByte(0xf500ea2d, 0x7f);	//GBE_TXC
	WriteRegByte(0xf500ea2e, 0x7f);	//GBE_TXD0
	WriteRegByte(0xf500ea2f, 0x7f);	//GBE_TXD1
	WriteRegByte(0xf500ea30, 0x7f);	//GBE_TXD2
	WriteRegByte(0xf500ea31, 0x7f);	//GBE_TXD3
	WriteRegByte(0xf500ea38, 0x7f);	//GBE_MDC
#elif defined(CONFIG_MACH_SIGMA_SX8)
	#warning "FIXME: SX8: Add code for ethernet init!"
#endif

	/* Initialize Designware Ethernet Controller */
	return designware_initialize( CONFIG_DESIGNWARE_ETH_HW_BASE,PHY_INTERFACE_MODE_RMII);
#elif defined (CONFIG_FTMAC110)
	MWriteRegWord(0xf502206c, 0x000e0000, 0x0fff0000); //set up UMAC agent21 for ethernet
	WriteRegHWord(0xfb00016c, 0x0021); //set MAC controller and DMA brigde as little endian
	WriteRegHWord(0xfb00016e, 0x0000); //write 16C and 16E orderly,referd to SPG 3.4.3
	return ftmac110_initialize(bis);
#else
	# error "unknown Ethernet Controller"
#endif
}
#endif

#ifdef CONFIG_GENERIC_MMC
int board_mmc_init(bd_t *bis)
{
#define CARD_DETECT_TIMEOUT_US	1000000
#define DELAY_TIME_SLICE_US	1000
	int retry = CARD_DETECT_TIMEOUT_US / DELAY_TIME_SLICE_US;
	monza_sdhi_init(MMC_BASE_ADDR, 0);
	/*wait for card ready*/
	while(retry--)
	{
		unsigned int status = ReadRegWord(MMC_BASE_ADDR + SDHCI_PRESENT_STATE);
		if (status & SDHCI_CARD_PRESENT) break;
		udelay(DELAY_TIME_SLICE_US);
	}

	if (retry < 0)
		printf("failed to detect mmc card in %d ms\n", CARD_DETECT_TIMEOUT_US / 1000);

	return 0;
}
#endif

#ifdef CONFIG_BOARD_EARLY_INIT_F
int board_early_init_f(void)
{
	return 0;
}
#endif

#ifdef CONFIG_BOARD_LATE_INIT
/*
 * return 1: off, 0:on
 */
static int is_console_off(void)
{
	char *s = getenv("console_off");
	return (s && (*s == 'y')) ? 1 : 0;
}

#ifdef CONFIG_SIGMA_MCONFIG
static int is_mconfig_on(void)
{
	char *s = getenv("mconfig");
	return (s && (*s == 'y')) ? 1 : 0;
}
#endif

int board_late_init(void)
{
#ifdef CONFIG_CMD_LOADCFG
	run_command("loadcfg", 0);
#endif
#ifdef CONFIG_DTV_SMC
	if (!secure_get_security_state())
		setenv ("armor_config", "with_armor");
	else
		setenv ("armor_config", "");
#endif
#ifdef CONFIG_SIGMA_MCONFIG
	if (is_mconfig_on()) {
		run_command("mprobe", 0);
	}
#endif
#ifdef CONFIG_SIGMA_DTV_SECURITY
	if(is_vizio_products())
	{
	/*
	 * for system security, we invalidate UART RXD pin
	 * to disable console input from now on (neither in Linux shell)
	 */
#ifdef CONFIG_MACH_SIGMA_SX7
		MWriteRegByte(0xf500ea4d , 0x0 ,0x1); /* disable internal input buffer */
#elif defined(CONFIG_MACH_SIGMA_SX6)
		MWriteRegByte(0xf500ee1b , 0x6 ,0x7); /* set pin RXD as bidirectional gpio */
		MWriteRegByte(0xf500ea11 , 0x0 ,0x1); /* disable internal input buffer */
#elif defined(CONFIG_MACH_SIGMA_UXLB)
		//TODO
#elif defined(CONFIG_MACH_SIGMA_SX8)
		#warning "FIXME: SX8: Set disable UART pinshare in secure case!!"
#endif
		/* Cleanup UART FIFO */
		WriteRegByte(0xFB005102, 0xc7);
	}
	else if (SIMGA_TRIX_SECURITY_STATE)
	{
		/*
		 * Currently, don't disable console for easily debug
		 */
		//gd->flags |= GD_FLG_DISABLE_CONSOLE;	/*disable console*/
	}
#endif
	if(is_console_off())
		gd->flags |= GD_FLG_DISABLE_CONSOLE;    /*disable console*/	
	return 0;
}
#endif

#ifdef CONFIG_REVISION_TAG
#define BOARD_REVISION 100
u32 get_board_rev(void)
{
	return BOARD_REVISION;
}
#endif

int board_phy_config(struct phy_device *phydev)
{
#ifdef CONFIG_PHY_MICREL
#define SX7A_KSZ9031_CTRL_SKEW 0x0000
#define SX7A_KSZ9031_RX_SKEW   0x0000
#define SX7A_KSZ9031_TX_SKEW   0x3330
#define SX7A_KSZ9031_CLK_SKEW  0x03ef

	if ((phydev->phy_id < 0) || (phydev->addr < 0))
	{
		if (phydev->drv->config)
			phydev->drv->config(phydev);

		return 0;
	}

	/* Apply phy config only for KSZ9031 */
	if ((phydev->phy_id & 0xfffff0) != 0x221620)
	{
		if (phydev->drv->config)
			phydev->drv->config(phydev);

		return 0;
	}

	/*PHY Interface Control Register - bit0 phy interface select rgmii*/
	WriteRegWord(0xf5031f00, 0x01);

	/* control data pad skew - devaddr = 0x02, register = 0x04 */
	ksz9031_phy_extended_write(phydev, 0x02,
						MII_KSZ9031_EXT_RGMII_CTRL_SIG_SKEW,
						MII_KSZ9031_MOD_DATA_NO_POST_INC,
						SX7A_KSZ9031_CTRL_SKEW);
	/* tx data pad skew - devaddr = 0x02, register = 0x06 */
	ksz9031_phy_extended_write(phydev, 0x02,
						MII_KSZ9031_EXT_RGMII_TX_DATA_SKEW,
						MII_KSZ9031_MOD_DATA_NO_POST_INC,
						SX7A_KSZ9031_TX_SKEW);

	/* gtx and rx clock pad skew - devaddr = 0x02, register = 0x08 */
	ksz9031_phy_extended_write(phydev, 0x02,
						MII_KSZ9031_EXT_RGMII_CLOCK_SKEW,
						MII_KSZ9031_MOD_DATA_NO_POST_INC,
						SX7A_KSZ9031_CLK_SKEW);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
#else
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
#endif
}
