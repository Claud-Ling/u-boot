/*
 * drivers/mtd/nand/monza_nand.c
 */
#include <common.h>
#include <nand.h>
#include <errno.h>
#include <malloc.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <asm/arch/reg_io.h>
#include <asm/arch/setup.h>

#include "trix_nand.h"
#include "trix_ecc.h"

#define FLASH_RNDOUT_READ

#ifndef CONFIG_SYS_NAND_BASE_LIST
#define CONFIG_SYS_NAND_BASE_LIST { CONFIG_SYS_NAND_BASE }
#endif


static char ecc_tmp_read[250]  __attribute__ ((aligned (4)));
static char ecc_tmp_write[250] __attribute__ ((aligned (4)));

extern nand_info_t nand_info[CONFIG_SYS_MAX_NAND_DEVICE];

//static struct nand_chip nand_chip[CONFIG_SYS_MAX_NAND_DEVICE];
static ulong base_address[CONFIG_SYS_MAX_NAND_DEVICE] = CONFIG_SYS_NAND_BASE_LIST;

#define BADBLOCK_MARKER_LENGTH  3
static struct nand_ecclayout monza_nand_oobinfo;
/*
 * Following table shows the standard main+spare area scheme
 * page main          spare area size
 * ==============     ==============
 *      2048-bytes          64-bytes
 *      4096-bytes         224-bytes
 *      8192-bytes         448-bytes
 */
static void monza_config_ecc_layout(struct mtd_info *mtd)
{
        struct nand_chip *chip = mtd->priv;
	int i, oobsize, ecc_total;

	struct nand_ecclayout *layout = &monza_nand_oobinfo;

	switch (mtd->writesize)
        {
                case 2048:
			oobsize = 64;
			break;
                case 4096:
			oobsize = 224;
			break;
                case 8192:
			oobsize = 448;
			break;
                default:
                        printk("ERROR: unsupport NAND device.\n");
			BUG();
	}

	chip->ecc.steps = mtd->writesize / chip->ecc.size;
	/*
 	 * chip->ecc.bytes:	ECC bytes per step
 	 * chip->ecc.steps:	number of ECC steps per page
 	 * ecc_total:		total number of ECC bytes per page
 	 * +++++++++++++++++++++++++++++++++++++++++++++
 	 * page size | ecc.steps | ecc.bytes | ecc_total
 	 * +++++++++++++++++++++++++++++++++++++++++++++
 	 *    2048   |    4      x  14    =     56
 	 *    4096   |    4      x  42    =     168
 	 *    8192   |    8      x  42    =     336
 	 */
	ecc_total = chip->ecc.bytes * chip->ecc.steps;

	layout->eccbytes = ecc_total;
	for (i = 0; i < ecc_total; i++)
		layout->eccpos[i] = oobsize - ecc_total + i;

	layout->oobfree[0].offset = BADBLOCK_MARKER_LENGTH;
	layout->oobfree[0].length =
		oobsize - ecc_total - layout->oobfree[0].offset;

	chip->ecc.layout = layout;
}

static void enable_nce(int enabled)
{
        unsigned int val;

        val = readl( NAND_BOOTROM_R_TIMG );

        if(!enabled){
                writew( val | 0x80, NAND_BOOTROM_R_TIMG );
        }
        else{
                writew( val & ~0x80, NAND_BOOTROM_R_TIMG );
        }
}

static void monza_nand_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
        struct nand_chip *nand_chip = (struct nand_chip *) (mtd->priv);
        static unsigned long IO_ADDR_W = 0;

        switch (ctrl) {
        case NAND_CTRL_CHANGE:
                enable_nce(0);
                break;

        default:
                enable_nce(1);
                if(ctrl & NAND_CTRL_CHANGE)
                {
                        if(ctrl == NAND_NCE)
                                IO_ADDR_W = ((unsigned long)nand_chip->IO_ADDR_W);
                        else if(ctrl & NAND_ALE)
                                IO_ADDR_W = ((unsigned long)nand_chip->IO_ADDR_W) + (1 << 14);
                        else if(ctrl & NAND_CLE)
                                IO_ADDR_W = ((unsigned long)nand_chip->IO_ADDR_W) + (1 << 13);
                        else
                                IO_ADDR_W = ((unsigned long)nand_chip->IO_ADDR_W);
                }

                break;
        }

        if (cmd != NAND_CMD_NONE){
        writeb( cmd, (void *)IO_ADDR_W );
                //udelay(500);
        }

}

