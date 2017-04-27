//#define DEBUG

#include <common.h>
#include <command.h>
#include <asm/byteorder.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <fis.h>
#include <libata.h>
#include <ata.h>
#include <linux/compiler.h>
#include <stdbool.h>
#include "sata_tangox.h"

#include <asm/arch/setup.h>
#include <asm/arch/xenvkeys.h>


// architect dependent macro redefine at here
#if defined (CONFIG_MIPS)
#   define NON_CACHED(a)           KSEG1ADDR(a)
#elif defined (CONFIG_ARM)
#   define NON_CACHED(a)           (a)
#endif

//#define CONFIG_LBA48i

/* addr and val has been switched in our arch header (mips/tango) */
#ifdef out_le32
#undef out_le32
#define out_le32( addr, val)    writel( val, addr )
#endif



//////////////////////////////////////////////////////////////////////////////
// this block comes from /arch/mips/include/asm/tango3/platform_dev.h
/* SATA */

#define DRV_NAME0    "Tangox SATA 0"
#define DRV_NAME1    "Tangox SATA 1"

#define TANGOX_SATA0_BASE       /*NON_CACHED*/(REG_BASE_host_interface + 0x3000)
#define TANGOX_SATA1_BASE       /*NON_CACHED*/(REG_BASE_host_interface + 0x3800)
#define TANGOX_SATA0_CTL_BASE   NON_CACHED(REG_BASE_host_interface + 0x4000)
#define TANGOX_SATA1_CTL_BASE   NON_CACHED(REG_BASE_host_interface + 0x4040)
//#define TANGOX_SATA_IRQ0        IRQ_CONTROLLER_IRQ_BASE  + LOG2_CPU_SATA_INT
//#define TANGOX_SATA_DMA_IRQ0    IRQ_CONTROLLER_IRQ_BASE  + LOG2_CPU_DMASATA_INT
//#define TANGOX_SATA_IRQ1        IRQ_CONTROLLER_IRQ_BASE  + LOG2_CPU_SATA1_INT
//#define TANGOX_SATA_DMA_IRQ1    IRQ_CONTROLLER_IRQ_BASE  + LOG2_CPU_DMASATA1_INT

//////////////////////////////////////////////////////////////////////////////////

struct sata_dwc_regs {
    u32 fptagr;
    u32 fpbor;
    u32 fptcr;
    u32 dmacr;
    u32 dbtsr;
    u32 intpr;
    u32 intmr;
    u32 errmr;
    u32 llcr;
    u32 phycr;
    u32 physr;
    u32 rxbistpd;
    u32 rxbistpd1;
    u32 rxbistpd2;
    u32 txbistpd;
    u32 txbistpd1;
    u32 txbistpd2;
    u32 bistcr;
    u32 bistfctr;
    u32 bistsr;
    u32 bistdecr;
    u32 res[15];
    u32 testr;
    u32 versionr;
    u32 idr;
    u32 unimpl[192];
    u32 dmadr[256];
};

/*HSATA Registers*/
#define HSATA_SCR0_REG              0x0024
#define HSATA_SCR1_REG              0x0028
#define HSATA_SCR2_REG              0x002C
#define HSATA_SCR3_REG              0x0030
#define HSATA_SCR4_REG              0x0034
#define HSATA_SERROR_REG            HSATA_SCR1_REG
#define HSATA_SCONTROL_REG          HSATA_SCR2_REG
#define HSATA_SACTIVE_REG           HSATA_SCR3_REG

#define HSATA_DMACR_TX_EN           (0x01 /*| HSATA_DMACR_TXMODE_BIT*/)
#define HSATA_DMACR_RX_EN           (0x02 /*| HSATA_DMACR_TXMODE_BIT*/)
#define HSATA_DMACR_TXRX_EN         (0x03 | HSATA_DMACR_TXMODE_BIT)
#define HSATA_DMACR_TXMODE_BIT      0x04
#define HSATA_FEAT_REG              0x0004
#define HSATA_CMD_REG               0x001c
#define HSATA_CONTROL_REG           0x0020
#define HSATA_DMACR_REG             0x0070
#define HSATA_DBTSR_REG             0x0074
#define HSATA_INTPR_REG             0x0078
#define HSATA_INTPR_ERR_BIT         0x00000008
#define HSATA_INTPR_FP_BIT          0x00000002  /* new DMA setup FIS arrived */
#define HSATA_INTMR_REG             0x007C
#define HSATA_INTMR_ERRM_BIT        0x00000008
#define HSATA_INTMR_NEWFP_BIT       0x00000002
#define HSATA_ERRMR_REG             0x0080
#define HSATA_ERRMR_BITS            0xFFFEF7FF

#define HSATA_VER_REG               0x00F8
#define HSATA_IDR_REG               0x00FC

#if 1
#define SATA_DWC_TXFIFO_DEPTH       0x01FF
#define SATA_DWC_RXFIFO_DEPTH       0x01FF

#define SATA_DWC_DBTSR_MWR(size)    ((size / 4) & SATA_DWC_TXFIFO_DEPTH)
#define SATA_DWC_DBTSR_MRD(size)    (((size / 4) &  \
                    SATA_DWC_RXFIFO_DEPTH) << 16)
#define SATA_DWC_INTPR_DMAT         0x00000001
#define SATA_DWC_INTPR_NEWFP        0x00000002
#define SATA_DWC_INTPR_PMABRT       0x00000004
#define SATA_DWC_INTPR_ERR          0x00000008
#define SATA_DWC_INTPR_NEWBIST      0x00000010
#define SATA_DWC_INTPR_IPF          0x10000000
#define SATA_DWC_INTMR_DMATM        0x00000001
#define SATA_DWC_INTMR_NEWFPM       0x00000002
#define SATA_DWC_INTMR_PMABRTM      0x00000004
#define SATA_DWC_INTMR_ERRM         0x00000008
#define SATA_DWC_INTMR_NEWBISTM     0x00000010

#define SATA_DWC_DMACR_TMOD_TXCHEN  0x00000004
#define SATA_DWC_DMACR_TXRXCH_CLEAR SATA_DWC_DMACR_TMOD_TXCHEN

#define SATA_DWC_QCMD_MAX           32

#define SATA_DWC_SERROR_ERR_BITS    0x0FFF0F03

#endif

#define sata_readl(reg)         gbus_read_uint32( pgbus, (unsigned int)reg);
#define sata_writel(val,reg)    gbus_write_uint32( pgbus, (unsigned int)reg, val);
#define sata_readw(reg)         gbus_read_uint16( pgbus, (unsigned int)reg);
#define sata_writew(val,reg)    gbus_write_uint16( pgbus, (unsigned int)reg, val);
#define sata_readb(reg)         gbus_read_uint8(pgbus, (unsigned int)reg);
#define sata_writeb(val,reg)    gbus_write_uint8( pgbus, (unsigned int)reg, val);

#define SATA_TANGOX_QCMD_MAX        32
#define ATA_SECTOR_WORDS            (ATA_SECT_SIZE/2)

#define DMA_EN                      0x00000001
#define DMA_DI                      0x00000000
#define DMA_CHANNEL(ch)	            (0x00000001 << (ch))
#define DMA_ENABLE_CHAN(ch)         ((0x00000001 << (ch)) |	\
                                    ((0x000000001 << (ch)) << 8))
#define DMA_DISABLE_CHAN(ch)        (0x00000000 | 	\
                                    ((0x000000001 << (ch)) << 8))

//#define SATA_DWC_MAX_PORTS  1
#define SATA_DWC_SCR_OFFSET 0x24
#define SATA_DWC_REG_OFFSET 0x64

#if 0
struct sata_tangox_device {
    struct device           *dev;
    struct ata_probe_ent    *pe;
    struct ata_host         *host;
    u8                      *reg_base;
    unsigned long           ctl_base;
    struct sata_dwc_regs *sata_dwc_regs;
    int                     irq_dma;
};
#endif

struct hsata_device {
    struct ata_device       *device;    /* shouls be first element for HSDEV_FROM_ATADEV */
    struct ata_host         *host;
    struct sata_dwc_regs    *sata_dwc_regs;
    unsigned long           *reg_base;
    int                     dev_state;   /* what's stat of device, one of enum sata_dev_state */
};

static struct hsata_device  hsata_dev[CONFIG_SYS_SATA_MAX_DEVICE];
static struct ata_port      ata_ports[CONFIG_SYS_SATA_MAX_DEVICE];
static struct ata_host      ata_hosts[CONFIG_SYS_SATA_MAX_DEVICE];
static struct ata_device    ata_devices[CONFIG_SYS_SATA_MAX_DEVICE];


#define HSDEV_FROM_AP(ap) (struct hsata_device*)ap->private_data

#if 0
struct sata_tangox_device_port {
    struct sata_tangox_device	*hsdev;
    int cmd_issued[SATA_TANGOX_QCMD_MAX];
    u32	dma_chan[SATA_TANGOX_QCMD_MAX];
    int	dma_pending[SATA_TANGOX_QCMD_MAX];
};
#endif

enum {
    SATA_DWC_CMD_ISSUED_NOT     = 0,
    SATA_DWC_CMD_ISSUED_PEND    = 1,
    SATA_DWC_CMD_ISSUED_EXEC    = 2,
    SATA_DWC_CMD_ISSUED_NODATA  = 3,

    SATA_DWC_DMA_PENDING_NONE   = 0,
    SATA_DWC_DMA_PENDING_TX     = 1,
    SATA_DWC_DMA_PENDING_RX     = 2,
};


#define ATA_TMOUT_INTERNAL		(30 * 100)
static int ata_probe_timeout = (ATA_TMOUT_INTERNAL / 100);

enum sata_dev_state {
	SATA_INIT = 0,
	SATA_READY = 1,
	SATA_NODEVICE = 2,
	SATA_ERROR = 3,
};
enum sata_dev_state dev_state = SATA_INIT;


static const int tangox_mem_base[2] = {TANGOX_SATA0_BASE, TANGOX_SATA1_BASE};
static const int tangox_ctl_base[2] = {TANGOX_SATA0_CTL_BASE, TANGOX_SATA1_CTL_BASE};
//static const int tangox_sata_irq[2] = {TANGOX_SATA_IRQ0, TANGOX_SATA_IRQ1};
//static const int tangox_sata_dma_irq[2] = {TANGOX_SATA_DMA_IRQ0, TANGOX_SATA_DMA_IRQ1};
///static const int tangox_sbox[2] = {SBOX_SATA0, SBOX_SATA1};
///static const int tangox_aes_config[2] = {HC_SATA0_AES_CONFIG, HC_SATA1_AES_CONFIG};

//static struct sata_info tangox_sata_info[CONFIG_SYS_SATA_MAX_DEVICE];

/* Throttle to gen1 speed */
static int gen1only = 0;

//static struct ahb_dma_regs		*sata_dma_regs = 0;
//static struct ata_host			*phost;
//static struct ata_port			ap;
//static struct ata_port			*pap = &ap;
//static struct ata_device		ata_device;
//static struct sata_tangox_device_port	tangox_devp;



