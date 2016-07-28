/*
 * (C) Copyright 2007
 * Vlad Lungu vlad.lungu@windriver.com
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
//#define DEBUG
#include <common.h>
#include <command.h>
#include <asm/io.h>

//#include <linux/compiler.h>
#include <asm/arch/setup.h>
#include <asm/arch/xenvkeys.h>

#include <phy.h>

#ifdef CONFIG_PHY_MICREL
#	include <micrel.h>
#endif
#include <sdhci.h>

DECLARE_GLOBAL_DATA_PTR;

// the variable below is defined in the "arch/tango/lib/tangoxinit.c"
//extern unsigned long xenv_mem;
extern int xenv_isvalid(u32 *base, u32 maxsize);
extern int xenv_get(u32 *base, u32 size, char *recordname, void *dst, u32 *datasize);
extern int tangox_eth_initialize( int num_eth_port );
extern int tangox_sdhci_init(u32 regbase, u32 max_clk, u32 min_clk, u32 quirks);
extern void tangox_nand_export_mtdinfo(void);
extern int tangoxinit(void);
extern unsigned long tangox_get_cdclock(unsigned int cd);
extern unsigned long tangox_chip_id(void);
extern void tangox_mbus_init(void);

int dram_init(void)
{
    // TODO: if we have way to determine ram size dynamically,
    //       set it on the fly. (may be able to read from xenv?) 
    gd->ram_size = 1024 * 1024 * MEM_SIZE;
    return 0;
}

int board_init(void)
{
#if 0	
    //TODO: implement board init properly
    gd->bd->bi_arch_number = MACH_TYPE_TANGO_8734; /* just use it until we got own*/
    gd->bd->bi_boot_params = (0x80000000 + 0x100); /* boot param addr */
#endif
    return 0;
}


#if 0 ///////////////////////////////////////////////
int checkboard(void)
{
    //TODO:  let's print out board information.
    // we may place this funtion in the ./arch/arm/cpu/armv7/tango4/a_file.c
    // and in this file just set the board info string like others do. e.g. panda, harmoney

    puts("BOARD: Sigma Designs SMP8734\n");

	return 0;
}
#endif ///////////////////////////////////////////////

#if defined (CONFIG_ARCH_MISC_INIT)
int arch_misc_init(void)
{
	tangox_mbus_init();
    puts("MBUS:  initialized\n");

	return 0;
}
#endif

int board_early_init_r (void)
{
	tangoxinit();
	return 0;
}

static int get_mtdids( char* src, char* dest )
{
    if ( !strncmp( src, "mtdparts=", 9 ) )
    {
        char* separator = NULL;
        separator = strchr( src+9, ':');
        
        if ( separator )
        {
            strcpy( dest, "nand0=" );
            strncat( dest, src+9, (unsigned int)(separator - src - 9) );
            return 0;
        }
    }
    
    return -1;
}

