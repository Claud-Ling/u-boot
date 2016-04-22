/*
 * (C) Copyright 2013
 * Sigma Designs inc.
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

#include <common.h>
#include <miiphy.h>
#include <malloc.h>
#include <linux/err.h>
#include <asm/io.h>
#include <linux/mii.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <linux/compiler.h>
#include <asm/arch/setup.h>
#include "tangox_eth.h"

DECLARE_GLOBAL_DATA_PTR;

//#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define eth_debug(fmt,...) printf(fmt,##__VA_ARGS__)
#else
#define eth_debug(fmt,...)
static void dump_packet( char* data, int size ) {}
static void dump_registers(struct eth_device *dev) {}
#endif

#if !defined(CONFIG_PHYLIB)
# error "SigmaDesigns Ether MAC requires PHYLIB - missing CONFIG_PHYLIB"
#endif

#define MAX_RECEIVE_BYTE        1534

/* Number of descriptors and buffer size */
#define R_BUF_SIZE              0x700	/* needs to be < 2KB */
#define T_BUF_SIZE              0x700	/* needs to be < 2KB */

#define ETH_ALEN                6       /* Octets in one ethernet addr   */
#define ETH_HLEN                14      /* Total octets in header.   */
#define ETH_ZLEN                60      /* Min. octets in frame sans FCS */
#define ETH_FRAME_LEN           1514    /* Max. octets in frame sans FCS */

#define MAX_MDC_CLOCK	        2500000

static int tangox_desc_init(struct eth_device *dev);
static int eth_mdio_read(struct tangox_priv *priv, u8 addr, u8 reg, u16 *val);
static int eth_mdio_write(struct tangox_priv *priv, u8 addr, u8 reg, u16 val);

#ifdef ENABLE_DEBUG
static void dump_packet( char* data, int size )
{
    int i = 0;

    for( i = 0; i < size; i++ )
    {
        if( i%16 == 0 )
            printf("\n");
        printf( "0x%02x ", *(data+i) & 0xff );
    }
    printf("\n");
}

static void dump_tx_descriptor(struct eth_device *dev)
{
	struct tangox_priv *priv = dev->priv;
	struct tangox_desc* desc = NULL;
	int i = 0;

	printf("\nTX Descriptor\n");
    
    /* invalidate cache to read report */
    invalidate_dcache_range( (unsigned long)&priv->tx_desc[0],
                             (unsigned long)&priv->tx_desc[0] + sizeof(struct tangox_desc) * DESCRIPTOR_COUNT);
    
	for(i=0; i< DESCRIPTOR_COUNT; i++)
	{
		desc = (struct tangox_desc*)&priv->tx_desc[i];
		printf("[%02d]\n", i);
		printf("\t Buffer Address          : 0x%08x\n", desc->starting_address);
		printf("\t Next Descriptor Address : 0x%08x\n", (u32)desc->next);
		printf("\t Report                  : 0x%08x(0x%08x)\n", desc->report, desc->txrx_report);
		printf("\t Configuration           : 0x%08x\n", (u32)desc->control);
	}

	printf("\n");
}

static void dump_rx_descriptor(struct eth_device *dev)
{
	struct tangox_priv *priv = dev->priv;
	struct tangox_desc* desc = NULL;
	int i = 0;

	printf("\nRX Descriptor\n");
    
    /* invalidate cache to read report */
    invalidate_dcache_range( (unsigned long)&priv->rx_desc[0],
                             (unsigned long)&priv->rx_desc[0] + sizeof(struct tangox_desc) * DESCRIPTOR_COUNT);

	for(i=0; i< DESCRIPTOR_COUNT; i++)
	{
		desc = (struct tangox_desc*)&priv->rx_desc[i];
		printf("[%02d]\n", i);
		printf("\t Buffer Address          : 0x%08x\n", desc->starting_address);
		printf("\t Next Descriptor Address : 0x%08x\n", (u32)desc->next);
		printf("\t Report                  : 0x%08x(0x%08x)\n", desc->report, desc->txrx_report);
		printf("\t Configuration           : 0x%08x\n", (u32)desc->control);
	}

	printf("\n");
}