static void	*scr_addr_sstatus;
static u32	temp_n_block = 0;

extern block_dev_desc_t sata_dev_desc[CONFIG_SYS_SATA_MAX_DEVICE];

static const struct ata_port_info sata_tangox_port_info[] = {
    {
        .flags      = ATA_FLAG_SATA      |
                      ATA_FLAG_NO_LEGACY | /* no legacy mode check */
                      ATA_FLAG_SRST      | /* use ATA SRST, not E.D.D. */
                      ATA_FLAG_MMIO      | /* use MMIO, not PortIO */
                      ATA_FLAG_PIO_POLLING |
//#ifndef HSATA_TANGOX_DMA
                      //ATA_DFLAG_PIO      | /* set to NOT use DMA */
//#endif
                      ATA_FLAG_NCQ       |
                      //ATA_DFLAG_LBA48    | /* READ/WRITE EXT support */
                      0x00,
        .pio_mask	= 0x1f,     /* pio0-4   - IDENTIFY DEVICE word 63 */
//#ifdef HSATA_TANGOX_DMA
//        .mwdma_mask	= 0x07,     /* mwdma0-2 - IDENTIFY DEVICE word 64 */
//        .udma_mask	= 0x7f,     /* udma0-6  - IDENTIFY DEVICE word 88 */
//#else
        .mwdma_mask	= 0x00,
        .udma_mask	= 0x00,
//#endif
    },
    //TODO: add one more for the port2 later
};

//
//
// function proto types
//
//

extern int xenv_get(u32 *base, u32 size, char *recordname, void *dst, u32 *datasize);

#if 1
static unsigned ata_exec_internal(struct ata_device *dev,
			struct ata_taskfile *tf, const u8 *cdb,
			int dma_dir, unsigned int buflen,
			unsigned long timeout, int curr_dev);
#else
static unsigned ata_exec_internal(struct hsata_device *hsdev,
			struct ata_taskfile *tf, const u8 *cdb,
			int dma_dir, unsigned int buflen,
			unsigned long timeout);
#endif

static unsigned int ata_qc_issue_prot(struct ata_queued_cmd *qc);

static void ata_qc_issue(struct ata_queued_cmd *qc);

static void ata_tf_to_host(struct ata_port *ap,
			const struct ata_taskfile *tf);

static void ata_tf_load(struct ata_port *ap,
			const struct ata_taskfile *tf);

static void ata_exec_command(struct ata_port *ap,
			const struct ata_taskfile *tf);

static void ata_pio_queue_task(struct ata_port *ap,
			void *data,unsigned long delay);

static void ata_qc_complete(struct ata_queued_cmd *qc);

static void ata_tf_read(struct ata_port *ap, struct ata_taskfile *tf);

static void __ata_qc_complete(struct ata_queued_cmd *qc);

static void ata_qc_free(struct ata_queued_cmd *qc);

static unsigned int ata_dev_init_params(struct ata_device *dev,
			u16 heads, u16 sectors, int curr_dev);

static unsigned int ata_dev_set_feature(struct ata_device *dev,
			u8 enable,u8 feature, int curr_dev);

static u8 ata_check_status(struct ata_port *ap);

//static int check_sata_dev_state(void);
static int check_sata_dev_state(int);

static int ata_dev_read_sectors(unsigned char* pdata,
			unsigned long datalen, u32 block, u32 n_block, u32 curr_dev);

static int ata_dev_write_sectors(unsigned char* pdata,
            unsigned long datalen , u32 block, u32 n_block, u32 curr_dev);

static struct ata_queued_cmd *__ata_qc_from_tag(struct ata_port *ap,
			unsigned int tag);

static int ata_hsm_move(struct ata_port *ap, struct ata_queued_cmd *qc,
			u8 status, int in_wq);

static void __ata_port_freeze(struct ata_port *ap);

static u8 ata_irq_on(struct ata_port *ap);

static int ata_port_freeze(struct ata_port *ap);

static void ata_pio_sectors(struct ata_queued_cmd *qc);

static void ata_pio_sector(struct ata_queued_cmd *qc);

static void ata_mmio_data_xfer(struct ata_device *dev,
			unsigned char *buf,
			unsigned int buflen,int do_write, struct ata_port* ap);

int reset_sata(int dev)
{
	return 0;
}

static u8 ata_check_altstatus(struct ata_port *ap)
{
	u8 val = 0;
	val = sata_readb(ap->ioaddr.altstatus_addr);
	return val;
}

static u8 ata_check_status(struct ata_port *ap)
{
	u8 val = 0;

    //debug( "   @@@@@ status_addr: 0x%x\n", (u32)ap->ioaddr.status_addr);
	val = sata_readb(ap->ioaddr.status_addr);
	return val;
}

//
// let's say this function returns 0 for noraml,
// and -EIO for no device error
//
static int sata_tangox_softreset(struct ata_port *ap)
{
	u8 nsect,lbal = 0;
	u8 tmp = 0;
	struct ata_ioports *ioaddr = &ap->ioaddr;
    struct hsata_device *hsdev;

    hsdev = HSDEV_FROM_AP(ap);

	in_le32((void *)ap->ioaddr.scr_addr + (SCR_ERROR * 4));

	writeb(0x55, ioaddr->nsect_addr);
	writeb(0xaa, ioaddr->lbal_addr);
	writeb(0xaa, ioaddr->nsect_addr);
	writeb(0x55, ioaddr->lbal_addr);
	writeb(0x55, ioaddr->nsect_addr);
	writeb(0xaa, ioaddr->lbal_addr);

    nsect = readb(ioaddr->nsect_addr);
	lbal = readb(ioaddr->lbal_addr);

	if ((nsect == 0x55) && (lbal == 0xaa)) {
		printf("Device found\n");
	} else {

        hsdev = HSDEV_FROM_AP(ap);
		//printf("No device found\n");
        hsdev->dev_state = SATA_NODEVICE;
		//dev_state = SATA_NODEVICE;
		//return false;
        return -EIO;
	}

	tmp = ATA_DEVICE_OBS;
	writeb(tmp, ioaddr->device_addr);
	writeb(ap->ctl, ioaddr->ctl_addr);

	udelay(200);

	writeb(ap->ctl | ATA_SRST, ioaddr->ctl_addr);

	udelay(200);
	writeb(ap->ctl, ioaddr->ctl_addr);

	mdelay(150);
	ata_check_status(ap);

	mdelay(50);
	ata_check_status(ap);

	while (1) {
		u8 status = ata_check_status(ap);

		if (!(status & ATA_BUSY))
			break;

		printf("Hard Disk status is BUSY.\n");

		mdelay(50);
	}


	tmp = ATA_DEVICE_OBS;
	writeb(tmp, ioaddr->device_addr);

	nsect = readb(ioaddr->nsect_addr);
	lbal  = readb(ioaddr->lbal_addr);

    return 0;
}

static u8 ata_busy_wait(struct ata_port *ap,
		unsigned int bits,unsigned int max)
{
	u8 status;
//debug(" %s:%d \n", __FUNCTION__, __LINE__);

	do {
		udelay(10);
		status = ata_check_status(ap);
		max--;
	} while (status != 0xff && (status & bits) && (max > 0));
//printf("    max: %d\n");
	return status;
}

static u8 ata_wait_idle(struct ata_port *ap)
{

	u8 status = ata_busy_wait(ap, ATA_BUSY | ATA_DRQ, 1000);
    //printf("st: 0x%x\n", status );
//debug(" %s:%d \n", __FUNCTION__, __LINE__);

    return status;
}

static void ata_std_dev_select(struct ata_port *ap, unsigned int device)
{
	u8 tmp;
//debug(" %s:%d \n", __FUNCTION__, __LINE__);
//debug("    device : %d\n", device );
	if (device == 0) {
		tmp = ATA_DEVICE_OBS;
	} else {
		tmp = ATA_DEVICE_OBS | ATA_DEV1;
	}

	writeb(tmp, ap->ioaddr.device_addr);

	readb(ap->ioaddr.altstatus_addr);

    udelay(1);
}

static void ata_dev_select(struct ata_port *ap, unsigned int device,
		unsigned int wait, unsigned int can_sleep)
{
    debug(" %s:%d \n", __FUNCTION__, __LINE__);

	if (wait)
		ata_wait_idle(ap);

	ata_std_dev_select(ap, device);

	if (wait)
		ata_wait_idle(ap);
}

static int check_sata_dev_state( int dev )
{
	unsigned long datalen;
	unsigned char *pdata;
	int ret = 0;
	int i = 0;
	char temp_data_buf[512];

    debug(" %s:%d \n", __FUNCTION__, __LINE__);

	while (1) {
		udelay(10000);

		pdata = (unsigned char*)&temp_data_buf[0];
		datalen = 512;

		ret = ata_dev_read_sectors(pdata, datalen, 0, 1, dev);

		if (ret == true)
			break;

		i++;
		if (i > (ATA_RESET_TIME * 100)) {
			printf("** TimeOUT **\n");
			//dev_state = SATA_NODEVICE;
            hsata_dev[dev].dev_state = SATA_NODEVICE;
			return false;
		}

		if ((i >= 100) && ((i % 100) == 0))
			printf(".");
	}

	//dev_state = SATA_READY;
    hsata_dev[dev].dev_state = SATA_READY;

	return true;
}

static unsigned int ata_dev_set_feature(struct ata_device *dev,
				u8 enable, u8 feature, int curr_dev)
{
	struct ata_taskfile tf;
	struct ata_port     *ap = &ata_ports[curr_dev];
	//ap = pap;
	unsigned int        err_mask;

    debug(" %s:%d \n", __FUNCTION__, __LINE__);

	memset(&tf, 0, sizeof(tf));
	tf.ctl = ap->ctl;

	tf.device = ATA_DEVICE_OBS;
	tf.command = ATA_CMD_SET_FEATURES;
	tf.feature = enable;
	tf.flags |= ATA_TFLAG_ISADDR | ATA_TFLAG_DEVICE;
	tf.protocol = ATA_PROT_NODATA;
	tf.nsect = feature;

#if 1
	err_mask = ata_exec_internal(dev, &tf, NULL, DMA_NONE, 0, 0, curr_dev);
#else
	err_mask = ata_exec_internal( hsdev, &tf, NULL, DMA_NONE, 0, 0 );
#endif

	return err_mask;
}


static unsigned int ata_dev_init_params(struct ata_device *dev,
				u16 heads, u16 sectors, int curr_dev)
{
	struct ata_taskfile tf;
	struct ata_port *ap = &ata_ports[curr_dev];
	//ap = pap;
	unsigned int err_mask;

    debug(" %s:%d \n", __FUNCTION__, __LINE__);

	if (sectors < 1 || sectors > 255 || heads < 1 || heads > 16)
		return AC_ERR_INVALID;

	memset(&tf, 0, sizeof(tf));
	tf.ctl = ap->ctl;
	tf.device = ATA_DEVICE_OBS;
	tf.command = ATA_CMD_INIT_DEV_PARAMS;
	tf.flags |= ATA_TFLAG_ISADDR | ATA_TFLAG_DEVICE;
	tf.protocol = ATA_PROT_NODATA;
	tf.nsect = sectors;
	tf.device |= (heads - 1) & 0x0f;

#if 1
	err_mask = ata_exec_internal(dev, &tf, NULL, DMA_NONE, 0, 0, curr_dev);
#else
	err_mask = ata_exec_internal( hsdev, &tf, NULL, DMA_NONE, 0, 0 );
#endif

	if (err_mask == AC_ERR_DEV && (tf.feature & ATA_ABORTED))
		err_mask = 0;

	return err_mask;
}



