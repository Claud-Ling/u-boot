#ifndef __TANGOX_ETH_H__
#define __TANGOX_ETH_H__

#define EM86XX_HOST_BASE					REG_BASE_host_interface
#define EM86XX_HOST_GB_ETHMAC				0x00006000
#define EM86XX_HOST_GB_ETHDMA				0x00006100
#define EM86XX_HOST_GB_2ND_PORT_OFFSET		0x800

#define RX_BUF_SIZE	1536

/* Mac Core Base Address */
#define MAC_BASE(core)						(EM86XX_HOST_BASE + (EM86XX_HOST_GB_2ND_PORT_OFFSET * core) + EM86XX_HOST_GB_ETHMAC)

/*
 * Mac/DMA registers offset, refer to documentation
 */
#define ENET_TX_CTL1(core)			(MAC_BASE(core) + 0x00)
#define TX_TPD		        		(1 << 5)
#define TX_APPEND_FCS       		(1 << 4)
#define TX_PAD_EN          			(1 << 3)
#define TX_RETRY_EN        			(1 << 2)
#define TX_EN		        		(1 << 0)
#define ENET_TX_CTL2(core)			(MAC_BASE(core) + 0x01)
#define ENET_RX_CTL(core)	   	 	(MAC_BASE(core) + 0x04)
#define RX_BC_DISABLE				(1 << 7)
#define RX_RUNT		        		(1 << 6)
#define RX_AF_EN					(1 << 5)
#define RX_PAUSE_EN	     			(1 << 3)
#define RX_SEND_CRC	    			(1 << 2)
#define RX_PAD_STRIP	   			(1 << 1)
#define RX_EN		        		(1 << 0)
#define ENET_RANDOM_SEED(core)		(MAC_BASE(core) + 0x8)
#define ENET_TX_SDP(core)	    	(MAC_BASE(core) + 0x14)
#define ENET_TX_TPDP1(core)			(MAC_BASE(core) + 0x18)
#define ENET_TX_TPDP2(core)			(MAC_BASE(core) + 0x19)
#define ENET_SLOT_TIME(core)		(MAC_BASE(core) + 0x1c)
#define ENET_MDIO_CMD1(core)		(MAC_BASE(core) + 0x20)
#define ENET_MDIO_CMD2(core)		(MAC_BASE(core) + 0x21)
#define ENET_MDIO_CMD3(core)		(MAC_BASE(core) + 0x22)
#define ENET_MDIO_CMD4(core)		(MAC_BASE(core) + 0x23)
#define MIIAR_ADDR(x)      			((x) << 21)
#define MIIAR_REG(x)       			((x) << 16)
#define MIIAR_DATA(x)      			((x) <<  0)
#define MDIO_CMD_GO	    			(1 << 31)
#define MDIO_CMD_WR   				(1 << 26)
#define ENET_MDIO_STS1(core)		(MAC_BASE(core) + 0x24)
#define ENET_MDIO_STS2(core)		(MAC_BASE(core) + 0x25)
#define ENET_MDIO_STS3(core)		(MAC_BASE(core) + 0x26)
#define ENET_MDIO_STS4(core)		(MAC_BASE(core) + 0x27)
#define MDIO_STS_ERR				(1 << 31)
#define ENET_MC_ADDR1(core)			(MAC_BASE(core) + 0x28)
#define ENET_MC_ADDR2(core)			(MAC_BASE(core) + 0x29)
#define ENET_MC_ADDR3(core)			(MAC_BASE(core) + 0x2a)
#define ENET_MC_ADDR4(core)			(MAC_BASE(core) + 0x2b)
#define ENET_MC_ADDR5(core)			(MAC_BASE(core) + 0x2c)
#define ENET_MC_ADDR6(core)			(MAC_BASE(core) + 0x2d)
#define ENET_MC_INIT(core)			(MAC_BASE(core) + 0x2e)
#define ENET_UC_ADDR1(core)			(MAC_BASE(core) + 0x3c)
#define ENET_UC_ADDR2(core)			(MAC_BASE(core) + 0x3d)
#define ENET_UC_ADDR3(core)			(MAC_BASE(core) + 0x3e)
#define ENET_UC_ADDR4(core)			(MAC_BASE(core) + 0x3f)
#define ENET_UC_ADDR5(core)			(MAC_BASE(core) + 0x40)
#define ENET_UC_ADDR6(core)			(MAC_BASE(core) + 0x41)
#define ENET_MAC_MODE(core)			(MAC_BASE(core) + 0x44)
#define RGMII_MODE	    			(1 << 7)
#define HALF_DUPLEX  				(1 << 4)
#define BST_EN	    	 			(1 << 3)
#define LB_EN	    				(1 << 2)
#define GMAC_MODE	    			(1 << 0)
#define ENET_IC_THRESHOLD(core)	(MAC_BASE(core) + 0x50)
#define ENET_PE_THRESHOLD(core)	(MAC_BASE(core) + 0x51)
#define ENET_PF_THRESHOLD(core)	(MAC_BASE(core) + 0x52)
/* TX buffer size must be set to 0x01 ??*/
#define ENET_TX_BUFSIZE(core)	(MAC_BASE(core) + 0x54)
#define ENET_FIFO_CTL(core)		(MAC_BASE(core) + 0x56)
#define ENET_PQ1(core)	    	(MAC_BASE(core) + 0x60)
#define ENET_PQ2(core)	    	(MAC_BASE(core) + 0x61)
#define ENET_MAC_ADDR1(core)	(MAC_BASE(core) + 0x6a)
#define ENET_MAC_ADDR2(core)	(MAC_BASE(core) + 0x6b)
#define ENET_MAC_ADDR3(core)	(MAC_BASE(core) + 0x6c)
#define ENET_MAC_ADDR4(core)	(MAC_BASE(core) + 0x6d)
#define ENET_MAC_ADDR5(core)	(MAC_BASE(core) + 0x6e)
#define ENET_MAC_ADDR6(core)	(MAC_BASE(core) + 0x6f)
#define ENET_STAT_DATA1(core)	(MAC_BASE(core) + 0x78)
#define ENET_STAT_DATA2(core)	(MAC_BASE(core) + 0x79)
#define ENET_STAT_DATA3(core)	(MAC_BASE(core) + 0x7a)
#define ENET_STAT_DATA4(core)	(MAC_BASE(core) + 0x7b)
#define ENET_STAT_INDEX(core)	(MAC_BASE(core) + 0x7c)
#define ENET_STAT_CLEAR(core)	(MAC_BASE(core) + 0x7d)
#define ENET_SLEEP_MODE(core)	(MAC_BASE(core) + 0x7e)
#define SLEEP_MODE	    			(1 << 0)
#define ENET_WAKEUP(core)	    (MAC_BASE(core) + 0x7f)
#define WAKEUP		    			(1 << 0)

