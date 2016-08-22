/*
 * Sigma designs USB HOST xHCI Controller
 *
 * (C) Copyright 2013
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <usb.h>
#include <asm-generic/errno.h>
#include <asm/io.h>
#include <asm/arch/reg_io.h>
#include <dm.h>
#include "xhci.h"

//#define DEBUG
#if 0
#ifdef DEBUG
#define debug(fmt, args...) printf(fmt, ##args)
#else
#define debug(fmt, args...) do{}while(0)
#endif
#endif

/* Declare global data pointer */
DECLARE_GLOBAL_DATA_PTR;
#ifndef BIT
#define BIT(x) (1 << (x))
#endif

#define XHCI_REG(x)	({		\
	((SIGMA_XHCI_BASE) + (x));	\
})

/*** Register Offset ***/
#define TRIX_USB3_USBCMD	0x0020	/*USBCMD*/
#define TRIX_USB3_GRXTHRCFG	0xc10c	/*GRXTHRCFG*/
#define TRIX_USB3_GUCTL		0xc12c	/*GUCTL*/
#define TRIX_USB3_GCTL		0xc110	/*GCTL*/
#define TRIX_USB3_GUSB2PHYCFG	0xc200	/*GUSB2PHYCFG*/
#define TRIX_USB3_GUSB3PIPECTL	0xc2c0	/*GUSB3PIPECTL*/
#define TRIX_USB3_TEST_DBG_REG	0xcd04	/*TEST_DBG_REG*/
#define TRIX_USB3_PHY_TUNE_REG2	0xcd0c	/*PHY_TUNE_REG2*/
#define TRIX_USB3_CFG_REG2	0xcd14	/*CFG_REG2*/

/*** Register Settings ***/
#define TRIX_USB3_USBCMD_HCRST		BIT(1)
#define TRIX_USB3_USBCMD_HCRST_MASK	BIT(1)

#define TRIX_USB3_GUCTL_VAL	0x0a808010

#define TRIX_USB3_GCTL_VAL	0x2fa01004

#define TRIX_USB3_GUSB3PIPECTL_VAL	0x130e0002 /*set bit[28] to enable detection in P2*/

#define TRIX_USB3_GUSB2PHYCFG_VAL	0x00002540

#define TRIX_USB3_CFG_REG2_USBPHY_REF_SSP_EN		BIT(9)
#define TRIX_USB3_CFG_REG2_USBPHY_REF_SSP_EN_MASK	BIT(9)

#define TRIX_USB3_PHY_TUNE_REG2_USBPHY_PCS_TX_SWING_FULL	0x7f
#define TRIX_USB3_PHY_TUNE_REG2_USBPHY_PCS_TX_SWING_FULL_MASK	0x7f	/*bit[6:0]*/

#define TRIX_USB3_GRXTHRCFG_USBMaxRxBurstSize_VAL	(0x2 << 19)
#define TRIX_USB3_GRXTHRCFG_USBMaxRxBurstSize_MASK	(0x1f << 19)	/*bit[23:19]*/
#define TRIX_USB3_GRXTHRCFG_USBRxPktCnt_VAL		(0x3 << 24)
#define TRIX_USB3_GRXTHRCFG_USBRxPktCnt_MASK		(0xf << 24)	/*bit[27:24]*/
#define TRIX_USB3_GRXTHRCFG_USBRxPktCntSel_VAL		BIT(29)		/*bit[29]*/
#define TRIX_USB3_GRXTHRCFG_USBRxPktCntSel_MASK		BIT(29)		/*bit[29]*/

#define SIGMA_XHCI_BASE    0xf5200000