static int ata_dev_read_id( /*struct hsata_device *hsdev,*/ struct ata_device *dev,
                            unsigned int      *p_class,
		                    unsigned int      flags,
                            u16               *id,
                            int               curr_dev )
{
	//struct ata_port      *ap    = pap;
    struct ata_port       *ap   = /*hsdev->host->ports[0];*/ &ata_ports[curr_dev];
    //struct ata_device     *dev  = hsdev->device;
	unsigned int          class = *p_class;
	struct ata_taskfile   tf;
	unsigned int          err_mask = 0;
	const char            *reason;
	int                   may_fallback = 1, tried_spinup = 0;
	u8                    status;
	int                   rc;
    unsigned int          id_cnt;


    debug(" %s:%d \n", __FUNCTION__, __LINE__ );

	status = ata_busy_wait( ap, ATA_BUSY, 30000 );
	if ( status & ATA_BUSY ) {
		printf("BSY = 0 check. timeout. (%d)\n", __LINE__);
		rc = false;
		return rc;
	}

	ata_dev_select( ap, dev->devno, 1, 1 );

retry:
	memset(&tf, 0, sizeof(tf));
	ap->print_id = 1;
	ap->flags &= ~ATA_FLAG_DISABLED;
	tf.ctl = ap->ctl;
	tf.device = ATA_DEVICE_OBS;
	tf.command = ATA_CMD_ID_ATA;
	tf.protocol = ATA_PROT_PIO;

	/* Some devices choke if TF registers contain garbage.  Make
	 * sure those are properly initialized.
	 */
	tf.flags |= ATA_TFLAG_ISADDR | ATA_TFLAG_DEVICE;

	/* Device presence detection is unreliable on some
	 * controllers.  Always poll IDENTIFY if available.
	 */
	tf.flags |= ATA_TFLAG_POLLING;

	temp_n_block = 1;

#if 1
	err_mask = ata_exec_internal(dev, &tf, NULL, DMA_FROM_DEVICE,
					sizeof(id[0]) * ATA_ID_WORDS, 0, curr_dev);
#else
	err_mask = ata_exec_internal( hsdev, &tf, NULL, DMA_FROM_DEVICE,
					sizeof(id[0]) * ATA_ID_WORDS, 0 );
#endif

    debug("    err_mask: %d\n", err_mask );

	if (err_mask) {
		if (err_mask & AC_ERR_NODEV_HINT) {
			printf("NODEV after polling detection\n");
			return -ENOENT;
		}

		if ((err_mask == AC_ERR_DEV) && (tf.feature & ATA_ABORTED)) {
			/* Device or controller might have reported
			 * the wrong device class.  Give a shot at the
			 * other IDENTIFY if the current one is
			 * aborted by the device.
			 */
			if (may_fallback) {
				may_fallback = 0;

				if (class == ATA_DEV_ATA) {
					class = ATA_DEV_ATAPI;
				} else {
					class = ATA_DEV_ATA;
				}
				goto retry;
			}
			/* Control reaches here iff the device aborted
			 * both flavors of IDENTIFYs which happens
			 * sometimes with phantom devices.
			 */
			printf("both IDENTIFYs aborted, assuming NODEV\n");
			return -ENOENT;
		}
		rc = -EIO;
		reason = "I/O error";
		goto err_out;
	}

	/* Falling back doesn't make sense if ID data was read
	 * successfully at least once.
	 */
	may_fallback = 0;

	for (id_cnt = 0; id_cnt < ATA_ID_WORDS; id_cnt++)
		id[id_cnt] = le16_to_cpu(id[id_cnt]);


	rc = -EINVAL;
	reason = "device reports invalid type";

	if (class == ATA_DEV_ATA) {
		if (!ata_id_is_ata(id) && !ata_id_is_cfa(id))
			goto err_out;
	} else {
		if (ata_id_is_ata(id))
			goto err_out;
	}

    debug("    spinup: %d\n", tried_spinup );

    if (!tried_spinup && (id[2] == 0x37c8 || id[2] == 0x738c)) {
		tried_spinup = 1;
		/*
		 * Drive powered-up in standby mode, and requires a specific
		 * SET_FEATURES spin-up subcommand before it will accept
		 * anything other than the original IDENTIFY command.
		 */
#if 1
		err_mask = ata_dev_set_feature(dev, SETFEATURES_SPINUP, 0, curr_dev);
#else // should  change the code properly after debugging
		err_mask = ata_dev_set_feature(dev, SETFEATURES_SPINUP, 0, 0 );
#endif
		if (err_mask && id[2] != 0x738c) {
			rc = -EIO;
			reason = "SPINUP failed";
			goto err_out;
		}
		/*
		 * If the drive initially returned incomplete IDENTIFY info,
		 * we now must reissue the IDENTIFY command.
		 */
		if (id[2] == 0x37c8)
			goto retry;
	}

    debug("    flags: 0x%x\n", flags );
    debug("    class: 0x%x\n", class );

    if ((flags & ATA_READID_POSTRESET) && class == ATA_DEV_ATA) {
		/*
		 * The exact sequence expected by certain pre-ATA4 drives is:
		 * SRST RESET
		 * IDENTIFY (optional in early ATA)
		 * INITIALIZE DEVICE PARAMETERS (later IDE and ATA)
		 * anything else..
		 * Some drives were very specific about that exact sequence.
		 *
		 * Note that ATA4 says lba is mandatory so the second check
		 * shoud never trigger.
		 */
		if (ata_id_major_version(id) < 4 || !ata_id_has_lba(id)) {
#if 1
			err_mask = ata_dev_init_params(dev, id[3], id[6], curr_dev);
#else // should change the code after debugging
			err_mask = ata_dev_init_params(dev, id[3], id[6], 0);
#endif
			if (err_mask) {
				rc = -EIO;
				reason = "INIT_DEV_PARAMS failed";
				goto err_out;
			}

			/* current CHS translation info (id[53-58]) might be
			 * changed. reread the identify device info.
			 */
			flags &= ~ATA_READID_POSTRESET;
			goto retry;
		}
	}

	*p_class = class;
	return 0;

err_out:
	printf("failed to READ ID (%s, err_mask=0x%x)\n", reason, err_mask);
	return rc;
}

extern unsigned long *xenv_mem;
static unsigned int tangox_get_sata_channel_cfg( unsigned int* cfg )
{
    unsigned int length;
    int          ret;

    /* set ethernet related zxenv at here */
    length = sizeof(unsigned int *);
    ret = xenv_get( (void *)xenv_mem,
                    MAX_XENV_SIZE,
                    XENV_KEY_SATA_CHANNEL_CFG,
                    cfg,
                    &length );

    if ( ret != RM_OK ) {
       printf("Fail to get SATA cof. Set default...\n");
#if defined(CONFIG_SATA_SD8670)
    *cfg = 0x00008527;
#elif defined(CONFIG_SATA_SD8654)
    *cfg = 0x00008057;
#endif
    }

    return 0;
}

static void tangox_sata_init(void)
{
    unsigned int val;
    unsigned int cfg = 0;
    //int tangox_get_sata_channel_cfg(unsigned int *);
    static int sata_init = 0;

    if (sata_init != 0)
        return;

    sata_init = 1;

    /* grab sata channel info from zxenv */
    if( tangox_get_sata_channel_cfg(&cfg) < 0 ){
        printf("fail to get sata channel info from zxenv\n");
        return;
    }

    debug(" sata ch cfg: 0x%x\n", cfg );

    /* bit14: force gen1? */
    gen1only = (cfg & (1 << 14)) ? 1 : 0; /* force gen1 speed? */

    val = sata_readl((void *)(TANGOX_SATA0_CTL_BASE + 0x0c));

    /* bit15: internal clock? */
    if (cfg & (1 << 15))
        val |= (1 << 24);   /* internal clock routing */
    else
        val &= ~(1 << 24);  /* external clock is used */
    val = (val & 0xff0fffff) | ((cfg & 0x0f00) << 12);  /* TX edge rate control */

    sata_writel(val, (void *)(TANGOX_SATA0_CTL_BASE + 0x0c));
    debug("PHY stat1(0x%x)=0x%x\n", TANGOX_SATA0_CTL_BASE + 0x0c, val);

    /*
     bit0: RX SSC port0
     bit1: RX SSC port1
     bit2: TX SSC port0/1
     */
    sata_writel(0x28903 | ((cfg & 1) ? 0x200 : 0) | ((cfg & 2) ? 0x1000 : 0),
        (void *)(TANGOX_SATA0_CTL_BASE + 0x10));

    val = sata_readl((void *)(TANGOX_SATA0_CTL_BASE + 0x14));
    val &= ~0x7fe;
    val |= ((cfg & 4) ? 0x400 : 0); /* TX SSC enable or not */

    /* bit7..4: reference clock frequency */
    switch ((cfg >> 4) & 0xf) {
        case 0: /* 120MHz ref clock */
            val |= 0x12c;
            break;
        case 2: /* 60MHz ref clock */
            val |= 0x128;
            break;
        case 4: /* 30MHz ref clock */
            val |= 0x12a;
            break;
        case 1: /* 100MHz ref clock */
            val |= 0x234;
            break;
        case 3: /* 50MHz ref clock */
            val |= 0x230;
            break;
        case 5: /* 25MHz ref clock */
            val |= 0x232;
            break;
        default:
            debug("Invalid frequency selection specified: %d\n", (cfg >> 4) & 0xf);
            val |= 0x12c;
            break;
    }

    sata_writel(val, (void *)(TANGOX_SATA0_CTL_BASE + 0x14));

    val = sata_readl((void *)(TANGOX_SATA0_CTL_BASE + 0x10));
    debug("PHY stat2(0x%x)=0x%x\n", TANGOX_SATA0_CTL_BASE + 0x10, val);

    val = sata_readl((void *)(TANGOX_SATA0_CTL_BASE + 0x14));
    debug("PHY stat3(0x%x)=0x%x\n", TANGOX_SATA0_CTL_BASE + 0x14, val);

    val = sata_readl((void *)(TANGOX_SATA0_CTL_BASE + 0x18));
    debug("PHY stat4(0x%x)=0x%x\n", TANGOX_SATA0_CTL_BASE + 0x18, val);

    val |= 1<<16; /* fast tech */
    val |= 1<<18; /* 3.3 v */
    debug("Setting PHY stat4(0x%x) to 0x%x\n", (TANGOX_SATA0_CTL_BASE + 0x18), val);

    sata_writel(val, (void *)(TANGOX_SATA0_CTL_BASE + 0x18));

    val = sata_readl((void *)(TANGOX_SATA0_CTL_BASE + 0x18));
    debug("PHY stat4(0x%x)=0x%x\n", TANGOX_SATA0_CTL_BASE + 0x18, val);
}