static int monza_nand_ready(struct mtd_info *mtd)
{
    return ( readl(NAND_BOOTROM_R_TIMG) & 0x40 ) !=0;
}

/*
 * set nand controller work mode
 *      mode: NAND_HW_MODE / NAND_SW_MODE
 */
static void set_nand_mode(struct monza_nand_info *info,int mode)
{
	if( likely(MONZA_NAND_NEW == info->nand_ctrler) )
		MWriteRegWord(NAND_CCR, mode<<24, 0xFF000000);
	else if( MONZA_NAND_OLD == info->nand_ctrler)
		MWriteRegWord(NAND_DCR, mode<<24, 0xFF000000);
}

/* Allocate the DMA buffers */
static int monza_nand_dma_init(struct monza_nand_info *info)
{
	return 0;
}

static void hidtv_nand_inithw(struct monza_nand_info *info)
{
        unsigned char val8;
        unsigned int val32;

        /* switch the nand source clock from 100Hz to 200Hz for this version of NAND mode setting
        * if the souce clock is not match the NAND mode, will cause the DMA read time out error */

        val8 = ReadRegByte((unsigned int)0xf500e849);
        val8 &= 0xcf;
        val8 |= 0x20;
        WriteRegByte((unsigned int)0xf500e849,val8);

        /* switch the nand source clock 200Hz clock for write,
        if the clock is not match the NAND init setting such like 1b002004 = 0x21, will cause WP*/
        writel(0x00000067 ,NAND_FCR);

        val32 = readl(NAND_MLC_ECC_CTRL);
        val32 |= 0x100;
        writel(val32 ,NAND_MLC_ECC_CTRL);

        val32 = readl(NAND_SPI_DLY_CTRL);
        val32 &= 0xffff3f3f;
        writel(val32 ,NAND_SPI_DLY_CTRL);

        writel(0x40193333 ,NAND_TIMG);

        /* ECC control reset */
        writel((readl(NAND_ECC_CTRL)&(~0x4)) , NAND_ECC_CTRL );
        writel(0x1000 ,NAND_ECC_CTRL);
        while((readl(NAND_ECC_CTRL)&0x1000)!=0);
        writel(0x02 , NAND_ECC_CTRL);

	writel( 0x00000002, NAND_DCR);
	writel( 0x00000420, NAND_CTRL);
	set_nand_mode(info, NAND_SW_MODE);
        return;
}

/*PIO Mode*/
static void monza_pio_read_buf32(struct mtd_info *mtd, uint8_t *buf, int len)
{
        int i;
        struct nand_chip *chip = mtd->priv;
        u32 *p = (u32 *) buf;

        for (i = 0; i < (len >> 2); i++)
                p[i] = ReadRegWord((unsigned int)chip->IO_ADDR_R);

}

static void monza_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
        struct nand_chip *chip = mtd->priv;
        register volatile void* hwaddr = chip->IO_ADDR_W;
        register u32 *p = (u32 *) buf;

        len >>= 2;
        if (len == 0)
                return;
        do {
                WriteRegWord((unsigned int)hwaddr,*p++);
        } while(--len);

}

/*
 * @param: is_read - 0 DMA read;1 DMA write
 * @return: 0 success;others fail.
 *
 * NOTE:dma write not support so far,design for future.
 */
