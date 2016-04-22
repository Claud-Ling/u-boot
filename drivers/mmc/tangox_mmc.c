#include <common.h>
#include <malloc.h>

#include <asm/io.h>
#include <asm/errno.h>
#include <linux/compiler.h>

#include <asm/arch/setup.h>

#include <sdhci.h>
#include <mmc.h>

//#define DEBUG

#ifdef CONFIG_MMC_SDHCI_IO_ACCESSORS
static struct sdhci_ops tangox_sdhci_ops;

static inline u32 tangox_sdhci_readl(struct sdhci_host *host, int reg)
{
#ifdef DEBUG    
    u32 ret;
    ret = gbus_read_uint32( pgbus, host->ioaddr + reg );
    printf("rl...(addr:0x%x, val: 0x%x)\n", host->ioaddr+reg, ret );
    return ret;
#else
    return gbus_read_uint32( pgbus, (unsigned int)(host->ioaddr + reg) );
#endif    
}

static inline u16 tangox_sdhci_readw(struct sdhci_host *host, int reg)
{
#ifdef DEBUG
    u16 ret;
    ret = gbus_read_uint16( pgbus, host->ioaddr + reg );
    printf("rw...(addr:0x%x, val: 0x%x)\n", host->ioaddr+reg, ret );
    return ret;
#else
    return gbus_read_uint16( pgbus, (unsigned int)(host->ioaddr + reg) );
#endif    
}

static inline u8 tangox_sdhci_readb(struct sdhci_host *host, int reg)
{
#ifdef DEBUG
    u8 ret;
    ret = gbus_read_uint8( pgbus, host->ioaddr + reg );
    printf("rb...(addr:0x%x, val: 0x%x)\n", host->ioaddr+reg, ret );
    return ret;
#else    
    return gbus_read_uint8( pgbus, (unsigned int)(host->ioaddr + reg) );
#endif    
}

static inline void tangox_sdhci_writel(struct sdhci_host *host, u32 val, int reg)
{
#ifdef DEBUG    
    printf("wl...(addr:0x%x, val: 0x%x)\n", host->ioaddr+reg, val );
#endif    
    gbus_write_uint32( pgbus, (unsigned int)(host->ioaddr) + reg, val );
}

static inline void tangox_sdhci_writew(struct sdhci_host *host, u16 val, int reg)
{
#ifdef DEBUG        
    printf("ww...(addr:0x%x, val: 0x%x)\n", host->ioaddr+reg, val );
#endif    
    gbus_write_uint16( pgbus, (unsigned int)(host->ioaddr + reg), val );
}

static inline void tangox_sdhci_writeb(struct sdhci_host *host, u8 val, int reg)
{
#ifdef DEBUG        
    printf("wb...(addr:0x%x, val: 0x%x)\n", host->ioaddr+reg, val );
#endif    
    gbus_write_uint8( pgbus, (unsigned int)(host->ioaddr + reg), val );
}

#endif /* CONFIG_MMC_SDHCI_IO_ACCESSORS */


int tangox_sdhci_init(u32 regbase, u32 max_clk, u32 min_clk, u32 quirks)
{
    struct sdhci_host *host = NULL;
    
    //tangox_sdhci_preinit( regbase );
    	
    host = (struct sdhci_host *)malloc(sizeof(struct sdhci_host));
	
    if (!host) {
		printf("sdh_host malloc fail!\n");
		return 1;
	}

	host->name = "tangox-sdhci";
	host->ioaddr = (void *)regbase;
	host->quirks = quirks;
    //host->host_caps = MMC_MODE_HC;

#ifdef CONFIG_MMC_SDHCI_IO_ACCESSORS
	memset(&tangox_sdhci_ops, 0, sizeof(struct sdhci_ops));

	tangox_sdhci_ops.read_l  = tangox_sdhci_readl;
    tangox_sdhci_ops.read_w  = tangox_sdhci_readw;
    tangox_sdhci_ops.read_b  = tangox_sdhci_readb;
    
    tangox_sdhci_ops.write_l = tangox_sdhci_writel;
    tangox_sdhci_ops.write_w = tangox_sdhci_writew;
    tangox_sdhci_ops.write_b = tangox_sdhci_writeb;
	
    host->ops = &tangox_sdhci_ops;
#endif

    // pad set??? don't know... but this register should be set.
    //sdhci_writel(host, 0x00000008, 0x0100);

	if (quirks & SDHCI_QUIRK_REG32_RW)
		host->version = sdhci_readl(host, SDHCI_HOST_VERSION - 2) >> 16;
	else
		host->version = sdhci_readw(host, SDHCI_HOST_VERSION);
        
    // sdhci.c file use host controller version without masking or shifting
    // so we need to masking the controller verion at here    
    //host->version = (host->version & SDHCI_SPEC_VER_MASK);
        
    debug( "host ver: 0x%x\n", host->version );
	
    add_sdhci(host, max_clk, min_clk);
   
	return 0;
}