static void dump_registers(struct eth_device *dev)
{
	struct tangox_priv *priv = dev->priv;

	/* MAC Core */
	printf("\nMAC Core\n");
	printf("\t Transmit Control 1           : 0x%02x\n", gbus_read_reg8(ENET_TX_CTL1(priv->mac_core_num)));
	printf("\t Transmit Control 2           : 0x%02x\n", gbus_read_reg8(ENET_TX_CTL2(priv->mac_core_num)));
	printf("\t Receive Control              : 0x%02x\n", gbus_read_reg8(ENET_RX_CTL(priv->mac_core_num)));
	printf("\t Random Seed                  : 0x%02x\n", gbus_read_reg8(ENET_RANDOM_SEED(priv->mac_core_num)));
	printf("\t TX Single Deferral Param     : 0x%02x\n", gbus_read_reg8(ENET_TX_SDP(priv->mac_core_num)));
	printf("\t TX Two Part Deferral Param 1 : 0x%02x\n", gbus_read_reg8(ENET_TX_TPDP1(priv->mac_core_num)));
	printf("\t TX Two Part Deferral Param 2 : 0x%02x\n", gbus_read_reg8(ENET_TX_TPDP2(priv->mac_core_num)));
	printf("\t SLOT Time                    : 0x%02x\n", gbus_read_reg8(ENET_SLOT_TIME(priv->mac_core_num)));
	printf("\t Buffer Size for Transmit     : 0x%02x\n", gbus_read_reg8(ENET_TX_BUFSIZE(priv->mac_core_num)));
	printf("\t FIFO Control                 : 0x%02x\n", gbus_read_reg8(ENET_FIFO_CTL(priv->mac_core_num)));
	printf("\t PAD Mode                     : 0x%02x\n", gbus_read_reg8(ENET_PAD_MODE(priv->mac_core_num)));
	printf("\t MDIO Clock Divider           : 0x%02x\n", gbus_read_reg8(ENET_MDIO_CLK(priv->mac_core_num)));

	printf("\nMAC TX DMA\n");
	printf("\t TX Channel Control           : 0x%08x\n", gbus_read_reg32(ENET_TXC_CR(priv->mac_core_num)));
	printf("\t TX Channel Status            : 0x%08x\n", gbus_read_reg32(ENET_TXC_SR(priv->mac_core_num)));
	printf("\t TX Descriptor Start Address  : 0x%08x\n", gbus_read_reg32(ENET_TX_DESC_ADDR(priv->mac_core_num)));
	printf("\t TX Channel FIFO Status       : 0x%08x\n", gbus_read_reg32(ENET_TX_FIFO_SR(priv->mac_core_num)));

	dump_tx_descriptor(dev);

	printf("\nMAC RX DMA\n");
	printf("\t RX Channel Control           : 0x%08x\n", gbus_read_reg32(ENET_RXC_CR(priv->mac_core_num)));
	printf("\t RX Channel Status            : 0x%08x\n", gbus_read_reg32(ENET_RXC_SR(priv->mac_core_num)));
	printf("\t RX Descriptor Start Address  : 0x%08x\n", gbus_read_reg32(ENET_RX_DESC_ADDR(priv->mac_core_num)));
	printf("\t RX Channel FIFO Status       : 0x%08x\n", gbus_read_reg32(ENET_RX_FIFO_SR(priv->mac_core_num)));

	dump_rx_descriptor(dev);

	return;
}
#endif

static int eth_mdio_read(struct tangox_priv *priv, u8 addr, u8 reg, u16 *val)
{
    unsigned long mdio_cmd;
    u16 data = 0;

	/* assemble read command */
    mdio_cmd = MIIAR_ADDR(addr) | MIIAR_REG(reg);

    /* write the command */
    gbus_write_reg32(ENET_MDIO_CMD1(priv->mac_core_num), mdio_cmd);
    mdelay(10);

    tangox_set_reg(ENET_MDIO_CMD1(priv->mac_core_num), MDIO_CMD_GO);

    while(gbus_read_reg32(ENET_MDIO_CMD1(priv->mac_core_num)) & MDIO_CMD_GO)
    {
		mdelay(1);
    }

    /* read data from PHY. 16 bits of the least significat bit on status reg
       keeps the data from PHY */
    data = gbus_read_reg32(ENET_MDIO_STS1(priv->mac_core_num)) & 0x0000FFFF;

	if(val != NULL)
		*val = data;

    return 0;
}

static int eth_mdio_write(struct tangox_priv *priv, u8 addr, u8 reg, u16 val)
{
	unsigned long mdio_cmd;

	mdio_cmd = MIIAR_ADDR(addr) | MIIAR_REG(reg) | MDIO_CMD_WR | MIIAR_DATA(val);

    gbus_write_reg32(ENET_MDIO_CMD1(priv->mac_core_num), mdio_cmd);
    mdelay(10);

    tangox_set_reg(ENET_MDIO_CMD1(priv->mac_core_num), MDIO_CMD_GO);

	while(gbus_read_reg32(ENET_MDIO_CMD1(priv->mac_core_num)) & MDIO_CMD_GO)
    {
		mdelay(1);
    }

	return 0;
}

static int tangox_mdiobus_read(struct mii_dev *bus, int phy_addr, int dev_addr, int regnum)
{
	struct tangox_priv *priv = (struct tangox_priv *)bus->priv;
	u16 val = 0;

	eth_mdio_read(priv, (u8)(phy_addr & 0xFF), (u8)(regnum & 0xFF), &val);

	return (int)val;
}

static int tangox_mdiobus_write(struct mii_dev *bus, int phy_addr, int dev_addr, int regnum, u16 value)
{
	struct tangox_priv *priv = (struct tangox_priv *)bus->priv;

	return eth_mdio_write(priv, (u8)(phy_addr & 0xFF), (u8)(regnum & 0xFF), (u16)(value & 0xFFFF));
}

/*
 *	Ethernet MAC Config
 *		- Base on Link speed and Link intarface(MII, RGMII)
 *		- forcemode : for init
 *			0 : reconfig mode(PHY Attached)
 *			1 : 10 / 100 T mode(MII)
 *			2 : 1000 mode(RGMII)
 */