void usb3_power_init(void)
{
#if defined(CONFIG_MACH_SIGMA_SX6) || defined(CONFIG_MACH_SIGMA_SX7)
	/* Reset USB Host will be down after xhci_hcd_init, so comment here */
	//MWriteRegWord((volatile void*)0x15200020, 0x2,0x2);
	//MWriteRegWord(XHCI_REG(TRIX_USB3_USBCMD), TRIX_USB3_USBCMD_HCRST, TRIX_USB3_USBCMD_HCRST_MASK);

	/* Set REFCLKPER */	
	//MWriteRegWord((volatile void*)0x1520c12c, 0xa808010, 0xffffffff);
	WriteRegWord(XHCI_REG(TRIX_USB3_GUCTL), TRIX_USB3_GUCTL_VAL);
	
	/* Set Power down scale */
	//MWriteRegWord((volatile void*)0x1520c110, 0x2fa01004, 0xffffffff);
	WriteRegWord(XHCI_REG(TRIX_USB3_GCTL), TRIX_USB3_GCTL_VAL);

	/* Set USB3 PIPE Control */
	/* Disable receiver detection in P3, Fix synopsys PHY bug */
	//MWriteRegWord((volatile void*)0x1520c2c0, 0x030e0002, 0xffffffff);
	WriteRegWord(XHCI_REG(TRIX_USB3_GUSB3PIPECTL), TRIX_USB3_GUSB3PIPECTL_VAL);

	/* Set USB2 PHY Configuration */
	//MWriteRegWord((volatile void*)0x1520c200, 0x00002540, 0xffffffff);
	WriteRegWord(XHCI_REG(TRIX_USB3_GUSB2PHYCFG), TRIX_USB3_GUSB2PHYCFG_VAL);

	/* Enable SS PHY REF clock */
	//MWriteRegWord((volatile void*)0x1520cd14, 0x200,0x200);
	MWriteRegWord(XHCI_REG(TRIX_USB3_CFG_REG2), 0x200, TRIX_USB3_CFG_REG2_USBPHY_REF_SSP_EN_MASK);

	/* Set USB3.0 PHY TX swing strength to full level */
	//MWriteRegWord((volatile void*)0x1520cd0c, 0x7f, 0x7f);
	MWriteRegWord(XHCI_REG(TRIX_USB3_PHY_TUNE_REG2), 0x7f, TRIX_USB3_PHY_TUNE_REG2_USBPHY_PCS_TX_SWING_FULL_MASK);

	/* Enable PHY power */
	MWriteRegWord(XHCI_REG(TRIX_USB3_TEST_DBG_REG), 0x00000000, 0xc0000000);

#elif defined(CONFIG_MACH_SIGMA_SX8)
	#warning "FIXME: SX8: Add code for XHCI controller init!!"
	/* Disable USB3 PHY Scramble */
	MWriteRegWord(XHCI_REG(TRIX_USB3_TEST_DBG_REG), 0x00202087, 0x00ffffff);
	MWriteRegWord(XHCI_REG(TRIX_USB3_TEST_DBG_REG), 0x00302087, 0x00ffffff);
	MWriteRegWord(XHCI_REG(TRIX_USB3_TEST_DBG_REG), 0x00202087, 0x00ffffff);
#endif

#ifdef CONFIG_MACH_SIGMA_SX6
	MWriteRegByte(0x1500ee20, 0x0, 0x7);	//[2:0]=3'b000
	MWriteRegByte(0x1500ee87, 0x4, 0x4);	//[2]=1'b1
	MWriteRegHWord(0x1b005520, 0x4, 0xc);	//[3:2]=2'b01
	MWriteRegHWord(0x1b005522, 0x2, 0x2);	//[1]=1'b1

#elif defined(CONFIG_MACH_SIGMA_SX7) || defined(CONFIG_MACH_SIGMA_SX8)
	/* Set GPIO9 pin as GPIO */
	MWriteRegByte(0x1500ee22, 0x0, 0x70);

	/* Set GPIO9 output mode */
	MWriteRegHWord(0x1b005520, 0x4, 0xc);

	/* Set GPIO9 output val equ 1 */
	MWriteRegHWord(0x1b005522, 0x2, 0x2);

	/* Set GPIO8 act as USB3_OC */
	MWriteRegByte(0x1500ee22, 0x2, 0x7);

	/* Set GPIO7 act as USB3_OC */
	MWriteRegByte(0x1500ee21, 0x20, 0x70);
#else
	#error "Unknow SOC type!!"
#endif


	return;
}

#ifdef CONFIG_DM_USB
int xhci_port_change(void)	
{
	uint32_t ret = 0, max_port = 0, i;
	volatile uint32_t status = 0;
	volatile uint32_t volatile *reg = NULL;

	struct xhci_hccr *hccr = (struct xhci_hccr *)(SIGMA_XHCI_BASE);
	struct xhci_hcor *hcor = (struct xhci_hcor *)((uint32_t) hccr
			+ HC_LENGTH(xhci_readl(&(hccr->cr_capbase))));

	printf("%s:hccr %x, hcor %x\n", __func__, (uint32_t)hccr, (uint32_t)hcor);
	max_port = HCS_MAX_PORTS(xhci_readl(&hccr->cr_hcsparams1));

	for (i=1; i<=max_port; i++) {
		reg = (volatile uint32_t *)
			(&hcor->portregs[i - 1].or_portsc);
		status = xhci_readl(reg);
		if (status & PORT_CONNECT) {
			printf("USB3 device found\n");
			ret = 1;
			goto out;
		} else {
			printf("No USB3 device found\n");
				ret = 0;
		}
	}
out:
	return ret;
}