static int monza_nand_dma_op(struct mtd_info *mtd, void *buf, int len,
                               int is_read)
{
	struct monza_nand_info *info = mtd_to_monza(mtd); 
        struct nand_chip *chip       = mtd->priv;

        int err = 0;
        dma_addr_t dma_src_addr, dma_dst_addr;
        unsigned short val16;
        unsigned int prev,cur;

	unsigned long start;
        unsigned long deadline =  MONZA_DMA_TRANS_TIMEOUT;

        if (is_read) {
                dma_src_addr = (dma_addr_t)chip->IO_ADDR_R;
                dma_dst_addr = info->data_buff_phys;
        } else {
                dma_src_addr = info->data_buff_phys;
                dma_dst_addr = (dma_addr_t)chip->IO_ADDR_W;
        }
        /*set up dma*/
        writel(dma_src_addr             , NAND_FDMA_SRC_ADDR );
        writel(dma_dst_addr             , NAND_FDMA_DEST_ADDR);
        writel(info->dma_int_descr_phys , NAND_FDMA_INT_ADDR );
        writel((len - 1) & 0xFFFF       , NAND_FDMA_LEN_ADDR );

        prev = readl(info->dma_int_descr_virt);

        /* enable flash DMA read access */
        val16 = ReadRegHWord((unsigned int)0xfb000006);
        WriteRegHWord((unsigned int)0xfb000006, val16 |0x1);

	start = get_timer(0);
	do{
                /*
                 * readl_relaxed here is a must!
                 * it guarantee that memory in ddr is strongly order accessed.
                 */
                cur = readl(info->dma_int_descr_virt);
                if(cur > prev ){
                        /*dma complete*/
                        break;
                }
        }while (get_timer(start) < deadline);

        if (get_timer(start) > deadline)
        {
                printf("warning:%s, DMA not ready.prev:%08x. current: %08x, xfer_type force to PIO mode\n",__func__,prev,cur);
                /*
                 * DMA fatal,force to PIO mode.
                 */
                err = -ETIMEDOUT;
                info->xfer_type = MONZA_NAND_PIO;
        }

        /* disable flash DMA read access */
        WriteRegHWord((unsigned int)0xfb000006, val16 & 0xfffe);
        memcpy(buf, info->data_buff_virt, len);
        return err;
}

static void monza_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	unsigned long nand_reg = 0x0;
	struct monza_nand_info *info = mtd_to_monza(mtd); 
	
	if(likely(MONZA_NAND_NEW == info->nand_ctrler))
		nand_reg = NAND_CCR;
	else if( MONZA_NAND_OLD == info->nand_ctrler)
		nand_reg = NAND_CTRL;
	set_nand_mode(info, NAND_HW_MODE);      /*set HC as hw mode to fast read*/
	MWriteRegWord(NAND_CTRL, len<<16, 0xFFFF0000);
	MWriteRegWord(nand_reg, 1, 0x1);

        /* only use DMA for bigger than oob size: better performances */
        if( (MONZA_NAND_DMA == info->xfer_type) && (len > mtd->oobsize) )
        {
                if(monza_nand_dma_op(mtd, buf, len, 1) == 0)
                        goto xfer_complete;
                else
                        goto xfer_err;
        }
        monza_pio_read_buf32(mtd, buf ,len);

xfer_complete:
        set_nand_mode(info, NAND_SW_MODE);      /*must exit hw mode*/
	MWriteRegWord(NAND_CTRL, 0<<16, 0xFFFF0000);
	MWriteRegWord(nand_reg, 0, 0x1);
        return;

xfer_err:
        /*
         * fatal during dma transfer
         * xfer_type will be force to MONZA_NAND_PIO from now.
         */
        set_nand_mode(info, NAND_SW_MODE);
        writel(0x00000420 ,NAND_CTRL);
        monza_pio_read_buf32(mtd, buf ,len);
        return;
}

static int monza_verify_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
        int i;
        struct nand_chip *chip = mtd->priv;
        u32 *p = (u32 *) buf;

        len >>= 2;
        for (i = 0; i < len; i++)
                if (p[i] != ReadRegWord((unsigned int)chip->IO_ADDR_R))
                        return -EFAULT;

        return 0;
}

