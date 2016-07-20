#include <common.h>
#include <asm/io.h>
#include <usb.h>
#include "ehci.h"
#include <dm.h>

#include <asm/arch/reg_io.h>


#if defined (CONFIG_MACH_SIGMA_SX6) || defined (CONFIG_MACH_SIGMA_SX7) || defined (CONFIG_MACH_SIGMA_SX8)
#define USB0_BASE  0xf502f000
#define USB1_BASE  0xfb008000

#elif defined (CONFIG_MACH_SIGMA_UXLB)
#define USB0_BASE  0xfb003000
#define USB1_BASE  0xfb007000

#else
#error "Unknow SOC type"
#endif

void usb2_power_init(void)
{
#if defined (CONFIG_MACH_SIGMA_SX6) || defined (CONFIG_MACH_SIGMA_SX7)
	/* Set GPIO16 functionally as GPIO */
	MWriteRegByte(0xf500ee23, 0x00, 0x70);
	MWriteRegHWord(0xfb005540, 0x0001, 0x0003);
	
	/* Set GPIO16 output high */
	MWriteRegHWord(0xfb005542, 0x0001, 0x0001);

	/* Set GPIO6 functionally as GPIO */
	MWriteRegByte(0xf500ee1e, 0x00, 0x70);
	MWriteRegHWord(0xfb005500, 0x0100, 0x0300);
		
	/* Set GPIO6 output high */
	MWriteRegHWord(0xfb005502, 0x0040, 0x0040);

	/*usb2 clock control*/
	WriteRegByte(0xf500e84d, 0x08);

#elif defined (CONFIG_MACH_SIGMA_UXLB)

	/* Set GPIO7 output high */
	MWriteRegHWord(0x1b0055a0, 0x4000, 0x4000);
	MWriteRegHWord(0x1b0055a2, 0x0080, 0x0080);
	/* Set GPIO6 output high */
	MWriteRegHWord(0x1b0055a0, 0x1000, 0x1000);
	MWriteRegHWord(0x1b0055a2, 0x0040, 0x0040);

	/* Empty fifo flag */
	MWriteRegHWord(0x1b00007c, 0xffff, 0xffff);
	MWriteRegHWord(0x1b00007e, 0x1fff, 0xffff);
	MWriteRegHWord(0x1b000174, 0xffff, 0xffff);
	MWriteRegHWord(0x1b000176, 0x1fff, 0xffff);

	/* CORE MCLK */
	MWriteRegHWord(0x1b000092, 0x6a0f, 0xffff);

	/* Set USB DMA burst size */
	MWriteRegWord(0x1b003160, 0x404, 0xffffffff);
	MWriteRegWord(0x1b007160, 0x404, 0xffffffff);

	/* Enable USB DMA agent */
	MWriteRegWord(0x1502204c, 0x82, 0x0000ffff);
	MWriteRegWord(0x15022060, 0x82, 0x0000ffff);

	/* Select UMAC bridge ?? */
	MWriteRegHWord(0x1b000042, 0x0004, 0xffff);

	/* Endian select */
	MWriteRegHWord(0x1b00001c, 0x8000, 0xffff);

	/* Tuning USB PHY */
	MWriteRegHWord(0x1b000178, 0x1300, 0xffff);
	MWriteRegHWord(0x1b000180, 0x1310, 0xffff);
#elif defined (CONFIG_MACH_SIGMA_SX8)
	/*USB1 Power*/
	/*Set GPIO4 functionally as GPIO*/
	MWriteRegByte(0xf500ee20, 0x00, 0x07);
	MWriteRegHWord(0xfb005500, 0x0100, 0x0300);
		
	/* Set GPIO4 output high */
	MWriteRegHWord(0xfb005502, 0x0010, 0x0010);
	

	/*USB2 Power*/
	/*Set GPIO6 functionally as GPIO*/
	MWriteRegByte(0xf500ee21, 0x00, 0x07);
	MWriteRegHWord(0xfb005500, 0x1000, 0x3000);
		
	/* Set GPIO6 output high */
	MWriteRegHWord(0xfb005502, 0x0040, 0x0040);
#else
#error "Unknow SOC type"
#endif

}

static void usb_agent_init(void)
{

}


static void usb_endin_setting(void)
{
	/*endian setting */
//	WriteRegHWord(0x1b00001c,0x8000);
//	WriteRegHWord(0x1b000084,0x41A1);


}

#ifdef CONFIG_DM_USB
#define ehci_port_change(base)	({			\
	int _ret_ = 0;					\
	if (ReadRegWord(base + 0x184) & EHCI_PS_CS)	\
		_ret_ = 1;				\
	else						\
		_ret_ = 0;				\
	_ret_;						\
})

extern int xhci_port_change(void);