static void tangox_mac_config(struct tangox_priv *priv, int forcemode, bool speedchanged, bool duplexchanged)
{
	int val, valpad;

	if (forcemode == 1) {
		val = gbus_read_reg8(ENET_MAC_MODE(priv->mac_core_num));
		if (val & (GMAC_MODE | RGMII_MODE)) {
			val &= ~(GMAC_MODE | RGMII_MODE);	/*disable Gigabit mode for now*/
			//val |= /*LB_EN |*/ BST_EN;		/*loopback off, burst on*/
			gbus_write_reg8(ENET_MAC_MODE(priv->mac_core_num), val);

			/* set pad_mode according Mac core interface */
			valpad = gbus_read_reg8(priv->mac_core_num + 0x400) & 0xf0;
			gbus_write_reg8(priv->mac_core_num + 0x400, valpad);

		}

		return;
	}
	else if (forcemode == 2) {
		val = gbus_read_reg8(ENET_MAC_MODE(priv->mac_core_num));
		if (((val & RGMII_MODE) == 0) || ((val & GMAC_MODE) == 0)) {
			val |= (GMAC_MODE | RGMII_MODE);
			gbus_write_reg8(ENET_MAC_MODE(priv->mac_core_num), val);

			/* set pad_mode according Mac core interface */
			valpad = gbus_read_reg8(ENET_PAD_MODE(priv->mac_core_num)) & 0xf0;
			gbus_write_reg8(ENET_PAD_MODE(priv->mac_core_num), valpad | 0x01);
		}

		return;
	}
	else if (forcemode == 0) {
		int speed = 0;
		int duplex = 0;

		/* Check Phy Link status */
		if (priv->phydev->link == 0) {
			printf("%s : link is down\n", priv->name);
			return;
		}

		/* Check Phy Link speed */
		speed = priv->phydev->speed;
		duplex = priv->phydev->duplex;

		switch(speed) {
			case SPEED_10:
			case SPEED_100:
			{
				val = gbus_read_reg8(ENET_MAC_MODE(priv->mac_core_num));
				if (speedchanged == true) {
					if ((val & RGMII_MODE) || (val & GMAC_MODE)) {
						val &= ~(GMAC_MODE | RGMII_MODE);
					}
				}

				if (duplexchanged == true) {
					if (duplex == DUPLEX_FULL)
						val &= ~HALF_DUPLEX;
					else if (duplex == DUPLEX_HALF)
						val |= HALF_DUPLEX;
				}

				if ((speedchanged == true) || (duplexchanged == true)) {
					gbus_write_reg8(ENET_MAC_MODE(priv->mac_core_num), val);

					/*set threshold for internal clock 0x1*/
					gbus_write_reg8(ENET_IC_THRESHOLD(priv->mac_core_num), 0x01);

					/*set slot time 0x7f for 10/100Mbps*/
					gbus_write_reg8(ENET_SLOT_TIME(priv->mac_core_num), 0x7f);

					if (priv->phy_interface == PHY_INTERFACE_MODE_RGMII) {
						/* Gigabit Mode : RGMII */
						val = gbus_read_reg8(ENET_PAD_MODE(priv->mac_core_num)) & 0xf0;
						gbus_write_reg8(ENET_PAD_MODE(priv->mac_core_num), val | 0x01);
					}
				}

				break;
			}

			case SPEED_1000:
			{
				val = gbus_read_reg8(ENET_MAC_MODE(priv->mac_core_num));
				if (speedchanged == true) {
					if (((val & RGMII_MODE) == 0) || ((val & GMAC_MODE) == 0)) {
						val |= (GMAC_MODE | RGMII_MODE);
					}
				}

				if (duplexchanged == true) {
					if (duplex == DUPLEX_FULL)
						val &= ~HALF_DUPLEX;
					else if (duplex == DUPLEX_HALF)
						val |= HALF_DUPLEX;
				}

				if ((speedchanged == true) || (duplexchanged == true)) {
					gbus_write_reg8(ENET_MAC_MODE(priv->mac_core_num), val);

					/*set threshold for internal clock 0x1*/
					gbus_write_reg8(ENET_IC_THRESHOLD(priv->mac_core_num), 0x03);

					/*set slot time 0x7f for 10/100Mbps*/
					gbus_write_reg8(ENET_SLOT_TIME(priv->mac_core_num), 0xff);

					if (priv->phy_interface == PHY_INTERFACE_MODE_MII) {
						/* 10/100T Mode : MII */
						val = gbus_read_reg8(ENET_PAD_MODE(priv->mac_core_num)) & 0xf0;
						gbus_write_reg8(ENET_PAD_MODE(priv->mac_core_num), val);
					}
				}
				break;
			}

			default:
			{
				printf("%s : link is down\n", priv->name);
				break;
			}
		}
	}
	else {
		printk("%s : undefined mode(mode = %d)\n", priv->name, forcemode);
		return;
	}

	return;
}

/*
 *	Ethernet Link adjust
 *
 *		- Phy driver will call this
 *		- duplex, speed : change MAC Configuration
 */