static void hsata_host_init(int dev)
{
    unsigned long membase;
    unsigned int pid, ver;
    u32 val32;

    membase = NON_CACHED(tangox_mem_base[dev]);

    /* Read IDR and Version registers*/
    pid = sata_readl((void *)(membase + HSATA_IDR_REG));
    ver = sata_readl((void *)(membase + HSATA_VER_REG));

    debug("SATA version 0x%x ID 0x%x is detected\n", ver, pid);

    /* some other initializations here*/
    tangox_sata_init();

    /* Enable IPM */
    val32 = sata_readl((void *) (membase + HSATA_SCR2_REG));
    sata_writel((val32 & ~(0x3 << 8)), (void *)(membase + HSATA_SCR2_REG));

    /*
     * We clear this bit here so that we can later on check to see if other bus
     * errors occured (during debug of this driver).
     */
    val32 = sata_readl((void *)(membase + HSATA_SERROR_REG));
    sata_writel(val32, (void *)(membase + HSATA_SERROR_REG));

}

//#define SATA_TANGOX_REG_OFFSET 0x64_
/*
 * SATA interface between low level driver and command layer
 */
int init_sata(int dev)
{
	struct ata_port_info pi = sata_tangox_port_info[0];
	struct ata_link *link;
    unsigned long base;

    ///u8 *sata_dma_regs_addr = 0;
	u8 status;
	//int chan = 0;
	int rc;
	int i;
    //u16 word;

#if 0 // temporary block
    if (tangox_sata_enabled() == 0) {
        printk("TangoX SATA support is disabled from XENV.\n");
        return -EINVAL;
    }
#endif

    // TODO: we need some sanity check like below
    if ( dev > CONFIG_SYS_SATA_MAX_DEVICE-1 )
        return 1;

    //printf( "hsdev: 0x%x\n", &hsata_dev[0] );
    //printf( "macro: 0x%x\n", HSDEV_FROM_ATADEV(&ata_devices[0]) );

    /* initialize device state */
    hsata_dev[dev].dev_state  = SATA_INIT;
    hsata_dev[dev].host       = &ata_hosts[dev];
    hsata_dev[dev].device     = &ata_devices[dev];

    ata_ports[dev].private_data = &hsata_dev[dev];

    debug("SATA dev%d mem base: 0x%x\n", dev, tangox_mem_base[dev] );

    //phost = &host;

	base = NON_CACHED(tangox_mem_base[dev]);
    scr_addr_sstatus = (void *)(base + SATA_DWC_SCR_OFFSET);
	//hsdev.sata_dwc_regs = (void __iomem *)(base + SATA_DWC_REG_OFFSET);
    hsata_dev[dev].sata_dwc_regs = (void __iomem *)(base + SATA_DWC_REG_OFFSET);

	//host.n_ports = CONFIG_SYS_SATA_MAX_DEVICE;
    ata_hosts[dev].n_ports = CONFIG_SYS_SATA_MAX_DEVICE;

#if 0
    //for (i = 0; i < SATA_DWC_MAX_PORTS; i++) {
		ap.pflags   |= ATA_PFLAG_INITIALIZING;
		ap.flags     = ATA_FLAG_DISABLED;
		ap.print_id  = -1;
		ap.ctl       = ATA_DEVCTL_OBS;
		ap.host      = &host;
		ap.last_ctl  = 0xFF;
#endif
	ata_ports[dev].pflags   |= ATA_PFLAG_INITIALIZING;
    ata_ports[dev].flags     = ATA_FLAG_DISABLED;
	ata_ports[dev].print_id  = -1;
	ata_ports[dev].ctl       = ATA_DEVCTL_OBS;
	ata_ports[dev].host      = &ata_hosts[dev];
	ata_ports[dev].last_ctl  = 0xFF;

#if 0
		link             = &ap.link;
		link->ap         = &ap;
		link->pmp        = 0;
		link->active_tag = ATA_TAG_POISON;
		link->hw_sata_spd_limit = 0;
#endif

    link                     = &ata_ports[dev].link;
    link->ap                 = &ata_ports[dev];
    link->pmp                = 0;
    link->active_tag         = ATA_TAG_POISON;
    link->hw_sata_spd_limit  = 0;

#if 0
		//ap.port_no = i;
		//host.ports[i] = &ap;
		ap.port_no      = dev;
		host.ports[0]   = &ap;
#endif

    ata_ports[dev].port_no   = dev;
    ata_hosts[dev].ports[0]  = &ata_ports[dev];

    //printf("@@@@@@@@@ ata_port: 0x%x\n", hsata_dev[dev].host->ports[0] );

    //host[dev]
	//}
#if 0
	ap.pio_mask    = pi.pio_mask;
	ap.mwdma_mask  = pi.mwdma_mask;
	ap.udma_mask   = pi.udma_mask;
	ap.flags      |= pi.flags;
	ap.link.flags |= pi.link_flags;
#endif
	ata_ports[dev].pio_mask    = pi.pio_mask;
	ata_ports[dev].mwdma_mask  = pi.mwdma_mask;
	ata_ports[dev].udma_mask   = pi.udma_mask;
	ata_ports[dev].flags      |= pi.flags;
	ata_ports[dev].link.flags |= pi.link_flags;

    // TODO: think about it....should we have two copies of ata_device?
    ata_devices[dev].link  = link;
    ata_devices[dev].devno = 0; /* it is alwasys 0 for SATA. 0: master 1:slave */

    ///sata_dma_regs_addr = (u8*)SATA_DMA_REG_ADDR;
	///sata_dma_regs = (void *__iomem)sata_dma_regs_addr;

    /* setup port */

	ata_hosts[dev].ports[0]->ioaddr.cmd_addr       = (void __iomem *)(base + 0x00);
	ata_hosts[dev].ports[0]->ioaddr.data_addr      = (void __iomem *)(base + 0x00);

    ata_hosts[dev].ports[0]->ioaddr.error_addr     = (void __iomem *)(base + 0x04);
	ata_hosts[dev].ports[0]->ioaddr.feature_addr   = (void __iomem *)(base + 0x04);

	ata_hosts[dev].ports[0]->ioaddr.nsect_addr     = (void __iomem *)(base + 0x08);

	ata_hosts[dev].ports[0]->ioaddr.lbal_addr      = (void __iomem *)(base + 0x0c);
	ata_hosts[dev].ports[0]->ioaddr.lbam_addr      = (void __iomem *)(base + 0x10);
	ata_hosts[dev].ports[0]->ioaddr.lbah_addr      = (void __iomem *)(base + 0x14);

	ata_hosts[dev].ports[0]->ioaddr.device_addr    = (void __iomem *)(base + 0x18);
	ata_hosts[dev].ports[0]->ioaddr.command_addr   = (void __iomem *)(base + 0x1c);
	ata_hosts[dev].ports[0]->ioaddr.status_addr    = (void __iomem *)(base + 0x1c);
	ata_hosts[dev].ports[0]->ioaddr.altstatus_addr = (void __iomem *)(base + 0x20);
	ata_hosts[dev].ports[0]->ioaddr.ctl_addr       = (void __iomem *)(base + 0x20);
    ata_hosts[dev].ports[0]->ioaddr.scr_addr       = (void __iomem *)(base + HSATA_SCR0_REG);


    hsata_host_init(dev);

	printf("Waiting for dev%d...", dev);

    status = ata_check_altstatus( &ata_ports[dev] );
    debug( "status: 0x%x\n", status );
	if (status == 0x7f) {
		printf("Hard Disk not found.\n");
		//dev_state = SATA_NODEVICE;
        hsata_dev[dev].dev_state = SATA_NODEVICE;
        rc = false;
		return rc;
	}

	i = 0;
	while (1) {
		udelay(10000);

		status = ata_check_altstatus( &ata_ports[dev] );

		if ((status & ATA_BUSY) == 0) {
			//printf("\n");
			break;
		}

		i++;
		if (i > (ATA_RESET_TIME * 100)) {
			printf("** TimeOUT **\n");

            //dev_state = SATA_NODEVICE;
            hsata_dev[dev].dev_state = SATA_NODEVICE;
			rc = false;
			return rc;
		}
		if ((i >= 100) && ((i % 100) == 0))
			printf(".");
	}

    rc = sata_tangox_softreset( &ata_ports[dev] );

	if (rc < 0) {
		//printf("sata_dwc : error. soft reset failed\n");
        printf("No device\n");
		return rc;
	}

#if 0 // this is specific to ppc460ex, looka like just disabling the dma.
	for (chan = 0; chan < DMA_NUM_CHANS; chan++) {
		out_le32(&(sata_dma_regs->interrupt_mask.error.low),
				DMA_DISABLE_CHAN(chan));

		out_le32(&(sata_dma_regs->interrupt_mask.tfr.low),
				DMA_DISABLE_CHAN(chan));
	}

	out_le32(&(sata_dma_regs->dma_cfg.low), DMA_DI);
#endif

    debug("   intmr: 0x%x\n", (unsigned int)(&hsata_dev[dev].sata_dwc_regs->intmr) );

    out_le32( &hsata_dev[dev].sata_dwc_regs->intmr,
               SATA_DWC_INTMR_ERRM | SATA_DWC_INTMR_PMABRTM );

    /* Unmask the error bits that should trigger
	 * an error interrupt by setting the error mask register.
	 */
	out_le32( &hsata_dev[dev].sata_dwc_regs->errmr,
              SATA_DWC_SERROR_ERR_BITS );

#if 0 // temporary block until I know how below code affect to rest of the code
	hsdev.host = ap.host;
	memset(&hsdevp, 0, sizeof(hsdevp));
	hsdevp.hsdev = &hsdev;
#endif

#if 0 // temporary blocl until I know what it does
	for (i = 0; i < SATA_DWC_QCMD_MAX; i++)
		hsdevp.cmd_issued[i] = SATA_DWC_CMD_ISSUED_NOT;
printf("==(3)==\n");
#endif

	out_le32((void __iomem *)scr_addr_sstatus + 4,
		in_le32((void __iomem *)scr_addr_sstatus + 4));

	rc = 0;

	return rc;

}


static int ata_id_has_hipm(const u16 *id)
{
	u16 val = id[76];

	if (val == 0 || val == 0xffff)
		return -1;

	return val & (1 << 9);
}

static int ata_id_has_dipm(const u16 *id)
{
	u16 val = id[78];

	if (val == 0 || val == 0xffff)
		return -1;

	return val & (1 << 3);
}


/*
 * SATA interface between low level driver and command layer
 */