/* AU-NB8800 Ethernet 10/100/1000 AMBA Subsystem Registers(DMA) */
#define ENET_TXC_CR(core)			(MAC_BASE(core) + 0x100)
#define TCR_LK		    			(1 << 12)
#define TCR_DS		    			(1 << 11)
#define TCR_BTS(x)          		(((x) & 0x7) << 8)
#define TCR_DIE		    			(1 << 7)
#define TCR_TFI(x)          		(((x) & 0x7) << 4)
#define TCR_LE		    			(1 << 3)
#define TCR_RS		    			(1 << 2)
#define TCR_DM		    			(1 << 1)
#define TCR_EN		    			(1 << 0)

#define ENET_TXC_SR(core)		(MAC_BASE(core) + 0x104)
#define TSR_DE						(1 << 3)
#define TSR_DI						(1 << 2)
#define TSR_TO						(1 << 1)
#define TSR_TI						(1 << 0)

#define ENET_TX_SAR(core)			(MAC_BASE(core) + 0x108)
#define ENET_TX_DESC_ADDR(core)		(MAC_BASE(core) + 0x10c)
#define TX_DESC_ID(x)			((x) << 23)
#define TX_DESC_EOC(x)			((x) << 22)
#define TX_DESC_LK(x)			((x) << 20)
#define TX_DESC_EOF(x)			((x) << 21)
#define TX_DESC_DS(x)			((x) << 19)
#define TX_DESC_BTS(x)			(((x) & 0x7) << 16)
#define TX_DESC_DMA_COUNT(x)	((x) & 0x0000FFFF)

#define ENET_TX_REPORT_ADDR(core)	(MAC_BASE(core) + 0x110)
#define TX_BYTES_TRASFERRED(x)			(((x) >> 16) & 0xffff)
#define TX_FIRST_DEFERRAL				(1 << 7)
#define TX_EARLY_COLLISIONS(x)			(((x) >> 3) & 0xf)
#define TX_LATE_COLLISION				(1 << 2)
#define TX_PACKET_DROPPED				(1 << 1)
#define TX_FIFO_UNDERRUN				(1 << 0)

#define ENET_TX_FIFO_SR(core)		(MAC_BASE(core) + 0x114)
#define ENET_TX_ITR(core)			(MAC_BASE(core) + 0x118)

#define ENET_RXC_CR(core)		(MAC_BASE(core) + 0x200)
#define RCR_RECV_COUNT(x)			((x & 0xFFFF) << 16)
#define RCR_FI						(1 << 13)
#define RCR_LK						(1 << 12)
#define RCR_DS						(1 << 11)
#define RCR_BTS(x)      			(((x) & 7) << 8)
#define RCR_DIE						(1 << 7)
#define RCR_RFI(x)      	   		(((x) & 7) << 4)
#define RCR_LE						(1 << 3)
#define RCR_RS						(1 << 2)
#define RCR_DM						(1 << 1)
#define RCR_EN						(1 << 0)