int board_late_init(void)
{
    unsigned int xenv_base = 0;
    unsigned int lrrw_xenv_addr = 0;

    int ret      = 0;
    int sizex    = 0;

    int env_size = 0;
    int i        = 0;
    int records  = 0;
    char *base   = NULL;

    char read_buffer[512];
    char mtdids[64];
    unsigned int ipu0_status = 0;

    xenv_base = gbus_read_uint32(pgbus, REG_BASE_cpu_block + LR_ZBOOTXENV_LOCATION);

    if ( (env_size = xenv_isvalid((void *)xenv_base, MAX_XENV_SIZE)) < 0 ) 
        return 1;

    /* get mtd partition info */
    sizex = sizeof(read_buffer);
    memset( read_buffer, 0x00, sizeof(read_buffer) );
    ret = xenv_get( (void *)xenv_base, 
                    MAX_XENV_SIZE, 
                    XENV_KEY_MTDPARTS, 
                    &read_buffer[0], 
                    (void *)&sizex);

    debug( "%s: %s\n", XENV_KEY_MTDPARTS, read_buffer );

    if ( ret == RM_OK )
    {
        memset( mtdids, 0x00, sizeof(mtdids) );

        if ( !get_mtdids(read_buffer, mtdids) )
        {
            setenv( "mtdids", mtdids );
            setenv( "mtdparts", read_buffer );
        }
    }

    /* get block device partition info */
    sizex = sizeof(read_buffer);
    memset( read_buffer, 0x00, sizeof(read_buffer) );

    ret = xenv_get( (void *)xenv_base, 
                    MAX_XENV_SIZE, 
                    XENV_KEY_BLKPARTS, 
                    &read_buffer[0], 
                    (void *)&sizex);

    debug( "%s: %s\n", XENV_KEY_BLKPARTS, read_buffer );
    if ( ret == RM_OK )
    {
        setenv( "blkdevparts", read_buffer );
    }

    /* get kernel boot arguments */
    sizex = sizeof(read_buffer);
    memset( read_buffer, 0x00, sizeof(read_buffer) );

    ret = xenv_get( (void *)xenv_base, 
                    MAX_XENV_SIZE, 
                    XENV_KEY_LINUX_CMD, 
                    &read_buffer[0], 
                    (void *)&sizex);
    debug( "%s: %s\n", XENV_KEY_LINUX_CMD, read_buffer );

    if ( ret == RM_OK )
    {
        char *tmp;

        tmp = getenv( "mtdparts");
        if ( tmp )
        {
            strcat( read_buffer, " ");
            strcat( read_buffer, tmp );
        }

        tmp = getenv( "blkdevparts");
        if ( tmp )
        {
            strcat( read_buffer, " ");
            strcat( read_buffer, tmp );
        }
        setenv( "bootargs", read_buffer );            
    }
    
    /* export mtd partition info */    
    //tangox_nand_export_mtdinfo();

    /* 
     * read zxenv and find the u.xyz keys
     */

    /* skip header part */
    i    = 36; 
    base = (char *)xenv_base;

    while( i < env_size )
    {
        char rec_attr;
		unsigned short rec_size;
		char *recordname;
		unsigned long key_len;
		unsigned char *x;

		rec_attr   = ( base[i] >> 4 ) & 0xf;
		rec_size   = ( (base[i] & 0xf) << 8 ) + ( ((unsigned short)base[i+1]) & 0xff );
		recordname = base + i + 2;
		key_len    = strlen( recordname );
		x          = (unsigned char *)( recordname + key_len + 1 );

        if ( strncmp( recordname, "u.", 2) == 0 ) {

            debug("(0x%02x) %4ld %s ", rec_attr, (rec_size - 2 - (key_len+1)), recordname);

            /* copy key value (string) */
            memcpy( read_buffer, x, rec_size - 2 - (key_len + 1) );
            read_buffer[(rec_size - 2 - (key_len + 1))] = '\0';

            /* may need to validate key & key values?? not sure yet... */
            setenv( recordname+2, read_buffer );
            debug( "%s\n", read_buffer );

            records++;
        }
		i+=rec_size;
    }
    printf( "Env:   %d xenv keys are imported\n", records );

    /* get boot device info */
    lrrw_xenv_addr = REG_BASE_cpu_block + LR_XENV2_RW;

    if (xenv_isvalid((void *)lrrw_xenv_addr, MAX_LR_XENV2_RW) < 0)
    {
        printf("no valid lrrw xenv found\n");
    }
    else
    {
        sizex = sizeof(unsigned int);
        ret = xenv_get( (void *)lrrw_xenv_addr, 
                        MAX_LR_XENV2_RW, 
                        XENV_LRRW_IPU_STAGE0_STATUS, 
                        &ipu0_status, 
                        (void *)&sizex);

        if ( ret == RM_OK )
        {
            sprintf( read_buffer, "%s", (ipu0_status & 0x80000000) ? "emmc" : "nand" ); 
            setenv( "bootdev", read_buffer );
        }                
    }

    return 0;
}

static unsigned int get_enabled_devices( void )
{
    unsigned int xenv_base   = 0;
    unsigned int enabled_dev = 0;
    int ret   = 0;
    int sizex = 0;
    
    xenv_base = gbus_read_uint32(pgbus, REG_BASE_cpu_block + LR_ZBOOTXENV_LOCATION);
    
    if ( xenv_isvalid((void *)xenv_base, MAX_XENV_SIZE) >= 0 )
    {
        /* get enabled devices */
        sizex = 4;
        ret = xenv_get( (void *)xenv_base, 
                        MAX_XENV_SIZE, 
                        XENV_KEY_ENABLED_DEVICES, 
                        &enabled_dev, 
                        (void *)&sizex);

        debug( "a.enabled_devices: 0x%x\n", enabled_dev );

        if ( (ret != RM_OK) || (sizex != 4) )
				printf("\nWARNING: cannot read a.enabled_devices\n");
    }
    /* when xenv_get is fialed enabled_dev will have 0 */
    return enabled_dev;
}