int scan_sata( int dev )
{
    int               i;
	int               rc;
	u8                status;
	const u16         *id;
	struct ata_device *ata_dev = &ata_devices[dev];
    unsigned long     pio_mask, mwdma_mask;
	char              revbuf[7];
	u16               iobuf[ATA_SECTOR_WORDS];

    printf("Scanning SATA dev%d...\n", dev);

    memset(iobuf, 0, sizeof(iobuf));

    if( hsata_dev[dev].dev_state == SATA_NODEVICE) {
        //printf("No device!\n");
        return 1;
    }

    //printf("Waiting for device...");

	i = 0;

	while (1) {
		udelay(10000);

		status = ata_check_altstatus( &ata_ports[dev] );

		if ((status & ATA_BUSY) == 0) {
			//printf("\n");
			break;
		}

		i++;
		if (i > (ATA_RESET_TIME * 100)) {
			printf("** TimeOUT **\n");

			//dev_state = SATA_NODEVICE;
            hsata_dev[dev].dev_state= SATA_NODEVICE;
			return 1;
		}
		if ((i >= 100) && ((i % 100) == 0))
			printf(".");
	}

	udelay(1000);

	rc = ata_dev_read_id( /*&hsata_dev[dev],*/ ata_dev,
                          &ata_dev->class,
			              ATA_READID_POSTRESET,
                          ata_dev->id,
                          dev );

    if (rc) {
		printf("sata_dwc : error. failed sata scan\n");
		return 1;
	}

	/* SATA drives indicate we have a bridge. We don't know which
	 * end of the link the bridge is which is a problem
	 */
	if ( ata_id_is_sata(ata_dev->id) )
		ata_ports[dev].cbl = ATA_CBL_SATA;

	id = ata_dev->id;

	ata_dev->flags       &= ~ATA_DFLAG_CFG_MASK;
	ata_dev->max_sectors  = 0;
	ata_dev->cdb_len      = 0;
	ata_dev->n_sectors    = 0;
	ata_dev->cylinders    = 0;
	ata_dev->heads        = 0;
	ata_dev->sectors      = 0;

	if (id[ATA_ID_FIELD_VALID] & (1 << 1)) {
		pio_mask = id[ATA_ID_PIO_MODES] & 0x03;
		pio_mask <<= 3;
		pio_mask |= 0x7;
	} else {
		/* If word 64 isn't valid then Word 51 high byte holds
		 * the PIO timing number for the maximum. Turn it into
		 * a mask.
		 */
		u8 mode = (id[ATA_ID_OLD_PIO_MODES] >> 8) & 0xFF;
		if (mode < 5) {
			pio_mask = (2 << mode) - 1;
		} else {
			pio_mask = 1;
		}
	}

	mwdma_mask = id[ATA_ID_MWDMA_MODES] & 0x07;

	if ( ata_id_is_cfa(id) ) {
		int pio = id[163] & 0x7;
		int dma = (id[163] >> 3) & 7;

		if (pio)
			pio_mask |= (1 << 5);
		if (pio > 1)
			pio_mask |= (1 << 6);
		if (dma)
			mwdma_mask |= (1 << 3);
		if (dma > 1)
			mwdma_mask |= (1 << 4);
	}

	if (ata_dev->class == ATA_DEV_ATA) {
		if ( ata_id_is_cfa(id) ) {
			if ( id[162] & 1 )
				printf("supports DRM functions and may "
					"not be fully accessable.\n");
			sprintf(revbuf, "%s", "CFA");
		} else {
			if ( ata_id_has_tpm(id) )
				printf("supports DRM functions and may "
						"not be fully accessable.\n");
		}

		ata_dev->n_sectors = ata_id_n_sectors((u16*)id);

		if ( ata_dev->id[59] & 0x100 )
			ata_dev->multi_count = ata_dev->id[59] & 0xff;

		if ( ata_id_has_lba(id) ) {
			char ncq_desc[20];

            debug("    has LBA...\n");

            ata_dev->flags |= ATA_DFLAG_LBA;

            if ( ata_id_has_lba48(id) ) {
				ata_dev->flags |= ATA_DFLAG_LBA48;

                debug("    has LBA48...\n");

                if ( ata_dev->n_sectors >= (1UL << 28) &&
					 ata_id_has_flush_ext(id))
					 ata_dev->flags |= ATA_DFLAG_FLUSH_EXT;
			}
			if ( !ata_id_has_ncq(ata_dev->id) )
				ncq_desc[0] = '\0';

			if ( ata_dev->horkage & ATA_HORKAGE_NONCQ )
				sprintf(ncq_desc, "%s", "NCQ (not used)");

			if ( ata_ports[dev].flags & ATA_FLAG_NCQ)
				 ata_dev->flags |= ATA_DFLAG_NCQ;
		}
		ata_dev->cdb_len = 16;
	}

	ata_dev->max_sectors = ATA_MAX_SECTORS;

	if (ata_dev->flags & ATA_DFLAG_LBA48)
		ata_dev->max_sectors = ATA_MAX_SECTORS_LBA48;

	if ( !(ata_dev->horkage & ATA_HORKAGE_IPM) ) {
		if ( ata_id_has_hipm( ata_dev->id ) )
			ata_dev->flags |= ATA_DFLAG_HIPM;
		if ( ata_id_has_dipm( ata_dev->id ) )
			ata_dev->flags |= ATA_DFLAG_DIPM;
	}

	if ( (ata_ports[dev].cbl == ATA_CBL_SATA) && ( !ata_id_is_sata(ata_dev->id)) ) {
		ata_dev->udma_mask  &= ATA_UDMA5;
		ata_dev->max_sectors = ATA_MAX_SECTORS;
	}

	if ( ata_dev->horkage & ATA_HORKAGE_DIAGNOSTIC ) {
		printf("Drive reports diagnostics failure."
				"This may indicate a drive\n");
		printf("fault or invalid emulation."
				"Contact drive vendor for information.\n");
	}

	//rc = check_sata_dev_state();
    rc = check_sata_dev_state( dev );

	ata_id_c_string(ata_dev->id,
			(unsigned char *)sata_dev_desc[dev].revision,
			 ATA_ID_FW_REV, sizeof(sata_dev_desc[dev].revision));
	ata_id_c_string(ata_dev->id,
			(unsigned char *)sata_dev_desc[dev].vendor,
			 ATA_ID_PROD, sizeof(sata_dev_desc[dev].vendor));
	ata_id_c_string(ata_dev->id,
			(unsigned char *)sata_dev_desc[dev].product,
			 ATA_ID_SERNO, sizeof(sata_dev_desc[dev].product));

	sata_dev_desc[dev].lba = (u32) ata_dev->n_sectors;

#ifdef CONFIG_LBA48
	if (ata_dev->id[83] & (1 << 10)) {
		sata_dev_desc[dev].lba48 = 1;
	} else {
		sata_dev_desc[dev].lba48 = 0;
	}
#endif

	return 0;
}

static void ata_qc_reinit(struct ata_queued_cmd *qc)
{
	qc->dma_dir = DMA_NONE;
	qc->flags = 0;
	qc->nbytes = qc->extrabytes = qc->curbytes = 0;
	qc->n_elem = 0;
	qc->err_mask = 0;
	qc->sect_size = ATA_SECT_SIZE;
	qc->nbytes = ATA_SECT_SIZE * temp_n_block;

	memset(&qc->tf, 0, sizeof(qc->tf));
	qc->tf.ctl = 0;
	qc->tf.device = ATA_DEVICE_OBS;

	qc->result_tf.command = ATA_DRDY;
	qc->result_tf.feature = 0;
}

static int waiting_for_reg_state(volatile u8 *offset,
				int timeout_msec,
				u32 sign)
{
	int i;
	u32 status;

    debug(" %s:%d \n", __FUNCTION__, __LINE__ );
    //debug("    offset: 0x%x\n", offset );

	for (i = 0; i < timeout_msec; i++) {
		status = readl(offset);
		if ((status & sign) != 0)
			break;
		mdelay(1);
	}
    //debug("    i: %d\n", i );
	return (i < timeout_msec) ? 0 : -1;
}

static void ata_pio_sectors(struct ata_queued_cmd *qc)
{
	struct ata_port *ap;

    //debug("%s:%d \n", __FUNCTION__, __LINE__ );

    //ap = pap;
    ap = qc->ap;

	qc->pdata = ap->pdata;
	ata_pio_sector(qc);

	readb(qc->ap->ioaddr.altstatus_addr);
	udelay(1);
}

static void ata_pio_sector(struct ata_queued_cmd *qc)
{
	int                 do_write = (qc->tf.flags & ATA_TFLAG_WRITE);
	struct ata_port     *ap = qc->ap;
	unsigned int        offset;
	unsigned char       *buf;
	char                temp_data_buf[512];


	if (qc->curbytes == qc->nbytes - qc->sect_size)
		ap->hsm_task_state = HSM_ST_LAST;

	offset = qc->curbytes;

	switch (qc->tf.command) {
	case ATA_CMD_ID_ATA:
		//buf = (unsigned char *)&ata_device.id[0];
        buf = (unsigned char *)&qc->dev->id[0];
		break;
	case ATA_CMD_PIO_READ_EXT:
	case ATA_CMD_PIO_READ:
	case ATA_CMD_PIO_WRITE_EXT:
	case ATA_CMD_PIO_WRITE:
		buf = qc->pdata + offset;
		//printf("    buf+offset: 0x%x\n", buf+offset);
        break;
	default:
        printf("... case default\n");
		buf = (unsigned char *)&temp_data_buf[0];
	}

	ata_mmio_data_xfer(qc->dev, buf, qc->sect_size, do_write, ap);

	qc->curbytes += qc->sect_size;

}

static unsigned int ac_err_mask(u8 status)
{
	if (status & (ATA_BUSY | ATA_DRQ))
		return AC_ERR_HSM;
	if (status & (ATA_ERR | ATA_DF))
		return AC_ERR_DEV;
	return 0;
}

static unsigned int __ac_err_mask(u8 status)
{
	unsigned int mask = ac_err_mask(status);
	if (mask == 0)
		return AC_ERR_OTHER;
	return mask;
}

#if 0
static void ata_pio_task(struct ata_port *arg_ap)
{
	struct ata_port *ap = arg_ap;
	struct ata_queued_cmd *qc = ap->port_task_data;
	u8 status;
	int poll_next;

fsm_start:
	/*
	 * This is purely heuristic.  This is a fast path.
	 * Sometimes when we enter, BSY will be cleared in
	 * a chk-status or two.  If not, the drive is probably seeking
	 * or something.  Snooze for a couple msecs, then
	 * chk-status again.  If still busy, queue delayed work.
	 */
	status = ata_busy_wait(ap, ATA_BUSY, 5);
	if (status & ATA_BUSY) {
		mdelay(2);
		status = ata_busy_wait(ap, ATA_BUSY, 10);
		if (status & ATA_BUSY) {
            printf("+"); mdelay(2);goto fsm_start;
			ata_pio_queue_task(ap, qc, ATA_SHORT_PAUSE);
			return;
		}
	}

	poll_next = ata_hsm_move(ap, qc, status, 1);

	/* another command or interrupt handler
	 * may be running at this point.
	 */
	if (poll_next)
		goto fsm_start;
}

#else