static void tangox_adjust_link(struct tangox_priv *priv)
{
	struct phy_device *phydev = priv->phydev;

	bool speedchanged = false;
	bool duplexchanged = false;

	if (!phydev->link) {
		printf("%s: No link.\n", priv->name);
		return;
	}

	if (phydev->link) {
		/* speed changed */
		speedchanged = true;

		/* duplex mode changed */
		duplexchanged = true;
	}

	if ((speedchanged == true) || (duplexchanged == true)) {
		/* Ethernet MAC Config */
		tangox_mac_config(priv, 0, speedchanged, duplexchanged);

		if (phydev->link) {
			printf("link up (%d Mbps - %s Duplex)\n", phydev->speed, (DUPLEX_FULL == phydev->duplex)? "Full" : "Half");
		}
		else {
			printf("link down\n");
		}
	}

	return;
}

static int tangox_tx_desc_init(struct eth_device *dev)
{
	struct tangox_priv *priv = dev->priv;
	struct tangox_desc* desc = NULL;
	int i = 0;

	/* Clear TX Data Buffer */
	memset(priv->txbuffs, 0x0, (CONFIG_ETH_BUFSIZE * DESCRIPTOR_COUNT));

	/* TX Descriptor */
	for(i = 0; i < DESCRIPTOR_COUNT ; i++)
	{
		desc = (struct tangox_desc*)&priv->tx_desc[i];

        desc->txrx_report = 0;

		desc->starting_address	= (u32)&priv->txbuffs[i*CONFIG_ETH_BUFSIZE];
		desc->report			= (u32)&desc->txrx_report;
		desc->control			= TX_DESC_BTS(2) | TX_DESC_EOC(1) | TX_DESC_EOF(1);

		if(i != (DESCRIPTOR_COUNT - 1))
		{
			desc->next			= (struct tangox_desc*)&priv->tx_desc[i + 1];
		}
		else
		{
			desc->next			= (struct tangox_desc*)&priv->tx_desc[0];
		}
	}

    /* flush cache - flush out descriptor only. tx buffer will be flushed out when it is used */
    flush_dcache_range( (unsigned long)&priv->tx_desc[0],
                        (unsigned long)&priv->tx_desc[0] + sizeof(struct tangox_desc) * DESCRIPTOR_COUNT);

	/* Reset Current Descriptor Number */
	priv->next_tx_desc_num = 0;

	return 0;
}

static int tangox_rx_desc_init(struct eth_device *dev)
{
	struct tangox_priv *priv = dev->priv;
	struct tangox_desc* desc = NULL;
	int i = 0;

	/* Clear Data Buffer */
	memset(priv->rxbuffs, 0x0, (CONFIG_ETH_BUFSIZE * DESCRIPTOR_COUNT));

	/* RX Descriptor */
	for(i = 0; i < DESCRIPTOR_COUNT ; i++)
	{
		desc = (struct tangox_desc*)&priv->rx_desc[i];

        desc->txrx_report = 0;

		desc->starting_address	= (u32)&priv->rxbuffs[i*CONFIG_ETH_BUFSIZE];
		desc->report			= (u32)&desc->txrx_report;

		if(i != (DESCRIPTOR_COUNT - 1))
		{
			desc->next			= (struct tangox_desc*)&priv->rx_desc[i + 1];
			desc->control		= RX_DESC_DMA_COUNT(1536) | RX_DESC_BTS(2);
		}
		else
		{
			desc->next			= (struct tangox_desc*)&priv->rx_desc[0];
			desc->control		= RX_DESC_DMA_COUNT(1536) | RX_DESC_BTS(2);
		}
	}
    
    /* flush out rx buffer, CONFIG_ETH_BUFSIZE and  DESCRIPTOR_COUNT are 32 byte aligned */
    flush_dcache_range( (unsigned long)priv->rxbuffs, 
                        (unsigned long)priv->rxbuffs + CONFIG_ETH_BUFSIZE * DESCRIPTOR_COUNT );
    
    /* flush cache - flush out descriptor */
    flush_dcache_range( (unsigned long)&priv->rx_desc[0],
                        (unsigned long)&priv->rx_desc[0] + sizeof(struct tangox_desc) * DESCRIPTOR_COUNT);
       
                        
	/* Reset Current Descriptor Number */
	priv->next_rx_desc_num = 0;

	return 0;
}

static int tangox_desc_init(struct eth_device *dev)
{
	tangox_tx_desc_init(dev);
	tangox_rx_desc_init(dev);

	return 0;
}

static int tangox_eth_tx_mac_enable(struct eth_device *dev, int enable)
{
	struct tangox_priv *priv = dev->priv;

	if(enable == 1)	/* Enable */
	{
		/* TX Retry Count */
		gbus_write_reg8(ENET_TX_CTL2(priv->mac_core_num), 5);
		tangox_set_reg8(ENET_TX_CTL1(priv->mac_core_num), TX_EN);
		udelay(100);
		return 0;
	}

	/* Stop TX MAC */
	tangox_clear_reg8(ENET_TX_CTL1(priv->mac_core_num), TX_EN);
	udelay(100);
	return 0;
}