static unsigned int check_available_eth( void )
{
    unsigned int available_ports = 0;
    unsigned int enabled_dev;
    
    enabled_dev = get_enabled_devices();

    if ( enabled_dev & (1<<ETHERNET_SHIFT) )
        available_ports |= 0x1;
    if ( enabled_dev & (1<<ETHERNET1_SHIFT) )
        available_ports |= 0x2;

    debug( "available ethernet bit map: 0x%x\n", available_ports);
   
    return available_ports;
}

static int check_mac_address( int eth_port, unsigned char* mac_addr_ck )
{
    unsigned long mac_hi, mac_lo, sizex = 4, lrrw_xenv_addr;
    unsigned long i, cksum = 0;
    const unsigned char marker = 'M';
    //unsigned char mac_addr_ck[8];
    unsigned char *addr = mac_addr_ck;
    int ret;

    lrrw_xenv_addr = KSEG1ADDR(REG_BASE_cpu_block + LR_XENV2_RW);
    if (xenv_isvalid((void *)lrrw_xenv_addr, MAX_LR_XENV2_RW) < 0)
    {
        printf("check_mac_address: no valid lrrw xenv found\n");
        return -1;
    }

	// PORT 0
    if ( eth_port == 0)
    { 
        ret = xenv_get( (void *)lrrw_xenv_addr, 
                        MAX_LR_XENV2_RW, 
                        XENV_LRRW_ETH_MACL, 
                        &mac_lo, 
                        (void *)&sizex );

        if ( (ret != RM_OK) || (sizex != 4) )
            return -1;

        ret = xenv_get( (void *)lrrw_xenv_addr, 
                        MAX_LR_XENV2_RW, 
                        XENV_LRRW_ETH_MACH, 
                        &mac_hi, 
                        (void *)&sizex );

        if ( (ret!= RM_OK) || (sizex != 4) )
            return -1;
    }
    else
    {
		// PORT 1
        ret = xenv_get( (void *)lrrw_xenv_addr, 
                        MAX_LR_XENV2_RW, 
                        XENV_LRRW_ETH1_MACL, 
                        &mac_lo, 
                        (void *)&sizex);

        if ( (ret != RM_OK) || (sizex != 4) )
            return -1;

        ret = xenv_get( (void *)lrrw_xenv_addr, 
                        MAX_LR_XENV2_RW, 
                        XENV_LRRW_ETH1_MACH, 
                        &mac_hi, 
                        (void *)&sizex);

        if (( ret != RM_OK) || (sizex != 4))
            return -1;
    }

    debug( "mac_lo: 0x%x\n", (unsigned int)mac_lo );
    debug( "mac_hi: 0x%x\n", (unsigned int)mac_hi );

    if(((mac_hi >> 16) & 0xff) != marker)
        return -2;

    *addr++ = (mac_hi & 0x0000ff00) >> 8;
    *addr++ = (mac_hi & 0x000000ff);
    *addr++ = (mac_lo & 0xff000000) >> 24;
    *addr++ = (mac_lo & 0x00ff0000) >> 16;
    *addr++ = (mac_lo & 0x0000ff00) >> 8;
    *addr   = (mac_lo & 0x000000ff);

    for(i=0;i<6;i++)
        cksum += mac_addr_ck[i];

    if(((mac_hi >> 24) & 0xff) != (cksum & 0xff) )
        return -4;

     return 0;
}


/*
 * board level eth device init
 */ 