static void ata_pio_task(struct ata_port *arg_ap)
{
	struct ata_port *ap = arg_ap;
	struct ata_queued_cmd *qc = ap->port_task_data;
	u8 status;
	int poll_next;

    int loop = 100;

fsm_start:
	/*
	 * This is purely heuristic.  This is a fast path.
	 * Sometimes when we enter, BSY will be cleared in
	 * a chk-status or two.  If not, the drive is probably seeking
	 * or something.  Snooze for a couple msecs, then
	 * chk-status again.  If still busy, queue delayed work.
	 */

    /* original code put the task in the queue and return, if device
       is not ready within 2.15msec. it doesn't make sense to me becuase
       we are polling the device. we should wait until we got either
       device ready (i.e. ATA_BUSY is cleared) or error.

       right now, simply wait for ready state during 100 iterations
       to see if how it works.
     */
	status = ata_busy_wait(ap, ATA_BUSY, 5);
	if (status & ATA_BUSY) {
		mdelay(2);
		status = ata_busy_wait(ap, ATA_BUSY, 10);
		if (status & ATA_BUSY) {
            if( loop > 0 ){
                loop--;
                mdelay(2);
                goto fsm_start;
            }else{
                printf("ata_pio_task: timeout! (0x%x)\n", (status & 0xff) );
                return;
            }
		}
	}

	poll_next = ata_hsm_move(ap, qc, status, 1);

	/* another command or interrupt handler
	 * may be running at this point.
	 */
	if (poll_next)
		goto fsm_start;
}

#endif

static void ata_mmio_data_xfer(struct ata_device *dev, unsigned char *buf,
				unsigned int buflen, int do_write, struct ata_port* ap)
{
	//struct ata_port *ap = pap;

	void __iomem *data_addr = ap->ioaddr.data_addr;
	unsigned int words = buflen >> 1;
	u16 *buf16 = (u16 *)buf;
	unsigned int i = 0;

	udelay(100);
//printf(" lbal: 0x%x\n", readb(ap->ioaddr.lbal_addr) );
//printf("st: 0x%x\n", readb(ap->ioaddr.altstatus_addr) );
	if (do_write) {
		for (i = 0; i < words; i++)
			writew(le16_to_cpu(buf16[i]), data_addr);
	} else {
		for (i = 0; i < words; i++)
			buf16[i] = cpu_to_le16(readw(data_addr));
	}

	if (buflen & 0x01) {
		__le16 align_buf[1] = { 0 };
		unsigned char *trailing_buf = buf + buflen - 1;

		if (do_write) {
			memcpy(align_buf, trailing_buf, 1);
			writew(le16_to_cpu(align_buf[0]), data_addr);
		} else {
			align_buf[0] = cpu_to_le16(readw(data_addr));
			memcpy(trailing_buf, align_buf, 1);
		}
	}
}


static void ata_hsm_qc_complete(struct ata_queued_cmd *qc, int in_wq)
{
	struct ata_port *ap = qc->ap;

	if (in_wq) {
		/* EH might have kicked in while host lock is
		 * released.
		 */
		qc = &ap->qcmd[qc->tag];
		if (qc) {
			if (!(qc->err_mask & AC_ERR_HSM)) {
				ata_irq_on(ap);
				ata_qc_complete(qc);
			} else {
				ata_port_freeze(ap);
			}
		}
	} else {
		if (!(qc->err_mask & AC_ERR_HSM)) {
			ata_qc_complete(qc);
		} else {
			ata_port_freeze(ap);
		}
	}
}

static u8 ata_irq_on(struct ata_port *ap)
{
	struct ata_ioports *ioaddr = &ap->ioaddr;
	u8 tmp;

	ap->ctl &= ~ATA_NIEN;
	ap->last_ctl = ap->ctl;

	if (ioaddr->ctl_addr)
		writeb(ap->ctl, ioaddr->ctl_addr);

	tmp = ata_wait_idle(ap);

	return tmp;
}

static int ata_hsm_move(struct ata_port *ap, struct ata_queued_cmd *qc,
			u8 status, int in_wq)
{
	int poll_next;

fsm_start:
	switch (ap->hsm_task_state) {
	case HSM_ST_FIRST:
		poll_next = (qc->tf.flags & ATA_TFLAG_POLLING);

		if ((status & ATA_DRQ) == 0) {
			if (status & (ATA_ERR | ATA_DF)) {
				qc->err_mask |= AC_ERR_DEV;
			} else {
				qc->err_mask |= AC_ERR_HSM;
			}

            debug("   move to HSM_ST_ERR (1)\n");

            ap->hsm_task_state = HSM_ST_ERR;
			goto fsm_start;
		}

		/* Device should not ask for data transfer (DRQ=1)
		 * when it finds something wrong.
		 * We ignore DRQ here and stop the HSM by
		 * changing hsm_task_state to HSM_ST_ERR and
		 * let the EH abort the command or reset the device.
		 */
		if (status & (ATA_ERR | ATA_DF)) {
			if (!(qc->dev->horkage & ATA_HORKAGE_STUCK_ERR)) {
				printf("DRQ=1 with device error, "
					"dev_stat 0x%X\n", status);
				qc->err_mask |= AC_ERR_HSM;
				ap->hsm_task_state = HSM_ST_ERR;
                goto fsm_start;
			}
		}

		if (qc->tf.protocol == ATA_PROT_PIO) {
			/* PIO data out protocol.
			 * send first data block.
			 */
			/* ata_pio_sectors() might change the state
			 * to HSM_ST_LAST. so, the state is changed here
			 * before ata_pio_sectors().
			 */
			ap->hsm_task_state = HSM_ST;
			ata_pio_sectors(qc);
		} else {
			printf("protocol is not ATA_PROT_PIO \n");
		}
		break;

	case HSM_ST:
		if ((status & ATA_DRQ) == 0) {
			if (status & (ATA_ERR | ATA_DF)) {
				qc->err_mask |= AC_ERR_DEV;
			} else {
				/* HSM violation. Let EH handle this.
				 * Phantom devices also trigger this
				 * condition.  Mark hint.
				 */
				qc->err_mask |= AC_ERR_HSM | AC_ERR_NODEV_HINT;
			}

            debug("    move to HSM_ST_ERR (2)\n");

            ap->hsm_task_state = HSM_ST_ERR;
			goto fsm_start;
		}
		/* For PIO reads, some devices may ask for
		 * data transfer (DRQ=1) alone with ERR=1.
		 * We respect DRQ here and transfer one
		 * block of junk data before changing the
		 * hsm_task_state to HSM_ST_ERR.
		 *
		 * For PIO writes, ERR=1 DRQ=1 doesn't make
		 * sense since the data block has been
		 * transferred to the device.
		 */
		if (status & (ATA_ERR | ATA_DF)) {
			qc->err_mask |= AC_ERR_DEV;

			if (!(qc->tf.flags & ATA_TFLAG_WRITE)) {
				ata_pio_sectors(qc);
				status = ata_wait_idle(ap);
			}

			if (status & (ATA_BUSY | ATA_DRQ))
				qc->err_mask |= AC_ERR_HSM;

			/* ata_pio_sectors() might change the
			 * state to HSM_ST_LAST. so, the state
			 * is changed after ata_pio_sectors().
			 */
            debug("    move to HSM_ST_ERR (3)\n");

            ap->hsm_task_state = HSM_ST_ERR;
			goto fsm_start;
		}

		ata_pio_sectors(qc);
		if (ap->hsm_task_state == HSM_ST_LAST &&
			(!(qc->tf.flags & ATA_TFLAG_WRITE))) {
			status = ata_wait_idle(ap);
			goto fsm_start;
		}

		poll_next = 1;
		break;

	case HSM_ST_LAST:
		if (!ata_ok(status)) {
			qc->err_mask |= __ac_err_mask(status);
			ap->hsm_task_state = HSM_ST_ERR;

            debug("    move to HSM_ST_ERR (4)\n");
			goto fsm_start;
		}

		ap->hsm_task_state = HSM_ST_IDLE;

        debug("    state HSM_ST_LAST\n");
		ata_hsm_qc_complete(qc, in_wq);

		poll_next = 0;
		break;

	case HSM_ST_ERR:
		/* make sure qc->err_mask is available to
		 * know what's wrong and recover
		 */
		ap->hsm_task_state = HSM_ST_IDLE;

        debug("    state HSM_ST_ERR\n");
		ata_hsm_qc_complete(qc, in_wq);

		poll_next = 0;
		break;
	default:
		poll_next = 0;
	}

	return poll_next;
}


struct ata_queued_cmd *__ata_qc_from_tag(struct ata_port *ap,
					unsigned int tag)
{
	if (tag < ATA_MAX_QUEUE)
		return &ap->qcmd[tag];
	return NULL;
}

static void __ata_port_freeze(struct ata_port *ap)
{
	debug("set port freeze.\n");
	ap->pflags |= ATA_PFLAG_FROZEN;
}

static int ata_port_freeze(struct ata_port *ap)
{
	__ata_port_freeze(ap);
	return 0;
}