static int monza_read_page_bch(struct mtd_info *mtd, struct nand_chip *chip,
                                uint8_t *buf,int oob_required, int page_addr)
{
        int i,  eccsize = chip->ecc.size;
        int eccbytes = chip->ecc.bytes;
        int eccsteps = chip->ecc.steps;
        u_char *p = buf;
        u_char *ecc_code = chip->buffers->ecccode;
        uint32_t *eccpos = chip->ecc.layout->eccpos;
        int stat = 0;

        /* Read the OOB area first */
        chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page_addr);//read oob first
        chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);

        for (i = 0; i < chip->ecc.total; i++)
                ecc_code[i] = chip->oob_poi[eccpos[i]];
#ifdef FLASH_SUPPORT_RNDOUT_READ
        chip->cmdfunc(mtd, NAND_CMD_RNDOUT, 0x00, -1);
#else
        chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page_addr);
#endif
	
	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize)
	{
		chip->ecc.hwctl(mtd, NAND_ECC_READ|(i<<8));
		chip->read_buf(mtd, p, eccsize);
		stat = chip->ecc.correct(mtd, p, NULL, NULL);
		if(stat)
			break;
        }

        return stat;
}

static int monza_write_page_bch(struct mtd_info *mtd, struct nand_chip *chip,
                                const uint8_t *buf,int oob_required)
{
        int i, eccsize = chip->ecc.size;
        int eccbytes = chip->ecc.bytes;
        int eccsteps = chip->ecc.steps;
        u_char *ecc_calc = chip->buffers->ecccalc;
        uint32_t *eccpos = chip->ecc.layout->eccpos;
        const uint8_t *p = buf;
        int stat = 0;
        
	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
                chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
                chip->write_buf(mtd, p, eccsize);
                stat = chip->ecc.calculate(mtd, p,(uint8_t *)ecc_tmp_write);
                if(stat)
                {
                        printf("BCH: write page error %d\n",stat);
                        return -EIO;
                }
		memcpy(&ecc_calc[i] ,ecc_tmp_write,chip->ecc.bytes);
        }

	for (i = 0; i < chip->ecc.total; i++)
		chip->oob_poi[eccpos[i]] = ecc_calc[i];

	chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);

	return 0;
}

/* stubs for ECC functions not used by the NAND core */
static int monza_ecc_calculate(struct mtd_info *mtd, const uint8_t *data,
                                uint8_t *ecc_code)
{
        int stat;
        struct nand_chip *chip = mtd->priv;

        stat = bch_encode_end((unsigned int*)ecc_code,chip);

        return stat;
}

static int monza_ecc_correct(struct mtd_info *mtd, uint8_t *data,
                                uint8_t *read_ecc, uint8_t *calc_ecc)
{
        int stat = 0;
        struct nand_chip *chip = mtd->priv;
	
	stat = bch_decode_end_correct(mtd,data,chip);

        return stat;
}

static void monza_ecc_hwctl(struct mtd_info *mtd, int param)
{
        struct nand_chip *chip = mtd->priv;
        uint8_t *ecc_code = chip->buffers->ecccode;

        int mode = param & 0xFF;
        int offset = (param >> 8) & 0xFFFF;

        switch(mode)
        {
        case NAND_ECC_READ:
		memcpy(ecc_tmp_read,&ecc_code[offset],chip->ecc.bytes);
                bch_decode_start((unsigned int*)ecc_tmp_read,chip);
                break;
        case NAND_ECC_WRITE:
                bch_encode_start(chip);
                break;
        }

        return;
}
static void monza_set_nand_ctrler(struct monza_nand_info *info)
{
	bool is_new_controller = otp_get_new_nand_sel_state();

	info->nand_ctrler = (is_new_controller == true)?MONZA_NAND_NEW:MONZA_NAND_OLD;
	printf("Nand ctrler type: %d\n",info->nand_ctrler);

	return;
}