int board_eth_init(bd_t *bi)
{
    int ret;
    int eth_port = 0;
    int eth_available_ports = 0;
    char cmd_buf[32];
    char mac_buf[32];
    unsigned char mac_addr_ck[8];

    eth_available_ports = check_available_eth();

    if ( eth_available_ports & 0x1 )
    {
        eth_port = 0;
        ret = check_mac_address( eth_port, mac_addr_ck );

        if ( !ret )
        {
            /* set mac address */
            debug("set MAC address...\n");

            sprintf( cmd_buf, "ethaddr" );
            sprintf( mac_buf, "%02x:%02x:%02x:%02x:%02x:%02x", 
                     mac_addr_ck[0],
                     mac_addr_ck[1],
                     mac_addr_ck[2],
                     mac_addr_ck[3],
                     mac_addr_ck[4],
                     mac_addr_ck[5] );

            debug(" cmd: %s\n", cmd_buf );
            debug(" mac: %s\n", mac_buf );

            setenv( cmd_buf, mac_buf ); 
        }
        else
        {
            printf("Fail to read MAC%d (err: %d)\n", eth_port, ret );
        }
    }

    if ( eth_available_ports & 0x2 )
    {
        eth_port = 1;
        ret = check_mac_address( eth_port, mac_addr_ck );

        if ( !ret )
        {
            /* set mac address */
            debug("set MAC address...\n");

            sprintf( cmd_buf, "eth%daddr", eth_port );
            sprintf( mac_buf, "%02x:%02x:%02x:%02x:%02x:%02x", 
                     mac_addr_ck[0],
                     mac_addr_ck[1],
                     mac_addr_ck[2],
                     mac_addr_ck[3],
                     mac_addr_ck[4],
                     mac_addr_ck[5] );

            debug(" cmd: %s\n", cmd_buf );
            debug(" mac: %s\n", mac_buf );
            setenv( cmd_buf, mac_buf ); 
        }
        else
        {
            printf("Fail to read MAC%d (err: %d)\n", eth_port, ret );
        }
    }

    /* initialize ethernet */
    if ( eth_available_ports & 0x1 )
        tangox_eth_initialize( 0 );

    if ( eth_available_ports & 0x2 )
        tangox_eth_initialize( 1 );

	return 0;
}

#if defined (CONFIG_TANGOX_MMC)

#define TANGOX_SDIO_BASE_ADDR0      (REG_BASE_host_interface + 0x1000)
#define TANGOX_SDIO_BASE_ADDR1      (REG_BASE_host_interface + 0x1200)

static void tangox_sdhci_8734_preinit( unsigned int enabled_ports )
{
    int fec_enable = 1;
    unsigned int infreq = (tangox_get_cdclock(6) / 2000000) & 0xff; /* in MHz */
    
    if ( enabled_ports & 0x01 )
    {
        /* the following routine is specific for 8734 */
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR0 + 0x100, 0x00000008); /* select SDIO from Eth1 pad */
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR0 + 0x140, 0x247f8074 | (infreq << 7)); /* set up CAP_IN */
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR0 + 0x144, 0x000c0008);
        /* ??? */
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR0 + 0x128, 0x0004022c);
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR0 + 0x12c, 0x00000002);
        
        if (fec_enable )
        { 
            /*by default, there is 2ns hold delay
              setting bit 21 and 22, inverts the clock but setup time is violated (too much hold delay)
              set bit 21,22 to zero, 23(delay denable) to 1 and 27.24 to F(max delay) to add another 2ns 
            */
            gbus_write_reg32(TANGOX_SDIO_BASE_ADDR0 + 0x150, 0x0F800000); /* falling edge clocking */
        }
        else
        {
            gbus_write_reg32(TANGOX_SDIO_BASE_ADDR0 + 0x150, 0x00000000);
        }
    }

    if ( enabled_ports & 0x02 )
    {
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR1 + 0x100, 0x00000008); /* select SDIO from Eth1 pad */
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR1 + 0x140, 0x247f8074 | (infreq << 7)); /* set up CAP_IN */
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR1 + 0x144, 0x000c0008);
        /* ??? */
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR1 + 0x128, 0x0004022c);
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR1 + 0x12c, 0x00000002);
     
        if (fec_enable )
        {
			//need enable/disable both 21 and 22 bits for 8734
            gbus_write_reg32(TANGOX_SDIO_BASE_ADDR1 + 0x150, 0x0F800000); /* falling edge clocking */
        }
        else
        {
            gbus_write_reg32(TANGOX_SDIO_BASE_ADDR1 + 0x150, 0x00000000);
        }
    }

    udelay(10); /* just to be sure ... */    
}