static int tangox_eth_rx_mac_enable(struct eth_device *dev, int enable)
{
	struct tangox_priv *priv = dev->priv;

	if(enable == 1)	/* Enable */
	{
		/* Enable RX MAC */
		tangox_set_reg8(ENET_RX_CTL(priv->mac_core_num), RX_EN);
		udelay(100);
		return 0;
	}

	/* Diable RX MAC */
	tangox_clear_reg8(ENET_RX_CTL(priv->mac_core_num), RX_EN);
	udelay(100);
	return 0;
}

static int tangox_eth_rx_dma_enable(struct eth_device *dev, int enable)
{
	struct tangox_priv *priv = dev->priv;

	if(enable != 0)	/* Enable */
	{
		/* Start RX DMA */
		tangox_set_reg(ENET_RXC_CR(priv->mac_core_num), RCR_EN);
		udelay(100);
		return 0;
	}

	udelay(100);
	return 0;
}

/*********************************************************************************
 *
 *	tangox_mac_reset
 *
 *		reset ethernet mac
 *			- clear TX & RX Buffer
 *			- clear TX & RX Buffer Status
 *			- clear TX & RX Status Report
 *
 *********************************************************************************/
static int tangox_mac_reset(struct eth_device *dev)
{
	eth_debug("[%s : %d] +++\n", __func__, __LINE__);

	/* STOP RX DMA */
	tangox_eth_rx_dma_enable(dev, 0);

	/* STOP RX MAC */
	tangox_eth_rx_mac_enable(dev, 0);

	/* STOP TX MAC */
	tangox_eth_tx_mac_enable(dev, 0);

	/* Descriptor Init */
	tangox_desc_init(dev);

	return 0;
}

static int tangox_write_hwaddr(struct eth_device *dev)
{
	struct tangox_priv *priv = dev->priv;

	eth_debug("MAC %02x:%02x:%02x:%02x:%02x:%02x\n", dev->enetaddr[0]
												   , dev->enetaddr[1]
												   , dev->enetaddr[2]
												   , dev->enetaddr[3]
												   , dev->enetaddr[4]
												   , dev->enetaddr[5]);

	/*set mac addr*/
	gbus_write_reg8(ENET_MAC_ADDR1(priv->mac_core_num), dev->enetaddr[0]);
	gbus_write_reg8(ENET_MAC_ADDR2(priv->mac_core_num), dev->enetaddr[1]);
	gbus_write_reg8(ENET_MAC_ADDR3(priv->mac_core_num), dev->enetaddr[2]);
	gbus_write_reg8(ENET_MAC_ADDR4(priv->mac_core_num), dev->enetaddr[3]);
	gbus_write_reg8(ENET_MAC_ADDR5(priv->mac_core_num), dev->enetaddr[4]);
	gbus_write_reg8(ENET_MAC_ADDR6(priv->mac_core_num), dev->enetaddr[5]);

	/* set unicast addr */
	gbus_write_reg8(ENET_UC_ADDR1(priv->mac_core_num), dev->enetaddr[0]);
	gbus_write_reg8(ENET_UC_ADDR2(priv->mac_core_num), dev->enetaddr[1]);
	gbus_write_reg8(ENET_UC_ADDR3(priv->mac_core_num), dev->enetaddr[2]);
	gbus_write_reg8(ENET_UC_ADDR4(priv->mac_core_num), dev->enetaddr[3]);
	gbus_write_reg8(ENET_UC_ADDR5(priv->mac_core_num), dev->enetaddr[4]);
	gbus_write_reg8(ENET_UC_ADDR6(priv->mac_core_num), dev->enetaddr[5]);

	return 0;
}

static int tangox_phy_init(struct eth_device *dev)
{
	struct tangox_priv *priv = dev->priv;
	struct phy_device *phydev;
	int mask = 0xffffffff;

	if ((priv->phy_addr != 0) && (priv->phydev != NULL)) {
		//printf("Found ethernet phy : %s\n", priv->phydev->drv->name);
		return 1;
	}

	phydev = phy_find_by_mask(priv->bus, mask, priv->mac_core_num);
	if (!phydev)
	{
		printf("fail to find phy\n");
		return -1;
	}

	printf("Found ethernet phy : %s\n", phydev->drv->name);

	/* Add Phy device information under Private context */
	priv->phydev = phydev;
	priv->phy_addr = phydev->addr;

	/* now we are supposed to have a proper phydev, to attach to... */
	if ((phydev->drv->features & (SUPPORTED_1000baseT_Half | SUPPORTED_1000baseT_Full)) > 0) {
		int val;

		/* Gigabit Mode : RGMII */
		val = gbus_read_reg8(ENET_PAD_MODE(priv->mac_core_num)) & 0xf0;
		gbus_write_reg8(ENET_PAD_MODE(priv->mac_core_num), val | 0x01);
		priv->phy_interface = PHY_INTERFACE_MODE_RGMII;

		phydev = phy_connect(priv->bus, phydev->addr, dev, PHY_INTERFACE_MODE_RGMII);
	}
	else {
		int val;

		/* 10/100 T : MII */
		val = gbus_read_reg8(ENET_PAD_MODE(priv->mac_core_num)) & 0xf0;
		gbus_write_reg8(ENET_PAD_MODE(priv->mac_core_num), val);
		priv->phy_interface = PHY_INTERFACE_MODE_MII;

		phydev = phy_connect(priv->bus, phydev->addr, dev, PHY_INTERFACE_MODE_MII);
	}

	phydev->supported &= (SUPPORTED_10baseT_Half
						| SUPPORTED_10baseT_Full
						| SUPPORTED_100baseT_Half
						| SUPPORTED_100baseT_Full
						| SUPPORTED_Autoneg
						| SUPPORTED_MII
						| SUPPORTED_TP
						| SUPPORTED_1000baseT_Half
						| SUPPORTED_1000baseT_Full);

	phydev->advertising = phydev->supported;

	phy_config(phydev);

	return 1;
}