static void nand_init_chip(int devnum)
{
	int maxchips = CONFIG_SYS_NAND_MAX_CHIPS;
	struct mtd_info *mtd   = &nand_info[devnum];
	struct nand_chip *nand ;
	struct monza_nand_info *info;
	info = malloc(sizeof(struct monza_nand_info));
	if (!info) {
		printf("Error: Failed to allocate memory for NAND%d\n", devnum);
		return;
	}
	memset(info, 0, sizeof(struct monza_nand_info));

	nand= &(info->nand);
	mtd->priv = nand;
	nand->priv = info;

	monza_set_nand_ctrler(info);
	nand->IO_ADDR_R = (void __iomem*)base_address[devnum];
	nand->IO_ADDR_W = (void __iomem*)base_address[devnum];
#ifdef CONFIG_MTD_NAND_DMA
	info->xfer_type  = MONZA_NAND_DMA;
#else
	info->xfer_type  = MONZA_NAND_PIO;
#endif

	/*basic hardware initialisation*/
	hidtv_nand_inithw(info);
	monza_nand_dma_init(info);

	//info->nand.options   = NAND_NO_AUTOINCR;    /*|NAND_USE_FLASH_BBT|NAND_USE_FLASH_BBT_NO_OOB*/

	nand->chip_delay  = 1000; //fix me
	nand->cmd_ctrl    = monza_nand_hwcontrol;
	nand->dev_ready   = monza_nand_ready;

        nand->write_buf   = monza_write_buf;
        nand->read_buf    = monza_read_buf;
       // nand->verify_buf  = monza_verify_buf;
        nand->ecc.mode     = NAND_ECC_HW;

	if (nand_scan_ident(mtd, maxchips, NULL)) {
		return;
	}

	nand->options |= NAND_NO_SUBPAGE_WRITE;    /*we not support subpage in UBI*/
	mtd->subpage_sft = 0;

	switch (mtd->writesize)
        {
                case 2048:
                        info->ecc_strength	= 8;
                        nand->ecc.size		= 512;
                        break;
                case 4096:
                        info->ecc_strength	= 24;
                        nand->ecc.size		= 1024;
			/*
			* TC58NVG2S0HBAI4 has a wrong description in the ONFI param
			* it reports 128-bytes spare size,but 256-bytes actually.
			*/
			mtd->oobsize		= 224;
                        break;
                case 8192:
                        info->ecc_strength	= 24;
                        nand->ecc.size		= 1024;
			/* we assume oob size is 448 on 8k page NAND device */
			mtd->oobsize		= 448;
                        break;
                default:
                        printk("ERROR: unsupport NAND device.\n");
                        return;
        }

        /*ECC bytes per step*/
	if( likely(MONZA_NAND_NEW == info->nand_ctrler) )
		/*BCH GF 15*/
        	nand->ecc.bytes = (15*info->ecc_strength + 7)>>3;
	else if( MONZA_NAND_OLD == info->nand_ctrler)
		/*BCH BF 14*/
        	nand->ecc.bytes = (14*info->ecc_strength + 7)>>3;
	
	monza_config_ecc_layout(mtd);
        /*
         * sanity checks
         * NOTE:it's important!
         */
        BUG_ON( (nand->ecc.size != 512) && (nand->ecc.size != 1024) );
        BUG_ON( (info->ecc_strength < 4) || (info->ecc_strength > 60)||(info->ecc_strength % 2 ));

	nand->ecc.strength = info->ecc_strength;

        if (!nand->ecc.read_page)
                nand->ecc.read_page = monza_read_page_bch;
        if (!nand->ecc.write_page)
                nand->ecc.write_page = monza_write_page_bch;

        nand->ecc.calculate = monza_ecc_calculate;
        nand->ecc.hwctl     = monza_ecc_hwctl;
        nand->ecc.correct   = monza_ecc_correct;

        /* second phase scan */
        if (nand_scan_tail(mtd)) {
                return;
        }

        printk("Detected NAND, %lldMiB, erasesize %dKiB, pagesize %dB,oobsize %dB,oobavail %dB\n",
        mtd->size >> 20,
        mtd->erasesize >> 10,
        mtd->writesize,
        mtd->oobsize,
        mtd->oobavail);

        printk("Nand ECC info:Strength=%d,ecc size=%d ecc_bytes=%d\n",
        info->ecc_strength,
        nand->ecc.size,
        nand->ecc.bytes);

	nand_register(devnum);
}


void board_nand_init(void)
{
	int i;

	debug("%s \n",__func__);
	for (i = 0; i < CONFIG_SYS_MAX_NAND_DEVICE; i++)
		nand_init_chip(i);
}