unsigned ata_exec_internal( struct ata_device   *dev,
			                struct ata_taskfile *tf,
                            const u8            *cdb,
			                int                 dma_dir,
                            unsigned int        buflen,
			                unsigned long       timeout,
                            int                 curr_dev )
{
	struct ata_link         *link = dev->link;
	struct ata_port         *ap   = &ata_ports[curr_dev];
	struct ata_queued_cmd   *qc;
	unsigned int            tag, preempted_tag;
	u32                     preempted_sactive, preempted_qc_active;
	int                     preempted_nr_active_links;
	unsigned int            err_mask;
	int                     rc = 0;
	u8                      status;


    struct hsata_device  *hsdev;

    hsdev = HSDEV_FROM_AP(ap);


    debug(" %s:%d \n", __FUNCTION__, __LINE__ );

	status = ata_busy_wait(ap, ATA_BUSY, 300000);
	if (status & ATA_BUSY) {
		printf("BSY = 0 check. timeout. (%d)\n", __LINE__);
		rc = false;
		return rc;
	}

	if (ap->pflags & ATA_PFLAG_FROZEN)
		return AC_ERR_SYSTEM;

	tag = ATA_TAG_INTERNAL;

    if (test_and_set_bit(tag, &ap->qc_allocated)) {
		printf("    error at here\n");
        rc = false;
		return rc;
	}

    qc = __ata_qc_from_tag(ap, tag);
	qc->tag = tag;
	qc->ap = ap;
	qc->dev = dev;

    ata_qc_reinit(qc);

	preempted_tag = link->active_tag;
	preempted_sactive = link->sactive;
	preempted_qc_active = ap->qc_active;
	preempted_nr_active_links = ap->nr_active_links;
	link->active_tag = ATA_TAG_POISON;

	link->sactive = 0;
	ap->qc_active = 0;
	ap->nr_active_links = 0;
	qc->tf = *tf;

	if (cdb)
		memcpy(qc->cdb, cdb, ATAPI_CDB_LEN);

	qc->flags |= ATA_QCFLAG_RESULT_TF;
	qc->dma_dir = dma_dir;
	qc->private_data = 0;

    ata_qc_issue(qc);

	if (!timeout)
		timeout = ata_probe_timeout * 1000 / HZ;

    //status = ata_busy_wait(ap, ATA_BUSY, 30000);
    status = ata_busy_wait(ap, ATA_BUSY, 50000);

	if (status & ATA_BUSY) {
		printf("BSY = 0 check. timeout. (%d)\n", __LINE__);
		printf("altstatus = 0x%x.\n", status);
		qc->err_mask |= AC_ERR_OTHER;
		return qc->err_mask;
	}

    // waiting for DRQ (data request) bit
    // - DRQ is asserted when data is reday to transfer
    if (waiting_for_reg_state(ap->ioaddr.altstatus_addr, 1000, 0x8)) {
		u8 status = 0;
		u8 errorStatus = 0;

        status = readb(ap->ioaddr.altstatus_addr);
		if ((status & 0x01) != 0) {
			errorStatus = readb(ap->ioaddr.feature_addr);
			if (errorStatus == 0x04 &&
				qc->tf.command == ATA_CMD_PIO_READ_EXT){
				printf("Hard Disk doesn't support LBA48\n");

                //dev_state = SATA_ERROR;
                hsdev->dev_state = SATA_ERROR;

                qc->err_mask |= AC_ERR_OTHER;
			    debug("    err_mask(1): %d\n", qc->err_mask );
                return qc->err_mask;
			}
		}
		qc->err_mask |= AC_ERR_OTHER;
        debug("    err_mask(2): %d\n", qc->err_mask );
		return qc->err_mask;
	}

	status = ata_busy_wait(ap, ATA_BUSY, 10);
	if (status & ATA_BUSY) {
		printf("BSY = 0 check. timeout. (%d)\n", __LINE__);
		qc->err_mask |= AC_ERR_OTHER;
        debug("    err_mask(3): %d\n", qc->err_mask );
		return qc->err_mask;
	}

	ata_pio_task(ap);

	if (!rc) {
		if (qc->flags & ATA_QCFLAG_ACTIVE) {
			qc->err_mask |= AC_ERR_TIMEOUT;
			ata_port_freeze(ap);
		}
	}

	if (qc->flags & ATA_QCFLAG_FAILED) {
		if (qc->result_tf.command & (ATA_ERR | ATA_DF))
			qc->err_mask |= AC_ERR_DEV;

		if (!qc->err_mask)
			qc->err_mask |= AC_ERR_OTHER;

		if (qc->err_mask & ~AC_ERR_OTHER)
			qc->err_mask &= ~AC_ERR_OTHER;
	}

	*tf = qc->result_tf;
	err_mask = qc->err_mask;
	ata_qc_free(qc);
	link->active_tag = preempted_tag;
	link->sactive = preempted_sactive;
	ap->qc_active = preempted_qc_active;
	ap->nr_active_links = preempted_nr_active_links;

	if (ap->flags & ATA_FLAG_DISABLED) {
		err_mask |= AC_ERR_SYSTEM;
		ap->flags &= ~ATA_FLAG_DISABLED;
	}
    debug( "    err_mask: %d\n", err_mask );
	return err_mask;
}

static void ata_qc_issue(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	struct ata_link *link = qc->dev->link;
	u8 prot = qc->tf.protocol;

	if (ata_is_ncq(prot)) {
		if (!link->sactive)
			ap->nr_active_links++;
		link->sactive |= 1 << qc->tag;
	} else {
		ap->nr_active_links++;
		link->active_tag = qc->tag;
	}

	qc->flags |= ATA_QCFLAG_ACTIVE;
	ap->qc_active |= 1 << qc->tag;

	if (qc->dev->flags & ATA_DFLAG_SLEEPING) {
		mdelay(1);
		return;
	}

	qc->err_mask |= ata_qc_issue_prot(qc);
	if (qc->err_mask)
		goto err;

	return;
err:
	ata_qc_complete(qc);
}

static unsigned int ata_qc_issue_prot(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;

	if (ap->flags & ATA_FLAG_PIO_POLLING) {
		switch (qc->tf.protocol) {
		case ATA_PROT_PIO:
		case ATA_PROT_NODATA:
		case ATAPI_PROT_PIO:
		case ATAPI_PROT_NODATA:
			qc->tf.flags |= ATA_TFLAG_POLLING;
			break;
		default:
			break;
		}
	}

	ata_dev_select(ap, qc->dev->devno, 1, 0);

	switch (qc->tf.protocol) {
	case ATA_PROT_PIO:
		if (qc->tf.flags & ATA_TFLAG_POLLING)
			qc->tf.ctl |= ATA_NIEN;

		ata_tf_to_host(ap, &qc->tf);

		ap->hsm_task_state = HSM_ST;

		if (qc->tf.flags & ATA_TFLAG_POLLING)
			ata_pio_queue_task(ap, qc, 0);

		break;

	default:
		return AC_ERR_SYSTEM;
	}

	return 0;
}

static void ata_tf_to_host(struct ata_port *ap,
			const struct ata_taskfile *tf)
{
	ata_tf_load(ap, tf);
	ata_exec_command(ap, tf);
}

static void ata_tf_load(struct ata_port *ap,
			const struct ata_taskfile *tf)
{
	struct ata_ioports *ioaddr = &ap->ioaddr;
	unsigned int is_addr = tf->flags & ATA_TFLAG_ISADDR;

	if (tf->ctl != ap->last_ctl) {
		if (ioaddr->ctl_addr)
			writeb(tf->ctl, ioaddr->ctl_addr);
		ap->last_ctl = tf->ctl;
		ata_wait_idle(ap);
	}

	if (is_addr && (tf->flags & ATA_TFLAG_LBA48)) {
		writeb(tf->hob_feature, ioaddr->feature_addr);
		writeb(tf->hob_nsect, ioaddr->nsect_addr);
		writeb(tf->hob_lbal, ioaddr->lbal_addr);
		writeb(tf->hob_lbam, ioaddr->lbam_addr);
		writeb(tf->hob_lbah, ioaddr->lbah_addr);
	}

	if (is_addr) {
		writeb(tf->feature, ioaddr->feature_addr);
		writeb(tf->nsect, ioaddr->nsect_addr);
		writeb(tf->lbal, ioaddr->lbal_addr);
		writeb(tf->lbam, ioaddr->lbam_addr);
		writeb(tf->lbah, ioaddr->lbah_addr);
	}

	if (tf->flags & ATA_TFLAG_DEVICE)
		writeb(tf->device, ioaddr->device_addr);

	ata_wait_idle(ap);
}

static void ata_exec_command(struct ata_port *ap,
			const struct ata_taskfile *tf)
{

    writeb(tf->command, ap->ioaddr.command_addr);

	readb(ap->ioaddr.altstatus_addr);

	udelay(1);
}

static void ata_pio_queue_task(struct ata_port *ap,
			void *data,unsigned long delay)
{
	ap->port_task_data = data;
}

static unsigned int ata_tag_internal(unsigned int tag)
{
	return tag == ATA_MAX_QUEUE - 1;
}

static void fill_result_tf(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;

	qc->result_tf.flags = qc->tf.flags;
	ata_tf_read(ap, &qc->result_tf);
}

static void ata_tf_read(struct ata_port *ap, struct ata_taskfile *tf)
{
	struct ata_ioports *ioaddr = &ap->ioaddr;

	tf->command = ata_check_status(ap);
	tf->feature = readb(ioaddr->error_addr);
	tf->nsect = readb(ioaddr->nsect_addr);
	tf->lbal = readb(ioaddr->lbal_addr);
	tf->lbam = readb(ioaddr->lbam_addr);
	tf->lbah = readb(ioaddr->lbah_addr);
	tf->device = readb(ioaddr->device_addr);

	if (tf->flags & ATA_TFLAG_LBA48) {
		if (ioaddr->ctl_addr) {
			writeb(tf->ctl | ATA_HOB, ioaddr->ctl_addr);

			tf->hob_feature = readb(ioaddr->error_addr);
			tf->hob_nsect = readb(ioaddr->nsect_addr);
			tf->hob_lbal = readb(ioaddr->lbal_addr);
			tf->hob_lbam = readb(ioaddr->lbam_addr);
			tf->hob_lbah = readb(ioaddr->lbah_addr);

			writeb(tf->ctl, ioaddr->ctl_addr);
			ap->last_ctl = tf->ctl;
		} else {
			printf("sata_dwc warnning register read.\n");
		}
	}
}

static void __ata_qc_complete(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	struct ata_link *link = qc->dev->link;

	link->active_tag = ATA_TAG_POISON;
	ap->nr_active_links--;

	if (qc->flags & ATA_QCFLAG_CLEAR_EXCL && ap->excl_link == link)
		ap->excl_link = NULL;

	qc->flags &= ~ATA_QCFLAG_ACTIVE;
	ap->qc_active &= ~(1 << qc->tag);
}

static void ata_qc_free(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	unsigned int tag;
	qc->flags = 0;
	tag = qc->tag;
	if (tag < ATA_MAX_QUEUE) {
		qc->tag = ATA_TAG_POISON;
#if defined (CONFIG_MIPS)
		clear_bit(tag, &ap->qc_allocated);
#elif defined (CONFIG_ARM)
		__clear_bit(tag, &ap->qc_allocated);
#endif
	}
}

static void ata_qc_complete(struct ata_queued_cmd *qc)
{
	struct ata_device *dev = qc->dev;
	if (qc->err_mask)
		qc->flags |= ATA_QCFLAG_FAILED;

	if (qc->flags & ATA_QCFLAG_FAILED) {
		if (!ata_tag_internal(qc->tag)) {
			fill_result_tf(qc);
			return;
		}
	}
	if (qc->flags & ATA_QCFLAG_RESULT_TF)
		fill_result_tf(qc);

	/* Some commands need post-processing after successful
	 * completion.
	 */
	switch (qc->tf.command) {
	case ATA_CMD_SET_FEATURES:
		if (qc->tf.feature != SETFEATURES_WC_ON &&
				qc->tf.feature != SETFEATURES_WC_OFF)
			break;
	case ATA_CMD_INIT_DEV_PARAMS:
	case ATA_CMD_SET_MULTI:
		break;

	case ATA_CMD_SLEEP:
		dev->flags |= ATA_DFLAG_SLEEPING;
		break;
	}

	__ata_qc_complete(qc);
}

//#if defined(CONFIG_SATA_DWC) && !defined(CONFIG_LBA48)
#if !defined(CONFIG_LBA48)
#define SATA_MAX_READ_BLK 0xFF
#else
#define SATA_MAX_READ_BLK 0xFFFF
#endif

