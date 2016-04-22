/*
 * (C) Copyright 2009 Stefan Roese <sr@denx.de>, DENX Software Engineering
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
#define DEBUG

#include <common.h>
#include <usb.h>
#include "ehci.h"
//#include "ehci-core.h"

#include <asm/io.h>
#include <linux/compiler.h>

#if defined (CONFIG_TANGO3) || defined (CONFIG_TANGO4)
#include <asm/arch/setup.h>
#endif


/* USB */
#define TANGOX_EHCI0_BASE   (REG_BASE_host_interface + 0x1400)
#define TANGOX_OHCI0_BASE   (REG_BASE_host_interface + 0x1500)
#define TANGOX_CTRL0_BASE   (REG_BASE_host_interface + 0x1700)
#define TANGOX_EHCI0_IRQ    (IRQ_CONTROLLER_IRQ_BASE + LOG2_CPU_USB_EHCI_INT)
#define TANGOX_OHCI0_IRQ    (IRQ_CONTROLLER_IRQ_BASE + LOG2_CPU_USB_OHCI_INT)
#define TANGOX_EHCI1_BASE   (REG_BASE_host_interface + 0x5400)
#define TANGOX_OHCI1_BASE   (REG_BASE_host_interface + 0x5500)
#define TANGOX_CTRL1_BASE   (REG_BASE_host_interface + 0x5700)
#define TANGOX_EHCI1_IRQ    (IRQ_CONTROLLER_IRQ_BASE + LOG2_CPU_USB_EHCI_INT-1)
#define TANGOX_OHCI1_IRQ    (IRQ_CONTROLLER_IRQ_BASE + LOG2_CPU_USB_OHCI_INT)

/* For 8652/867X OTG host or 8646 host */
#define TANGOX_EHCI_REG_OFFSET      0x100
#define TANGOX_USB_MODE             0x1A8


/* for 8670 has two controllers */
static const int tangox_ehci_base[2] = {TANGOX_EHCI0_BASE, TANGOX_EHCI1_BASE};
static const int tangox_ohci_base[2] = {TANGOX_OHCI0_BASE, TANGOX_OHCI1_BASE};
static const int tangox_ctrl_base[2] = {TANGOX_CTRL0_BASE, TANGOX_CTRL1_BASE};

int tangox_ehci_hcd_init(int ctrl);
int tangox_ehci_hcd_stop(int index);

extern unsigned long tangox_chip_id(void);

inline void wait_ms( unsigned int val )
{
	mdelay( val );
}


/*
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 */
int ehci_hcd_init(int index, enum usb_init_type init,
		struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	tangox_ehci_hcd_init(index);

    if ( index == 0 ) {
        *hccr = (struct ehci_hccr *)(TANGOX_EHCI0_BASE + TANGOX_EHCI_REG_OFFSET);
        *hcor = (struct ehci_hcor *)((uint32_t) *hccr +
            HC_LENGTH(ehci_readl(&(*hccr)->cr_capbase)));
    }
    else if (index == 1 ) {
        *hccr = (struct ehci_hccr *)(TANGOX_EHCI1_BASE + TANGOX_EHCI_REG_OFFSET);
        *hcor = (struct ehci_hcor *)((uint32_t) *hccr +
            HC_LENGTH(ehci_readl(&(*hccr)->cr_capbase)));
    }
    else {
        printf("ehci-tangox: wrong controller index!\n");
        return -1;
    }

    
    debug("tangox-ehci: init hccr %x and hcor %x hc_length %d\n",
        (uint32_t)*hccr, (uint32_t)*hcor,
        (uint32_t)HC_LENGTH(ehci_readl(&(*hccr)->cr_capbase)));
    
	return 0;
}

/*
 * Destroy the appropriate control structures corresponding
 * the the EHCI host controller.
 */
int ehci_hcd_stop(int index)
{
    tangox_ehci_hcd_stop( index );
    
	return 0;
}

/*
 * Initialize the Tango3 EHCI controller and PHY.
 */
int tangox_ehci_hcd_init(int ctrl)
{
    unsigned long chip_id = (tangox_chip_id() >> 16) & 0xfffe;
	unsigned long temp;
    
     /*check see if it's inited*/
    temp = gbus_read_reg32(tangox_ctrl_base[ctrl] + 0x0);
    if (temp & (1<<19)) {
        printf("TangoX USB was initialized.\n");
        return 0;
    }
    else
        printf("TangoX USB initializing...\n");


    /* Reset USB host(s), redo USB initialization */
    gbus_write_reg32(tangox_ctrl_base[ctrl] + 0x0, gbus_read_reg32(tangox_ctrl_base[ctrl] + 0x0) | 0x2);
	mdelay(2);
	gbus_write_reg32(tangox_ctrl_base[ctrl] + 0x0, gbus_read_reg32(tangox_ctrl_base[ctrl] + 0x0) & ~0x2);
	mdelay(2);
    

    /*
	1. Program the clean divider and clock multiplexer to provide 
	   a 48 MHz reference to the USB block.
	   This is done in bootloader.
	*/

    if ((chip_id == 0x8734) || (chip_id == 0x8756) || 
        (chip_id == 0x2400) || (chip_id == 0x8758)) {
        printf("Initializing 87xx USB Host%d base at 0x%x\n", ctrl, tangox_ctrl_base[ctrl]);	
		/* 0. Program the clean divider and clock multiplexors to provide 48MHz clock reference*/
		/* this is to be done in zboot */

	    /* Reset PLL and set clock mux */
		temp = 0;
		temp = 1<<13 | 1<<7 | 1<<1 | 1<<0;
		//temp &= ~(1<<28);
		gbus_write_reg32(tangox_ctrl_base[ctrl] + 0x0, temp);
		wait_ms(5);

	    /* Release PHY suspend */
		temp &= (~(1<<7));
		gbus_write_reg32(tangox_ctrl_base[ctrl] + 0x0, temp);
		wait_ms(5);

	    /* Release PHY reset */
		temp &= (~(1<<0));
		gbus_write_reg32(tangox_ctrl_base[ctrl] + 0x0, temp);
		wait_ms(5);

	    /* Release Link reset */
		temp &= (~(1<<1));
		gbus_write_reg32(tangox_ctrl_base[ctrl] + 0x0, temp);
		wait_ms(5);

		/* set it to host mode*/
		temp = gbus_read_reg32(tangox_ehci_base[ctrl] + TANGOX_EHCI_REG_OFFSET +0xA8);
		temp |= 3;
		gbus_write_reg32(tangox_ehci_base[ctrl] + TANGOX_EHCI_REG_OFFSET +0xA8, temp);
		wait_ms(20);
    }
    else {
        printf("Unsupported model!\n");
    }
    
    /* increase gbus bandwidth for USB hosts */
	gbus_write_reg32(REG_BASE_system_block + 0x138,
		(gbus_read_reg32(REG_BASE_system_block + 0x138) & 0xffffff00) | 0x3f);

    return 0;
}

int tangox_ehci_hcd_stop(int index)
{
    return 0;
}