static int tangox_mdio_init(char *name, struct tangox_priv *priv)
	{
	struct mii_dev *bus = mdio_alloc();

	if (!bus) {
		printf("Failed to allocate MDIO bus\n");
		return -1;
	}

	bus->read = tangox_mdiobus_read;
	bus->write = tangox_mdiobus_write;
	sprintf(bus->name, "%s", name);

	bus->priv = (void *)priv;

	return mdio_register(bus);
}

static int tangox_eth_init(struct eth_device *dev, bd_t *bis)
{
	struct tangox_priv *priv = dev->priv;
	unsigned char tmpval = 0;

	eth_debug("[%s : %d] +++\n", __func__, __LINE__);

	if (tangox_mac_reset(dev) < 0)
		return -1;

	/* Setup MAC Address */
	tangox_write_hwaddr(dev);

	/* set MDIO clock divider */
	gbus_write_reg16(ENET_MDIO_CLK(priv->mac_core_num), 74);  //74, hand calculated 295Mhz/ (2Mhz*2)

	/*set threshold for internal clock 0x1*/
	gbus_write_reg8(ENET_IC_THRESHOLD(priv->mac_core_num), 1);

	/*set Random seed 0x8*/
	gbus_write_reg8(ENET_RANDOM_SEED(priv->mac_core_num), 0x08);

	/*set TX single deferral params 0xc*/
	gbus_write_reg8(ENET_TX_SDP(priv->mac_core_num), 0xc);

	/*set slot time 0x7f for 10/100Mbps*/
	gbus_write_reg8(ENET_SLOT_TIME(priv->mac_core_num), 0x7f);

	/*set Threshold for partial full 0xff */
	gbus_write_reg8(ENET_PF_THRESHOLD(priv->mac_core_num), 0xff);

	/* set Pause Quanta 65535 */
	gbus_write_reg8(ENET_PQ1(priv->mac_core_num), 0xff);
	gbus_write_reg8(ENET_PQ2(priv->mac_core_num), 0xff);

	tangox_desc_init(dev);

	/* configure MAC controller */
	tmpval = (TX_RETRY_EN | TX_PAD_EN | TX_APPEND_FCS);
	gbus_write_reg8(ENET_TX_CTL1(priv->mac_core_num), (unsigned char)tmpval);

	/* set retry 5 time when collision occurs */
	gbus_write_reg8(ENET_TX_CTL2(priv->mac_core_num), 5);

	tmpval = (RX_RUNT | RX_PAD_STRIP | RX_PAUSE_EN | RX_AF_EN);
	gbus_write_reg8(ENET_RX_CTL(priv->mac_core_num), (unsigned char)tmpval);

	/* buffer size for transmit must be 1 from the doc
	   however, it's said that using 0xff ??*/
	gbus_write_reg8(ENET_TX_BUFSIZE(priv->mac_core_num), 0xff);

	tangox_phy_init(dev);

	/* Start up the PHY */
	if (phy_startup(priv->phydev)) {
		printf("Could not initialize PHY %s\n",
		       priv->phydev->dev->name);
		return -1;
	}

	tangox_adjust_link(priv);

	/***********************
	 *	TX MAC & RX MAC
	 ***********************/
	priv->first_transmit = 0;

	return 0;
}