ulong sata_read(int device, ulong blknr, lbaint_t blkcnt, void *buffer)
{
	ulong start,blks, buf_addr;
	unsigned short smallblks;
	unsigned long datalen;
	unsigned char *pdata;
	device &= 0xff;

	u32 block = 0;
	u32 n_block = 0;

    debug(" %s:%d \n", __FUNCTION__,  __LINE__ );

    if( hsata_dev[device].dev_state != SATA_READY ) {
        printf("device is not ready!");
        return 0;
    }

	buf_addr = (unsigned long)buffer;

	start = blknr;
	blks = blkcnt;
	do {
		pdata = (unsigned char *)buf_addr;

		if (blks > SATA_MAX_READ_BLK) {
			datalen = sata_dev_desc[device].blksz * SATA_MAX_READ_BLK;
			smallblks = SATA_MAX_READ_BLK;

			block = (u32)start;
			n_block = (u32)smallblks;

			start += SATA_MAX_READ_BLK;
			blks -= SATA_MAX_READ_BLK;
		} else {
			datalen = sata_dev_desc[device].blksz * SATA_MAX_READ_BLK;
			datalen = sata_dev_desc[device].blksz * blks;
			smallblks = (unsigned short)blks;

			block = (u32)start;
			n_block = (u32)smallblks;

			start += blks;
			blks = 0;
		}

		if (ata_dev_read_sectors(pdata, datalen, block, n_block, device) != true) {
			printf("sata_dwc : Hard disk read error.\n");
			blkcnt -= blks;
			break;
		}
		buf_addr += datalen;
	} while (blks != 0);

	return (blkcnt);
}

static int ata_dev_read_sectors( unsigned char *pdata,
                                 unsigned long datalen,
						         u32           block,
                                 u32           n_block,
                                 u32           curr_dev )
{
	//struct ata_port     *ap = pap;
    struct ata_port     *ap  = &ata_ports[curr_dev];
	struct ata_device   *dev = &ata_devices[curr_dev];
	struct ata_taskfile tf;
	unsigned int        class = ATA_DEV_ATA;
	unsigned int        err_mask = 0;
	const char          *reason;
	int                 may_fallback = 1;

    struct hsata_device *hsdev;

    hsdev = HSDEV_FROM_AP(ap);

    if ( hsdev->dev_state == SATA_ERROR ) {
        printf("device error state\n");
        return false;
    }

	ata_dev_select(ap, dev->devno, 1, 1);

retry:
	memset(&tf, 0, sizeof(tf));
	tf.ctl = ap->ctl;
	ap->print_id = 1;
	ap->flags &= ~ATA_FLAG_DISABLED;
	ap->pdata = pdata;

	tf.device = ATA_DEVICE_OBS;

	temp_n_block = n_block;

#ifdef CONFIG_LBA48
	tf.command = ATA_CMD_PIO_READ_EXT;
	tf.flags |= ATA_TFLAG_LBA | ATA_TFLAG_LBA48;

	tf.hob_feature = 31;
	tf.feature = 31;
	tf.hob_nsect = (n_block >> 8) & 0xff;
	tf.nsect = n_block & 0xff;

	tf.hob_lbah = 0x0;
	tf.hob_lbam = 0x0;
	tf.hob_lbal = (block >> 24) & 0xff;
	tf.lbah = (block >> 16) & 0xff;
	tf.lbam = (block >> 8) & 0xff;
	tf.lbal = block & 0xff;

	tf.device = 1 << 6;
	if (tf.flags & ATA_TFLAG_FUA)
		tf.device |= 1 << 7;
#else
	tf.command = ATA_CMD_PIO_READ;
	tf.flags |= ATA_TFLAG_LBA ;

	tf.feature = 31;
	tf.nsect = n_block & 0xff;

	tf.lbah = (block >> 16) & 0xff;
	tf.lbam = (block >> 8) & 0xff;
	tf.lbal = block & 0xff;

	tf.device = (block >> 24) & 0xf;

	tf.device |= 1 << 6;
	if (tf.flags & ATA_TFLAG_FUA)
		tf.device |= 1 << 7;

#endif

	tf.protocol = ATA_PROT_PIO;

	/* Some devices choke if TF registers contain garbage.  Make
	 * sure those are properly initialized.
	 */
	tf.flags |= ATA_TFLAG_ISADDR | ATA_TFLAG_DEVICE;
	tf.flags |= ATA_TFLAG_POLLING;
#if 1
	err_mask = ata_exec_internal(dev, &tf, NULL, DMA_FROM_DEVICE, 0, 0, curr_dev);
#else
	err_mask = ata_exec_internal( hsdev, &tf, NULL, DMA_FROM_DEVICE, 0, 0 );
#endif

	if (err_mask) {

        debug("    err_mask: 0x%x\n", err_mask );

		if (err_mask & AC_ERR_NODEV_HINT) {
			printf("READ_SECTORS NODEV after polling detection\n");
			return -ENOENT;
		}

		if ((err_mask == AC_ERR_DEV) && (tf.feature & ATA_ABORTED)) {
			/* Device or controller might have reported
			 * the wrong device class.  Give a shot at the
			 * other IDENTIFY if the current one is
			 * aborted by the device.
			 */
			if (may_fallback) {
				may_fallback = 0;

				if (class == ATA_DEV_ATA) {
					class = ATA_DEV_ATAPI;
				} else {
					class = ATA_DEV_ATA;
				}
				goto retry;
			}
			/* Control reaches here iff the device aborted
			 * both flavors of IDENTIFYs which happens
			 * sometimes with phantom devices.
			 */
			printf("both IDENTIFYs aborted, assuming NODEV\n");
			return -ENOENT;
		}

		reason = "I/O error";
		goto err_out;
	}

	return true;

err_out:
	printf("failed to READ SECTORS (%s, err_mask=0x%x)\n", reason, err_mask);
	return false;
}

//#if defined(CONFIG_SATA_DWC) && !defined(CONFIG_LBA48)
#if !defined(CONFIG_LBA48)
#define SATA_MAX_WRITE_BLK 0xFF
#else
#define SATA_MAX_WRITE_BLK 0xFFFF
#endif

ulong sata_write(int device, ulong blknr, lbaint_t blkcnt, void *buffer)
{
	ulong start,blks, buf_addr;
	unsigned short smallblks;
	unsigned long datalen;
	unsigned char *pdata;
	device &= 0xff;


	u32 block = 0;
	u32 n_block = 0;

    if ( hsata_dev[device].dev_state != SATA_READY ) {
        printf( "device is not ready\n");
        return 0;
    }

	buf_addr = (unsigned long)buffer;
	start = blknr;
	blks = blkcnt;
	do {
		pdata = (unsigned char *)buf_addr;
		if (blks > SATA_MAX_WRITE_BLK) {
			datalen = sata_dev_desc[device].blksz * SATA_MAX_WRITE_BLK;
			smallblks = SATA_MAX_WRITE_BLK;

			block = (u32)start;
			n_block = (u32)smallblks;

			start += SATA_MAX_WRITE_BLK;
			blks -= SATA_MAX_WRITE_BLK;
		} else {
			datalen = sata_dev_desc[device].blksz * blks;
			smallblks = (unsigned short)blks;

			block = (u32)start;
			n_block = (u32)smallblks;

			start += blks;
			blks = 0;
		}

		if (ata_dev_write_sectors(pdata, datalen, block, n_block, device) != true) {
			printf("sata_dwc : Hard disk write error.\n");
			blkcnt -= blks;
			break;
		}
		buf_addr += datalen;
	} while (blks != 0);

	return (blkcnt);
}

static int ata_dev_write_sectors( unsigned char *pdata,
                                  unsigned long datalen,
						          u32           block,
                                  u32           n_block,
                                  u32           curr_dev )
{
	struct ata_port     *ap  = &ata_ports[curr_dev];
	struct ata_device   *dev = &ata_devices[curr_dev];
	struct ata_taskfile tf;
	unsigned int        class = ATA_DEV_ATA;
	unsigned int        err_mask = 0;
	const char          *reason;
	int                 may_fallback = 1;

    struct hsata_device *hsdev;

    hsdev = HSDEV_FROM_AP(ap);

    if ( hsdev->dev_state == SATA_ERROR ) {
        printf("device error state!\n");
        return false;
    }

	ata_dev_select(ap, dev->devno, 1, 1);

retry:
	memset(&tf, 0, sizeof(tf));
	tf.ctl = ap->ctl;
	ap->print_id = 1;
	ap->flags &= ~ATA_FLAG_DISABLED;

	ap->pdata = pdata;

	tf.device = ATA_DEVICE_OBS;

	temp_n_block = n_block;


#ifdef CONFIG_LBA48
	tf.command = ATA_CMD_PIO_WRITE_EXT;
	tf.flags |= ATA_TFLAG_LBA | ATA_TFLAG_LBA48 | ATA_TFLAG_WRITE;

	tf.hob_feature = 31;
	tf.feature = 31;
	tf.hob_nsect = (n_block >> 8) & 0xff;
	tf.nsect = n_block & 0xff;

	tf.hob_lbah = 0x0;
	tf.hob_lbam = 0x0;
	tf.hob_lbal = (block >> 24) & 0xff;
	tf.lbah = (block >> 16) & 0xff;
	tf.lbam = (block >> 8) & 0xff;
	tf.lbal = block & 0xff;

	tf.device = 1 << 6;
	if (tf.flags & ATA_TFLAG_FUA)
		tf.device |= 1 << 7;
#else
	tf.command = ATA_CMD_PIO_WRITE;
	tf.flags |= ATA_TFLAG_LBA | ATA_TFLAG_WRITE;

	tf.feature = 31;
	tf.nsect = n_block & 0xff;

	tf.lbah = (block >> 16) & 0xff;
	tf.lbam = (block >> 8) & 0xff;
	tf.lbal = block & 0xff;

	tf.device = (block >> 24) & 0xf;

	tf.device |= 1 << 6;
	if (tf.flags & ATA_TFLAG_FUA)
		tf.device |= 1 << 7;

#endif

	tf.protocol = ATA_PROT_PIO;

	/* Some devices choke if TF registers contain garbage.  Make
	 * sure those are properly initialized.
	 */
	tf.flags |= ATA_TFLAG_ISADDR | ATA_TFLAG_DEVICE;
	tf.flags |= ATA_TFLAG_POLLING;
#if 1
	err_mask = ata_exec_internal(dev, &tf, NULL, DMA_FROM_DEVICE, 0, 0, curr_dev);
#else
	err_mask = ata_exec_internal(hsdev, &tf, NULL, DMA_FROM_DEVICE, 0, 0 );
#endif

	if (err_mask) {
		if (err_mask & AC_ERR_NODEV_HINT) {
			printf("READ_SECTORS NODEV after polling detection\n");
			return -ENOENT;
		}

		if ((err_mask == AC_ERR_DEV) && (tf.feature & ATA_ABORTED)) {
			/* Device or controller might have reported
			 * the wrong device class.  Give a shot at the
			 * other IDENTIFY if the current one is
			 * aborted by the device.
			 */
			if (may_fallback) {
				may_fallback = 0;

				if (class == ATA_DEV_ATA) {
					class = ATA_DEV_ATAPI;
				} else {
					class = ATA_DEV_ATA;
				}
				goto retry;
			}
			/* Control reaches here iff the device aborted
			 * both flavors of IDENTIFYs which happens
			 * sometimes with phantom devices.
			 */
			printf("both IDENTIFYs aborted, assuming NODEV\n");
			return -ENOENT;
		}

		reason = "I/O error";
		goto err_out;
	}

	return true;

err_out:
	printf("failed to WRITE SECTORS (%s, err_mask=0x%x)\n", reason, err_mask);
	return false;
}
