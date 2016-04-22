#ifndef __TANGO4_API_H__
#define __TANGO4_API_H__

// switchbox (Host interface)
enum { 
	SBOX_MBUS_W0 = 0, SBOX_MBUS_W1, SBOX_PCIMASTER, SBOX_PCISLAVE, 
	SBOX_SATA0, SBOX_IDEFLASH, SBOX_IDEDVD, SBOX_SATA1, SBOX_MBUS_W2, SBOX_HOST_CIPHER, SBOX_MAX
};

// MBUS DMA 
typedef void (*mbus_irq_handler_t)(int irq, void *arg);

int em86xx_mbus_alloc_dma(int sbox, int fromdev, unsigned long *pregbase, int *pirq, int canwait);
void em86xx_mbus_free_dma(unsigned long regbase, int sbox);
int em86xx_mbus_setup_dma(unsigned int regbase, unsigned int addr, unsigned int count, mbus_irq_handler_t handler, void *arg, unsigned int flags);

#endif
