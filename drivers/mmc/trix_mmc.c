#include <common.h>
#include <malloc.h>
#include <sdhci.h>
#include <asm/io.h>
#include <asm/arch/reg_io.h>


static int sdh_pinshare_init(void)
{
#ifdef CONFIG_MACH_SIGMA_SX6

	/*
	 * bit[6:4] = 001	SDIO data 1
	 * bit[2:0] = 001	SDIO data 0
	 */
	MWriteRegByte(0x1500ee30, 0x11, 0x77);

	/*
	 * bit[6:4] = 001	SDIO data 3
	 * bit[2:0] = 001	SDIO data 2
	 */
	MWriteRegByte(0x1500ee31, 0x11, 0x77);

	/*
	 * bit[6:4] = 011	SDIO CD
	 */
	MWriteRegByte(0x1500ee20, 0x30, 0x70);

	/*
	 * bit[2:0] = 011	SDIO WP
	 */
	MWriteRegByte(0x1500ee21, 0x03, 0x07);

	/*
	 * bit[2:0] = 010	SDIO CLK
	 * bit[6:4] = 010	SDIO CMD
	 */
	MWriteRegByte(0x1500ee3b, 0x22, 0x77);
	
	/*
	 * bit[1] = 1	Select IO for function sdin of SDIO
	 */
	MWriteRegByte(0x1500e868, 0x02, 0x02);

	/*
	 * bit[4] = 1	Select IO for function sdin of SDIO
	 */
	MWriteRegByte(0x1500ee41, 0x10, 0x10);

#elif defined (CONFIG_MACH_SIGMA_SX7)
	MWriteRegByte(0x1500ee27, 0x10, 0x70);
	WriteRegByte(0x1500ee29, 0x11);
	WriteRegByte(0x1500ee2a, 0x11);
	WriteRegByte(0x1500ee2b, 0x11);
	WriteRegByte(0x1500ee2c, 0x11);
	MWriteRegByte(0x1500ee2d, 0x01, 0x03);

#elif defined (CONFIG_MACH_SIGMA_SX8)
	MWriteRegByte(0x1500ee2e, 0x11, 0x77);	//bit[6:4]data1,bit[2:0]data0
	MWriteRegByte(0x1500ee2f, 0x11, 0x77);	//bit[6:4]data3,bit[2:0]data2
	MWriteRegByte(0x1500ee30, 0x11, 0x77);	//bit[6:4]data5,bit[2:0]data4
	MWriteRegByte(0x1500ee31, 0x11, 0x77);	//bit[6:4]data7,bit[2:0]data6
	MWriteRegByte(0x1500ee32, 0x11, 0x77);	//bit[6:4]cmd,bit[2:0]clk
#else
	#error "unknown chip!!"
#endif
	return 0;
}

static char *SDHI_NAME = "sigma_sdhi";

int monza_sdhi_init(u32 regbase,  u32 quirks)
{
	struct sdhci_host *host = NULL;
	//u32 cap;
	//u32 max_clk;

	sdh_pinshare_init();

    host = (struct sdhci_host *)malloc(sizeof(struct sdhci_host));

    if (!host) {
		printf("sdh_host malloc fail!\n");
		return 1;
	}

	host->name   = SDHI_NAME;
	host->ioaddr = (void *)regbase;
	host->quirks = quirks;

	/* Host support high capacity */
//	host->host_caps = MMC_MODE_HC;
//	host->host_caps = MMC_MODE_DDR_52MHz;
	host->host_caps = MMC_MODE_HS_52MHz | MMC_MODE_HS | MMC_MODE_8BIT | MMC_MODE_DDR_52MHz;

	/* Host support UHS DDR mode */
//	host->host_caps |= MMC_MODE_UHS_DDR50;

#ifdef CONFIG_MMC_SDHCI_IO_ACCESSORS
	memset(&mv_ops, 0, sizeof(struct sdhci_ops));
	mv_ops.write_b = mv_sdhci_writeb;
	host->ops = &mv_ops;
#endif
	host->set_clock = NULL;
	host->set_control_reg = NULL;

	if (quirks & SDHCI_QUIRK_REG32_RW)
		host->version = sdhci_readl(host, SDHCI_HOST_VERSION - 2) >> 16;
	else
		host->version = sdhci_readw(host, SDHCI_HOST_VERSION);



	add_sdhci(host, 0, 0);

	return 0;
}