static int usb_stick_is_exist(void)
{
	char *s = getenv("usbport");


#if CONFIG_USB_EHCI_TRIX
	if(s != NULL && *s == '1') {
		return ehci_port_change(USB0_BASE);
	}
#endif

#if CONFIG_USB_XHCI_TRIX
	printf("1\n");
	if(s != NULL && *s == '3') {
	printf("2\n");
		return xhci_port_change();
	}
#endif

#if CONFIG_USB_EHCI_TRIX
	return ehci_port_change(USB1_BASE);
#endif

	/* No device exist */
	return 0;
}

int is_update_from_usb(void)
{

	uint32_t auto_update = 1;
	int32_t ret = 0;
	char *s = getenv("auto_update");

	usb2_power_init();

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

	if (auto_update) {
		ret = usb_stick_is_exist();
	}

	/* No usb device*/
	return ret;
}

static int ehci_trix_probe(struct udevice *dev)
{
	struct ehci_hccr *hccr;
	struct ehci_hcor *hcor;
	fdt_addr_t hcd_base;
	struct ehci_ctrl *ctrl = dev_get_priv(dev);

	memset(ctrl, 0, sizeof(struct ehci_ctrl));
	usb2_power_init();
	usb_agent_init();
	usb_endin_setting();
	/*
	 * Get the base address for EHCI controller from the device node
	 */
	hcd_base = dev_get_addr(dev);
	if (hcd_base == FDT_ADDR_T_NONE) {
		debug("Can't get the EHCI register base address\n");
		return -ENXIO;
	}

	hccr = (struct ehci_hccr *)(hcd_base + 0x100);
	hcor = (struct ehci_hcor *)
		((u32)hccr + HC_LENGTH(ehci_readl(&hccr->cr_capbase)));

	printf("ehci-trix: init hccr %x and hcor %x hc_length %d\n",
	      (u32)hccr, (u32)hcor,
	      (u32)HC_LENGTH(ehci_readl(&hccr->cr_capbase)));

	return ehci_register(dev, hccr, hcor, NULL, 0, USB_INIT_HOST);
}

static int ehci_trix_remove(struct udevice *dev)
{
	int ret;

	ret = ehci_deregister(dev);
	if (ret)
		return ret;

	return 0;
}

static const struct udevice_id ehci_usb_ids[] = {
	{ .compatible = "sigmadtv,trix-ehci", },
	{ }
};

U_BOOT_DRIVER(ehci_trix) = {
	.name	= "ehci_trix",
	.id	= UCLASS_USB,
	.of_match = ehci_usb_ids,
	.probe = ehci_trix_probe,
	.remove = ehci_trix_remove,
	.ops	= &ehci_usb_ops,
	.platdata_auto_alloc_size = sizeof(struct usb_platdata),
	.priv_auto_alloc_size = sizeof(struct ehci_ctrl),
	.flags	= DM_FLAG_ALLOC_PRIV_DMA,
};
#else /* CONFIG_DM_USB */

static int get_enabled_port(void)
{
        char *s = getenv("usbport");

	if (s && (*s == '1')) {
		return 0;
	} else if (s && (*s == '2')) {
		return 1;
	} else {
	/* No specified USB port for update*/
		return -1;
	}

	return -1;
}

static unsigned int do_get_usb_base(void)
{
        char *s = getenv("usbport");

        return (s && (*s == '1')) ? USB0_BASE : USB1_BASE;
}

int is_update_from_usb(void)
{
	uint32_t auto_update = 1;
	int32_t ret = 0;
	char *s = getenv("auto_update");
	uint32_t ehci_base = do_get_usb_base();

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

	if (auto_update) {
		run_command("dcache off", 0);
		usb_init();

		if (ReadRegWord(ehci_base + 0x184) & EHCI_PS_CS) {
			printf("USB Device found\n");
			ret = 1;
		} else {
			printf("USB Device not found\n");
			ret = 0;
			run_command("dcache on", 0);
		}
	}


	return ret;
}

/*
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 */
int ehci_hcd_init(int index, enum usb_init_type init,
                struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{

	int enabled_port = get_enabled_port();
	usb2_power_init();
	usb_agent_init();
	usb_endin_setting();

    if ( index == 0 ) {
	if (enabled_port != -1 && enabled_port != 0) {
	/* Skip initialize this controller */
		return -1;
	}

        *hccr = (struct ehci_hccr *)(USB0_BASE + 0x100);
        *hcor = (struct ehci_hcor *)((uint32_t) *hccr +
            HC_LENGTH(ehci_readl(&(*hccr)->cr_capbase)));
    }
    else if (index == 1 ) {
	if (enabled_port != -1 && enabled_port != 1) {
	/* Skip initialize this controller */
		return -1;
	}
        *hccr = (struct ehci_hccr *)(USB1_BASE + 0x100);
        *hcor = (struct ehci_hcor *)((uint32_t) *hccr +
            HC_LENGTH(ehci_readl(&(*hccr)->cr_capbase)));
    }
    else {
        printf("ehci-monza: wrong controller index!\n");
        return -1;
    }

    printf("ehci-monza: init hccr %x and hcor %x hc_length %d\n",
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
	return 0;
}
#endif