#define ENET_RXC_SR(core)		(MAC_BASE(core) + 0x204)
#define RSR_DE						(1 << 3)
#define RSR_DI						(1 << 2)
#define RSR_RO						(1 << 1)
#define RSR_RI						(1 << 0)

#define ENET_RX_SAR(core)		(MAC_BASE(core) + 0x208)
#define ENET_RX_DESC_ADDR(core)	(MAC_BASE(core) + 0x20c)
#define RX_DESC_ID(x)			((x) << 23)
#define RX_DESC_EOC(x)			((x) << 22)
#define RX_DESC_LK(x)			((x) << 20)
#define RX_DESC_EOF(x)			((x) << 21)
#define RX_DESC_DS(x)			((x) << 19)
#define RX_DESC_BTS(x)			(((x) & 0x7) << 16)
#define RX_DESC_DMA_COUNT(x)	((x) & 0x0000FFFF)

#define ENET_RX_REPORT_ADDR(core)		(MAC_BASE(core) + 0x210)
#define RX_BYTES_TRANSFERRED(x)			(((x) >> 16) & 0xFFFF)
#define RX_MULTICAST_PKT				(1 << 9)
#define RX_BROADCAST_PKT				(1 << 8)
#define RX_LENGTH_ERR					(1 << 7)
#define RX_FCS_ERR						(1 << 6)
#define RX_RUNT_PKT						(1 << 5)
#define RX_FIFO_OVERRUN					(1 << 4)
#define RX_LATE_COLLISION				(1 << 3)
#define RX_FRAME_LEN_ERROR				(1 << 2)

#define ENET_RX_FIFO_SR(core)		(MAC_BASE(core) + 0x214)
#define ENET_RX_ITR(core)			(MAC_BASE(core) + 0x218)

#define ENET_PAD_MODE(core)			(MAC_BASE(core) + 0x400)
#define ENET_MDIO_CLK(core)			(MAC_BASE(core) + 0x420)
#define ENET_CORE_RESET(core)		(MAC_BASE(core) + 0x424)


/* Extra Configuration */
#define CONFIG_MACRESET_TIMEOUT	(3 * CONFIG_SYS_HZ)
#define CONFIG_MDIO_TIMEOUT	(3 * CONFIG_SYS_HZ)
#define CONFIG_PHYRESET_TIMEOUT	(3 * CONFIG_SYS_HZ)
#define CONFIG_AUTONEG_TIMEOUT	(5 * CONFIG_SYS_HZ)


/**********************************************************
 * SSN8800 Ethernet MAC registers
 **********************************************************/
#define CONFIG_ETH_BUFSIZE	2048	/* Aligned Data size */
#define DESCRIPTOR_COUNT	32

struct tangox_desc {
	u32					starting_address;
	struct tangox_desc*	next;
	u32 				report;
	u32 				control;
    u32                 dummy[3];
    u32                 txrx_report;
};

struct tangox_priv {
	/* Descriptor */
	struct tangox_desc	tx_desc[DESCRIPTOR_COUNT];
	struct tangox_desc	rx_desc[DESCRIPTOR_COUNT];

	/* DATA Buffer */
	char txbuffs[CONFIG_ETH_BUFSIZE * DESCRIPTOR_COUNT];
	char rxbuffs[CONFIG_ETH_BUFSIZE * DESCRIPTOR_COUNT];

	
	u32 next_tx_desc_num;
	u32 next_rx_desc_num;

    /* device */
    const char                  *name;
    int                         mac_core_num;

	/* Device */
	int							first_transmit;

	/* Ethernet Device */
	int							phy_addr;
    struct eth_device 			*dev;
	struct phy_device			*phydev;
	struct mii_dev				*bus;

	phy_interface_t				phy_interface;
};

/* Duplex mode specific definitions */
#define NET_HALF_DUPLEX		1
#define NET_FULL_DUPLEX		2


static void __inline__ tangox_set_reg( u32 reg, u32 data )
{
	u32 temp = data;
	temp |= gbus_read_reg32(reg);
	gbus_write_reg32(reg, temp);
}

static void __inline__ tangox_clear_reg( u32 reg, u32 data )
{
	u32 data1;
	udelay(100);
	data1 = (gbus_read_reg32(reg) & (~data));
	gbus_write_reg32(reg, data1);
}

static void __inline__ tangox_set_reg8( u32 reg, u8 data )
{
	u8 temp = data;
	temp |= gbus_read_reg8(reg);
	gbus_write_reg8(reg, temp);
}

static void __inline__ tangox_clear_reg8( u32 reg, u8 data )
{
	u8 data1;
	data1 = (gbus_read_reg8(reg) & (~data));
	udelay(100);
	gbus_write_reg8(reg, data1);
}


#endif /* __TANGOX_ETH_H__ */