static void tangox_sdhci_8756_preinit( unsigned int enabled_ports )
{
    int fec_enable = 1;
    unsigned int infreq = (tangox_get_cdclock(6) / 2000000) & 0xff; /* in MHz */
    
    if ( enabled_ports & 0x01 )
    {
        /* the following routine is specific for 8734 */
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR0 + 0x100, 0x00000008); /* select SDIO from Eth1 pad */
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR0 + 0x140, 0x247f8074 | (infreq << 7)); /* set up CAP_IN */
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR0 + 0x144, 0x000c0008);
        /* ??? */
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR0 + 0x128, 0x0004022c);
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR0 + 0x12c, 0x00000002);
        
        if (fec_enable )
        { 
            /*by default, there is 2ns hold delay
              setting bit 21 and 22, inverts the clock but setup time is violated (too much hold delay)
              set bit 21,22 to zero, 23(delay denable) to 1 and 27.24 to F(max delay) to add another 2ns 
            */
            gbus_write_reg32(TANGOX_SDIO_BASE_ADDR0 + 0x150, 0x00600000); /* falling edge clocking */
        }
        else
        {
            gbus_write_reg32(TANGOX_SDIO_BASE_ADDR0 + 0x150, 0x00000000);
        }
    }

    if ( enabled_ports & 0x02 )
    {
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR1 + 0x100, 0x00000008); /* select SDIO from Eth1 pad */
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR1 + 0x140, 0x247f8074 | (infreq << 7)); /* set up CAP_IN */
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR1 + 0x144, 0x000c0008);
        /* ??? */
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR1 + 0x128, 0x0004022c);
        gbus_write_reg32(TANGOX_SDIO_BASE_ADDR1 + 0x12c, 0x00000002);
     
        if (fec_enable )
        {
			//need enable/disable both 21 and 22 bits for 8734
            gbus_write_reg32(TANGOX_SDIO_BASE_ADDR1 + 0x150, 0x00600000); /* falling edge clocking */
        }
        else
        {
            gbus_write_reg32(TANGOX_SDIO_BASE_ADDR1 + 0x150, 0x00000000);
        }
    }

    udelay(10); /* just to be sure ... */    
}


unsigned int check_enabled_mmc( bd_t* bi )
{
    unsigned int available_ports = 0;
    unsigned int enabled_dev    = 0;

    enabled_dev = get_enabled_devices();
      
    /* only one of the device may be used. in that case we should return 
     * the largest index of the devices to make u-boot work correctly */
    if ( enabled_dev & (1<<SDIO_SHIFT) )
        available_ports |= 0x1;
    if ( enabled_dev & (1<<SDIO1_SHIFT) )
        available_ports |= 0x2;

    debug( "available mmc bit map: 0x%x\n", available_ports);

    return available_ports;
}

int board_mmc_init(bd_t *bd)
{
    int i = 0;
    unsigned int mmc_enabled_ports;
    ulong mmc_base_address[] = { TANGOX_SDIO_BASE_ADDR0, TANGOX_SDIO_BASE_ADDR1 };
    unsigned long chip_id = (tangox_chip_id() >> 16) & 0xfffe;

    /* check enabled device before init */
    mmc_enabled_ports = check_enabled_mmc( bd );

    switch( chip_id )
    {
        case 0x8734:
		case 0x2400:
            tangox_sdhci_8734_preinit( mmc_enabled_ports );		
            break;
        case 0x8756:
        case 0x8758:
            tangox_sdhci_8756_preinit( mmc_enabled_ports );
            break;
        default:
            puts("mmc preinit - no matching chip id\n");
            break;
    }
        
    /* mmc port 0 */
    if ( mmc_enabled_ports & 0x01 )
    {
        if ( tangox_sdhci_init(mmc_base_address[i], 0, 0,
				SDHCI_QUIRK_32BIT_DMA_ADDR  ) )
            puts("fail to init (mmc0)\n");
        i++;
    }

    /* mmc port 1 */
    if ( mmc_enabled_ports & 0x02 )
    {
        if ( tangox_sdhci_init(mmc_base_address[i], 0, 0,
				SDHCI_QUIRK_32BIT_DMA_ADDR  ) )
			puts("fail to init (mmc1)\n");
    }

    if( mmc_enabled_ports == 0 )
        puts("none\n");

	return 0;
}
#endif


static int ar8035_phy_fixup(struct phy_device *phydev)
{
#define AR8031_MII_REG_LED_CONTROL				0x18
#define AR8031_MII_REG_DEBUG_PORT_ADDR_OFFSET	0x1D
#define AR8031_MII_REG_DEBUG_PORT_DATA			0x1E

	int val = 0;

	if (phydev == NULL)
		return -1;

	if ((phydev->phy_id < 0) || (phydev->addr < 0))
		return -1;

	/* Apply phy config only for AR8035 */
	if (phydev->phy_id != 0x4dd072)
		return -1;

	/* enable regmii tx clock delay */
	phy_write(phydev, MDIO_DEVAD_NONE, AR8031_MII_REG_DEBUG_PORT_ADDR_OFFSET, 0x5);
	val = phy_read(phydev, MDIO_DEVAD_NONE, AR8031_MII_REG_DEBUG_PORT_DATA);
	val |= (1 << 8);
	phy_write(phydev, MDIO_DEVAD_NONE, AR8031_MII_REG_DEBUG_PORT_DATA, val);

	phy_write(phydev, MDIO_DEVAD_NONE, AR8031_MII_REG_DEBUG_PORT_ADDR_OFFSET, 0xB);
	phy_write(phydev, MDIO_DEVAD_NONE, AR8031_MII_REG_DEBUG_PORT_DATA, 0xbc20);

	/* LED control */
	phy_write(phydev, MDIO_DEVAD_NONE, AR8031_MII_REG_LED_CONTROL, 0x2100);

	/*check phy power */
	val = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMCR);
	if (val & BMCR_PDOWN)
		phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, (val & ~BMCR_PDOWN));

	return 0;
}