static int tangox_eth_send(struct eth_device *dev, void *packet,
		int length)
{
	struct tangox_priv *priv = dev->priv;
    struct tangox_desc *desc = NULL;
	u32 control = 0;
	u32 val = 0;
    u32 tx_start_addr = 0;

	eth_debug("[%s : %d] +++\n", __func__, __LINE__);

	/* check frame length */
	if ((length = ((ETH_ZLEN < length) ? length : ETH_ZLEN)) > T_BUF_SIZE)
        printf("WARNING: frame is too big!\n");

	/* copy data to the buffer */
    tx_start_addr = (unsigned long)(&priv->txbuffs[priv->next_tx_desc_num * CONFIG_ETH_BUFSIZE]);
	memcpy((unsigned char *)tx_start_addr, (unsigned char*)packet, length);
        
    /* flush out buffer for dma */
    flush_dcache_range( tx_start_addr, (tx_start_addr + length + 0x1F) & ~0x1F );   

	/* Update Control Field inside TX Descriptor */
	control = priv->tx_desc[priv->next_tx_desc_num].control;
	priv->tx_desc[priv->next_tx_desc_num].control = (control & 0xffff0000) | ((((length+7)/8)*8) & 0x0000ffff);

	if(priv->first_transmit == 0)
	{
		/* configure RX DMA Channels */
		val = RCR_BTS(2) | RCR_DIE | RCR_RFI(1) | RCR_LE | RCR_RS | RCR_DM;
		gbus_write_reg32(ENET_RXC_CR(priv->mac_core_num), val);

		/* Clear RX Status */
		gbus_write_reg32(ENET_RXC_SR(priv->mac_core_num), RSR_DE | RSR_DI | RSR_RO | RSR_RI);

		/* RX Descriptor */
		gbus_write_reg32(ENET_RX_DESC_ADDR(priv->mac_core_num), (u32)&priv->rx_desc[0]);

		/* Enable TX DMA */
		gbus_write_reg32(ENET_TXC_CR(priv->mac_core_num), TCR_BTS(2) | TCR_LE | TCR_RS | TCR_DM);

		/***********************
		 *	DMA
		 ***********************/
		tangox_eth_rx_mac_enable(dev, 1);
		tangox_eth_tx_mac_enable(dev, 1);

		tangox_eth_rx_dma_enable(dev, 1);

		priv->first_transmit = 1;
	}
	else
	{
		/* Check Current RX DMA Status */
		if(((gbus_read_reg32(ENET_RXC_CR(priv->mac_core_num)) & RCR_EN) == 0)
		|| (priv->next_rx_desc_num == DESCRIPTOR_COUNT))
		{
	
			priv->next_rx_desc_num = 0;

			dump_registers(dev);

			tangox_eth_rx_dma_enable(dev, 1);
		}
	}

	/* TX Descriptor Address */
	gbus_write_reg32(ENET_TX_DESC_ADDR(priv->mac_core_num), (u32)&priv->tx_desc[priv->next_tx_desc_num]);
	udelay(1);

	//dump_registers(dev);

	/* Clear DMA TX channel status reg */
	gbus_write_reg32(ENET_TXC_SR(priv->mac_core_num), TSR_DE | TSR_DI | TSR_TO | TSR_TI);
	udelay(1);

	/* Clear Report */
    desc = (struct tangox_desc *)&priv->tx_desc[priv->next_tx_desc_num];
	desc->txrx_report = 0;
    
    /* flush descriptor */
    flush_dcache_range( (unsigned long)&priv->tx_desc[priv->next_tx_desc_num],
                        (unsigned long)&priv->tx_desc[priv->next_tx_desc_num] + sizeof(struct tangox_desc));

	/* Enable TX DMA */
	tangox_set_reg(ENET_TXC_CR(priv->mac_core_num), TCR_EN);
	udelay(1);
    do{
        mdelay(1);
    }
    while (((gbus_read_reg32(ENET_TXC_CR(priv->mac_core_num)) & TCR_EN) != 0));

	/* Update Next Descriptor Number */
	priv->next_tx_desc_num++;

	if(priv->next_tx_desc_num == DESCRIPTOR_COUNT)
		priv->next_tx_desc_num = 0;

	if(priv->first_transmit == 0)
		priv->first_transmit = 1;

	eth_debug("[%s : %d] ---\n", __func__, __LINE__);

	return 0;
}