static int xhci_trix_probe(struct udevice *dev)
{
	struct xhci_hccr *hccr;
	struct xhci_hcor *hcor;
	fdt_addr_t hcd_base;
	struct xhci_ctrl *ctrl = dev_get_priv(dev);

	usb3_power_init();
//	mdelay(200);
	memset(ctrl, 0, sizeof(struct xhci_ctrl));
	/*
	 * Get the base address for EHCI controller from the device node
	 */
	hcd_base = dev_get_addr(dev);
	if (hcd_base == FDT_ADDR_T_NONE) {
		debug("Can't get the EHCI register base address\n");
		return -ENXIO;
	}

	hccr = (struct xhci_hccr *)(hcd_base);
	hcor = (struct xhci_hcor *)
		((u32)hccr + HC_LENGTH(xhci_readl(&hccr->cr_capbase)));

	printf("xhci-trix: init hccr %x and hcor %x hc_length %d\n",
	      (u32)hccr, (u32)hcor,
	      (u32)HC_LENGTH(xhci_readl(&hccr->cr_capbase)));

	return xhci_register(dev, hccr, hcor);
}

static int xhci_trix_remove(struct udevice *dev)
{
	int ret;

	ret = xhci_deregister(dev);
	if (ret)
		return ret;

	return 0;
}

static const struct udevice_id xhci_usb_ids[] = {
	{ .compatible = "sigmadtv,trix-xhci", },
	{ }
};

U_BOOT_DRIVER(xhci_trix) = {
	.name	= "xhci_trix",
	.id	= UCLASS_USB,
	.of_match = xhci_usb_ids,
	.probe = xhci_trix_probe,
	.remove = xhci_trix_remove,
	.ops	= &xhci_usb_ops,
	.platdata_auto_alloc_size = sizeof(struct usb_platdata),
	.priv_auto_alloc_size = sizeof(struct xhci_ctrl),
	.flags	= DM_FLAG_ALLOC_PRIV_DMA,
};
#else /* CONFIG_DM_USB */
int xhci_hcd_init(int index, struct xhci_hccr **hccr, struct xhci_hcor **hcor)
{
	int ret = 0;

	usb333_power_init();

//	mdelay(200);

	*hccr = (struct xhci_hccr *)(SIGMA_XHCI_BASE);
	*hcor = (struct xhci_hcor *)((uint32_t) *hccr
				+ HC_LENGTH(xhci_readl(&(*hccr)->cr_capbase)));

	printf("sigma-xhci: init hccr %x and hcor %x hc_length %d\n",
	      (uint32_t)*hccr, (uint32_t)*hcor,
	      (uint32_t)HC_LENGTH(xhci_readl(&(*hccr)->cr_capbase)));

	return ret;
}

void xhci_hcd_stop(int index)
{
	return;
}

unsigned int finded_usb_device = 0;
#define SCAN_MAX_TIMES (15)
int is_update_from_usb(void)
{
	uint32_t auto_update = 1;
	int32_t ret = 0, max_port = 0, i;
	volatile uint32_t  status;
	volatile uint32_t volatile *reg;
	char *s = getenv("auto_update");


	struct xhci_hccr *hccr = (struct xhci_hccr *)(SIGMA_XHCI_BASE);
	struct xhci_hcor *hcor = (struct xhci_hcor *)((uint32_t) hccr
			+ HC_LENGTH(xhci_readl(&(hccr->cr_capbase))));

	printf("hccr %x, hcor %x\n", (uint32_t)hccr, (uint32_t)hcor);

	max_port = HCS_MAX_PORTS(xhci_readl(&hccr->cr_hcsparams1));
	debug("max_port = %d\n", max_port);

	if (s != NULL) {
		switch (*s) {
		case 'N':
		case 'n':
			auto_update = 0;
			break;

		case 'Y':
		case 'y':
		default:
			auto_update = 1;
			break;
		}
	}

	//int count = 0;
	if (auto_update) {

		usb_init();
		for (i=1; i<=max_port; i++) {

			reg = (volatile uint32_t *)
				(&hcor->portregs[i - 1].or_portsc);
			status = xhci_readl(reg);
			if (status & PORT_CONNECT) {
				printf("USB Device found\n");
				ret = 1;
				finded_usb_device = 1;
				run_command("dcache off", 0);
				goto end;
			} else {
			//	printf("USB Device not found\n");
				ret = 0;
			}
		}

	}

end:
	//debug("count = %d\n", count);
	return ret;
}
#endif /* CONFIG_DM_USB */