static int ksz9031_phy_fixup(struct phy_device *phydev)
{
#define MBA6X_KSZ9031_CTRL_SKEW 0x0032
#define MBA6X_KSZ9031_CLK_SKEW  0x0273

#define MBA6X_KSZ9031_RX_SKEW   0x3333
#define MBA6X_KSZ9031_TX_SKEW   0x2036

#define MII_KSZ9031_EXT_FLP_BURST_TRANSMIT_LO	0x3
#define MII_KSZ9031_EXT_FLP_BURST_TRANSMIT_HI	0x4

	if (phydev == NULL)
		return -1;

	if ((phydev->phy_id < 0) || (phydev->addr < 0))
		return -1;

	/* Apply phy config only for KSZ9031 */
	if ((phydev->phy_id & 0xfffff0) != 0x221620)
		return -1;

	/*
	 * Change FLP
	 * AN FLP Burst Transmit HI - devaddr = 0x00, register = 0x04
	 * AN FLP Burst Transmit LO - devaddr = 0x00, register = 0x03
	 */
	ksz9031_phy_extended_write(phydev, 0x00,
								MII_KSZ9031_EXT_FLP_BURST_TRANSMIT_HI,
								MII_KSZ9031_MOD_DATA_NO_POST_INC,
								0x0006);
	ksz9031_phy_extended_write(phydev, 0x00,
								MII_KSZ9031_EXT_FLP_BURST_TRANSMIT_LO,
								MII_KSZ9031_MOD_DATA_NO_POST_INC,
								0x1A80);

	/*
	 * min rx/tx ctrl delay
	 * control data pad skew - devaddr = 0x02, register = 0x04
	 */
	ksz9031_phy_extended_write(phydev, 0x02,
								MII_KSZ9031_EXT_RGMII_CTRL_SIG_SKEW,
								MII_KSZ9031_MOD_DATA_NO_POST_INC,
								MBA6X_KSZ9031_CTRL_SKEW);
	/*
	 * min rx delay
	 * rx data pad skew - devaddr = 0x02, register = 0x05
	 */
	ksz9031_phy_extended_write(phydev, 0x02,
								MII_KSZ9031_EXT_RGMII_RX_DATA_SKEW,
								MII_KSZ9031_MOD_DATA_NO_POST_INC,
								MBA6X_KSZ9031_RX_SKEW);
	/*
	 * max tx delay
	 * tx data pad skew - devaddr = 0x02, register = 0x05
	 */
	ksz9031_phy_extended_write(phydev, 0x02,
								MII_KSZ9031_EXT_RGMII_TX_DATA_SKEW,
								MII_KSZ9031_MOD_DATA_NO_POST_INC,
								MBA6X_KSZ9031_TX_SKEW);

	/*
	 * rx/tx clk skew
	 * gtx and rx clock pad skew - devaddr = 0x02, register = 0x08
	 */
	ksz9031_phy_extended_write(phydev, 0x02,
								MII_KSZ9031_EXT_RGMII_CLOCK_SKEW,
								MII_KSZ9031_MOD_DATA_NO_POST_INC,
								MBA6X_KSZ9031_CLK_SKEW);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	/*
	 * Gigabit not working properly.
	 * Remove 1000T speed from advertisement
	 */
	phy_write(phydev, MDIO_DEVAD_NONE, MII_CTRL1000, 0x1C00);

	return 0;
}


int board_phy_config(struct phy_device *phydev)
{
	/* AR8035 */
	if (ar8035_phy_fixup(phydev) == 0)
	{
		if (phydev->drv->config)
			phydev->drv->config(phydev);

		return 0;
	}

	/* KSZ9031 */
	if (ksz9031_phy_fixup(phydev) == 0)
	{
		if (phydev->drv->config)
			phydev->drv->config(phydev);

		phy_write(phydev, MDIO_DEVAD_NONE, MII_CTRL1000, 0x1C00);

		return 0;
	}

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}