static int tangox_eth_recv(struct eth_device *dev)
{
	struct tangox_priv *priv = dev->priv;
	u32 report = 0;
	int length = 0;
	int error = 0;
	struct tangox_desc* desc = NULL;
	u32 start_desc_number = priv->next_rx_desc_num;
	int i = 0;
    u32 loop_cnt = 0;

	eth_debug("[%s : %d] +++\n", __func__, __LINE__);
  
    i = start_desc_number;
    
    while ( loop_cnt < DESCRIPTOR_COUNT )
	{
        /* invalidate descriptor */
        invalidate_dcache_range( (unsigned long)&priv->rx_desc[i], 
                                 (unsigned long)&priv->rx_desc[i] + sizeof(struct tangox_desc) );
        
		/* Check EOC Field */
		desc = (struct tangox_desc*)&priv->rx_desc[i];
		error = 0;
		report = 0;
		length = 0;

        switch ( priv->phydev->speed )
        {
            case SPEED_1000:
                udelay(1);
                break;
            case SPEED_100:
                udelay(5);
                break;
            case SPEED_10:
                mdelay(5);
                break;
            default:
                break;
        }

		/* Check EOC Field */
		desc = (struct tangox_desc*)&priv->rx_desc[i];

		/* Check Report */
		report = desc->txrx_report;
        
		if(report == 0) 
        {
            mdelay(10);
            return 0;
        }

		length = RX_BYTES_TRANSFERRED(report);

		if(report & RX_FCS_ERR)
		{
			eth_debug("[%s : %d] RX_FCS_ERR\n", __func__, __LINE__);
			error++;
		}

		if(report & RX_LATE_COLLISION)
		{
			eth_debug("[%s : %d] RX_LATE_COLLISION\n", __func__, __LINE__);
			error++;
		}

		if(report & RX_FIFO_OVERRUN)
		{
			eth_debug("[%s : %d] RX_FIFO_OVERRUN\n", __func__, __LINE__);
			error++;
		}

		if(report & RX_RUNT_PKT)
		{
			eth_debug("[%s : %d] RX_RUNT_PKT\n", __func__, __LINE__);
			error++;
		}

		if((report & (RX_FRAME_LEN_ERROR | RX_LENGTH_ERR)) || (length > RX_BUF_SIZE))
		{
			eth_debug("[%s : %d] RX LENTH ERROR\n", __func__, __LINE__);
			error++;
		}

		if(error > 0)
		{
			eth_debug("[%s : %d] error ---\n", __func__, __LINE__);
            
            loop_cnt++;
            i++;
            if ( i >= DESCRIPTOR_COUNT )
                i = 0;
			continue;
		}

		if(length > 0)
		{
            /* invalidate rx data buffer */
            invalidate_dcache_range( (unsigned long)&priv->rxbuffs[i*CONFIG_ETH_BUFSIZE],
                                     (unsigned long)&priv->rxbuffs[i*CONFIG_ETH_BUFSIZE] + CONFIG_ETH_BUFSIZE  );
                         
			dump_packet((char*)&priv->rxbuffs[i*CONFIG_ETH_BUFSIZE], length);
            
            eth_debug("[%s : %d] Recv %d Bytes\n", __func__, __LINE__, length);
        
            net_process_received_packet((uchar *)&priv->rxbuffs[i*CONFIG_ETH_BUFSIZE], length);     
		}
        

		/* Update Report Field */
        desc->txrx_report = 0;
        memset( (char*)&priv->rxbuffs[i*CONFIG_ETH_BUFSIZE], 0x00, length );
        flush_dcache_range( (unsigned long)&priv->rx_desc[i],
                            (unsigned long)&priv->rx_desc[i] + sizeof(struct tangox_desc));
                
        priv->next_rx_desc_num = i + 1;

		eth_debug("[%s : %d] ---\n", __func__, __LINE__);

		if( priv->next_rx_desc_num == DESCRIPTOR_COUNT)
			priv->next_rx_desc_num = 0;

		return (length);
	}

	eth_debug("[%s : %d] ---\n", __func__, __LINE__);

	return 0;
}

static void tangox_eth_halt(struct eth_device *dev)
{
	struct tangox_priv *priv = dev->priv;

	eth_debug("[%s : %d] +++\n", __func__, __LINE__);

	tangox_eth_rx_dma_enable(dev, 0);

	tangox_eth_rx_mac_enable(dev, 0);
	tangox_eth_tx_mac_enable(dev, 0);

	tangox_desc_init(dev);

	priv->first_transmit = 0;

	/* software reset IP */
	gbus_write_reg8(ENET_CORE_RESET(priv->mac_core_num), 0);
	udelay(10);
	gbus_write_reg8(ENET_CORE_RESET(priv->mac_core_num), 1);

	return;
}

int tangox_eth_initialize(int eth_port_num)
{
	struct eth_device *dev;
	struct tangox_priv *priv;

	eth_debug("[%s : %d] +++\n", __func__, __LINE__);

	dev = (struct eth_device *) malloc(sizeof(struct eth_device));
	if (!dev)
		return -ENOMEM;

	/*
	 * Since the priv structure contains the descriptors which need a strict
	 * buswidth alignment, memalign is used to allocate memory
	 */
	priv = (struct tangox_priv *) memalign(32, sizeof(struct tangox_priv));
	if (!priv) {
		free(dev);
		return -ENOMEM;
	}

	memset(dev, 0, sizeof(struct eth_device));
	memset(priv, 0, sizeof(struct tangox_priv));

	sprintf(dev->name, "tangox_enet_%d", eth_port_num);
	dev->priv = priv;

	/* Set MAC Core Name */
	priv->name = dev->name;

	/* Set MAC core Number */
	priv->mac_core_num = eth_port_num;

	/* Phy Interface */
	priv->phydev = NULL;
	priv->bus = NULL;
	priv->phy_addr = 0;
	priv->phy_interface = PHY_INTERFACE_MODE_NONE;

	eth_getenv_enetaddr_by_index("eth", eth_port_num, &dev->enetaddr[0]);

	/* PAD Mode : default */
	gbus_write_reg8(ENET_PAD_MODE(priv->mac_core_num), 0);

	/* Set MDIO Clock */
	gbus_write_reg16(ENET_MDIO_CLK(priv->mac_core_num), 74);  //74, hand calculated 295Mhz/ (2Mhz*2)

	/* Setup MAC Address */
	tangox_write_hwaddr(dev);

	dev->init   = tangox_eth_init;
	dev->halt   = tangox_eth_halt;
	dev->send   = tangox_eth_send;
	dev->recv   = tangox_eth_recv;
	dev->write_hwaddr = tangox_write_hwaddr; // able to write mac address to mac core

	/* Ethernet MAC Reset */
	gbus_write_reg8(ENET_CORE_RESET(priv->mac_core_num), 0);
	mdelay(1);
	gbus_write_reg8(ENET_CORE_RESET(priv->mac_core_num), 1);

	eth_register(dev);

	tangox_mdio_init(dev->name, priv);
	priv->bus = miiphy_get_dev_by_name(dev->name);

	return 1;
}

