//#define DEBUG

#include <common.h>
#include <malloc.h>

#include <asm/io.h>
#include <asm/errno.h>

#include <nand.h>
#include <linux/mtd/nand.h>

#include <asm/arch/setup.h>
#include <asm/arch/xenvkeys.h>

/**
    NAND access register
*/
#define SMP8XXX_REG_CMD             0
#define SMP8XXX_REG_ADDR            4
#define SMP8XXX_REG_DATA            8


#define PB_IORDY                    0x80000000
#define PB_padmode_pbusb            0x08f0
#define tReset                      5 //us


/* Enabling AUTOPIO operations */
#define USE_AUTOPIO
/* Enabling CTRLER specific IRQ */
//#define USE_CTRLER_IRQ

/* Definitions used by new controller */
#define MAXPAGES 16
/**
    New NAND interface 
 */
#define MLC_CHA_REG                 0x4400
#define MLC_CHB_REG                 0x4440
#define MLC_CHA_MEM                 0x4800
#define MLC_CHB_MEM                 0x4a00
#define MLC_BADBLOCK_OFFSET         0x100
#define MLC_ECCREPORT_OFFSET        0x1c0

#define MLC2_CHA_REG                 0xC000
#define MLC2_CHB_REG                 0xC040
#define MLC2_CHA_MEM                 0xD000
#define MLC2_CHB_MEM                 0xD800
#define MLC2_BADBLOCK_OFFSET         0x100
#define MLC2_ECCREPORT_OFFSET        0x1c0



/* channnel A/B are tied to CS0/1 */

static unsigned int chx_reg[2];  //= { MLC_CHA_REG, MLC_CHB_REG };
static unsigned int chx_mem[2];  //= { MLC_CHA_MEM, MLC_CHB_MEM };
static const unsigned int sbox_tgt[2] = { SBOX_PCIMASTER, SBOX_PCISLAVE };

#define STATUS_REG(b)       ((b) + 0x0)
#define FLASH_CMD(b)        ((b) + 0x4)
#define DEVICE_CFG(b)       ((b) + 0x8)
#define TIMING1(b)          ((b) + 0xc)
#define TIMING2(b)          ((b) + 0x10)
#define XFER_CFG(b)         ((b) + 0x14)
#define PACKET_0_CFG(b)     ((b) + 0x18)
#define PACKET_N_CFG(b)     ((b) + 0x1c)
#define BAD_BLOCK_CFG(b)    ((b) + 0x20)
#define ADD_PAGE(b)         ((b) + 0x24)
#define ADD_OFFSET(b)       ((b) + 0x28)

#define MAX_CS          CONFIG_SYS_MAX_NAND_DEVICE  /* Maximum number of CS */
#define MAX_NAND_DEVS   CONFIG_SYS_MAX_NAND_DEVICE
#define MAX_PARTITIONS  16

/* XENV keys to be used */
#define CS_RESERVED	    "a.cs%d_rsvd_pblk"
#define CS_PARTS        "a.cs%d_pblk_parts"
#define CS_PART_SIZE    "a.cs%d_pblk_part%d_size"
#define CS_PART_OFFSET  "a.cs%d_pblk_part%d_offset" 
#define CS_PART_NAME    "a.cs%d_pblk_part%d_name" 
#define CS_TIMING1      "a.cs%d_nand_timing1"
#define CS_TIMING2      "a.cs%d_nand_timing2"
#define CS_DEVCFG       "a.cs%d_nand_devcfg"
#define NAND_PARAM      "a.nandpart%d_params"
#define HIGH_32         "_hi"

/* u-boot mtd env */
#define ENV_MTD_IDS     "mtdids"   
#define ENV_MTD_PARTS   "mtdparts" 
#define ENV_NAND_NAME   "sigm-nand%d"

#define BUFSIZE		    256

#if defined (CONFIG_MIPS)
#define NON_CACHED(a)   KSEG1ADDR(a)
#define DMA_ADDR(a)     tangox_phys_addr(a)
#elif defined (CONFIG_ARM)
#define NON_CACHED(a)   a
#define DMA_ADDR(a)     a
#endif

enum nand_controller
{
    PB_NAND_CTRLER = 0,
    MLC_NAND_CTRLER,
    MLC2_NAND_CTRLER
};

struct chip_private
{
    unsigned int cs;    /* chip select */
    uint8_t *bbuf;      /* bounce buffer */
};


enum PAD_MODES 
{
    PAD_MODE_PB,
    PAD_MODE_MLC
};

static struct nand_chip nand_chip[CONFIG_SYS_MAX_NAND_DEVICE];
static struct chip_private chip_privs[MAX_CS];

/* Internal data structure */
static char env_mtdids[128];
static char env_mtdparts[256];

//static struct mtd_info tangox_mtds[MAX_CS];
///static struct nand_chip tangox_chips[MAX_CS];
///static struct mtd_partition *tangox_partitions[MAX_CS];
static struct nand_hw_control tangox_hw_control;
//static int cs_avail[MAX_CS], cs_parts[MAX_CS];
static int cs_avail[MAX_CS];
static int cs_offset;

static int nand_ctrler = MLC_NAND_CTRLER;      /* use new controller as default */
static int max_page_shift = 13; /* up to 8KB page */


static RMuint8 local_pb_cs_ctrl;
static RMuint32 local_pb_cs_config, local_pb_cs_config1;
static int chip_cnt = 0;

extern unsigned long xenv_addr;

extern int zxenv_get(char *recordname, void *dst, u32 *datasize);
extern int xenv_get(u32 *base, u32 size, char *recordname, void *dst, u32 *datasize);
extern unsigned long tangox_phys_addr(volatile void* virtual_addr);
extern unsigned long tangox_dma_address(unsigned long physaddr);
extern int tangox_nand_scan_ident(struct mtd_info *mtd, int maxchips, 
                                  const struct nand_flash_dev *table);

extern unsigned long tangox_chip_id(void);

/*
 *  PB Nand Controller ECC Layout
 *
 */
// for 512B page, typically with 16B OOB
static struct nand_ecclayout tangox_nand_pb_ecclayout512_16 = {
    .eccbytes = 3,
    .eccpos = {10, 11, 12},
    .oobfree = {
        {.offset = 6, .length = 4},
    },
};

/* for 2KB page, typically with 64B OOB */
static struct nand_ecclayout tangox_nand_pb_ecclayout2048_64 = {
    .eccbytes = 12,
    .eccpos = {10, 11, 12, 13, 14, 15, 16, 17,
               18, 19, 20, 21},
    .oobfree = {
        {.offset = 6, .length = 4},
        {.offset = 22, .length = 38},
    },
};


/*
 *	MLC1 Nand Controller ECC Layout
 *
 */
/* for 512B page, typically with 16B OOB, and we use 4bit ECC */
static struct nand_ecclayout tangox_nand_mlc1_ecclayout512_16_4 = { // may not be supported
	.eccbytes = 7,
	.eccpos = {6, 7, 8, 9, 10, 11, 12},
	.oobfree = {
		{.offset = 13, .length = 3}
	},
};

/* for 2KB page, typically with 64B OOB, and we use 8bit ECC */
static struct nand_ecclayout tangox_nand_mlc1_ecclayout2048_64_8 = {
	.eccbytes = 13,
	.eccpos = {49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61},
	.oobfree = {
		{.offset = 62, .length = 2}
	},
};

// for 4KB page, typically with 128B OOB, and we use 9bit ECC
static struct nand_ecclayout tangox_nand_mlc1_ecclayout4096_128_9 = {
	.eccbytes = 15,
	.eccpos = {110, 111, 112, 113, 114, 115, 116, 117,
			   118, 119, 120, 121, 122, 123, 124},
	.oobfree = {
		{.offset = 125, .length = 3}
	},
};

// for 4KB page, typically with 218B OOB or above, and we use 16bit ECC
static struct nand_ecclayout tangox_nand_mlc1_ecclayout4096_218_16 = {
	.eccbytes = 26,
	.eccpos = {187, 188, 189, 190, 191, 192, 193, 194,
			   195, 196, 197, 198, 199, 200, 201, 202,
			   203, 204, 205, 206, 207, 208, 209, 210,
			   211, 212},
	.oobfree = {
		{.offset = 213, .length = 5}
	},
};

// for 8KB page, typically with 448 OOB, and we use 16bit ECC
static struct nand_ecclayout tangox_nand_mlc1_ecclayout8192_448_16 = {
	.eccbytes = 26,
	.eccpos = {395, 396, 397, 398, 399, 400, 401, 402,
			   403, 404, 405 ,406, 407, 408, 409, 410,
			   411, 412, 413, 414, 415, 416, 417, 418,
			   419, 420},
	.oobfree = {
		{.offset = 421, .length = 27}
	}
};


/*
 *	MLC2 Nand Controller ECC Layout
 *
 */

/* for 2KB page, typically with 64B OOB, and we use 14bit ECC */
static struct nand_ecclayout tangox_nand_mlc2_ecclayout2048_64_14 = {
	.eccbytes = 27,
	.eccpos = { 37, 38, 39, 40, 41, 42, 43, 44, 
                45, 46, 47, 48, 49, 50, 51, 52, 
                53, 54, 55, 56, 57, 58, 59, 60, 
                61, 62, 63
               },
	.oobfree = {
		{.offset = 0, .length = 0}
	},
};

// for 4KB page, typically with 218B OOB or above, and we use 27bit ECC
static struct nand_ecclayout tangox_nand_mlc2_ecclayout4096_218_27 = {
	.eccbytes = 51,
	.eccpos = { 163, 164, 165, 166, 167, 168, 169, 170, 171, 172,
             173, 174, 175, 176, 177, 178, 179, 180, 181, 182,
             183, 184, 185, 186, 187, 188, 189, 190, 191, 192,
             193, 194, 195, 196, 197, 198, 199, 200, 201, 202,
             203, 204, 205, 206, 207, 209, 210, 211, 212, 213,
             214
            },
	.oobfree = {
		{.offset = 214, .length = 4}
	},
};

// for 4KB page, typically with 224B OOB or above, and we use 27bit ECC
static struct nand_ecclayout tangox_nand_mlc2_ecclayout4096_224_27 = {
	.eccbytes = 51,
	.eccpos = { 163, 164, 165, 166, 167, 168, 169, 170, 171, 172,
             173, 174, 175, 176, 177, 178, 179, 180, 181, 182,
             183, 184, 185, 186, 187, 188, 189, 190, 191, 192,
             193, 194, 195, 196, 197, 198, 199, 200, 201, 202,
             203, 204, 205, 206, 207, 209, 210, 211, 212, 213,
             214
            },
	.oobfree = {
		{.offset = 214, .length = 10}
	},
};

// for 4KB page, typically with 232B OOB or above, and we use 27bit ECC
static struct nand_ecclayout tangox_nand_mlc2_ecclayout4096_232_27 = {
	.eccbytes = 51,
	.eccpos = { 163, 164, 165, 166, 167, 168, 169, 170, 171, 172,
                173, 174, 175, 176, 177, 178, 179, 180, 181, 182,
                183, 184, 185, 186, 187, 188, 189, 190, 191, 192,
                193, 194, 195, 196, 197, 198, 199, 200, 201, 202,
                203, 204, 205, 206, 207, 209, 210, 211, 212, 213,
                214
               },
	.oobfree = {
		{.offset = 214, .length = 18}
	},
};

/* for 8KB page, typically with 448 OOB, and we use 28bit ECC */
static struct nand_ecclayout tangox_nand_mlc2_ecclayout8192_448_28 = {
    .eccbytes = 53,
	.eccpos = { 381, 382, 383, 384, 385, 386, 387, 388, 389, 390,
                391, 392, 393, 394, 395, 396, 397, 398, 399, 400,
                401, 402, 403, 404, 405, 406, 407, 408, 409, 410,
                411, 412, 413, 414, 415, 416, 417, 418, 419, 420,
                421, 422, 423, 424, 425, 426, 427, 428, 429, 430,
                431, 432, 433
            },
	.oobfree = {
		{.offset = 434, .length = 14}
	}
};

/* sets pad mode */
inline static void tangox_set_padmode( int pad_mode )
{
    unsigned long chip_id = (tangox_chip_id() >> 16) & 0xff00;
    
    /* filter out unnecessary setup */
    if ( ((chip_id & 0x0000ff00) != 0x8700) && ( (chip_id & 0x0000ff00) != 0x2400) )
        return;
    
    switch( pad_mode ) {
        case PAD_MODE_PB:
            gbus_write_reg32(REG_BASE_host_interface+PB_padmode_pbusb,0x00000000);
            break;
        case PAD_MODE_MLC:
            gbus_write_reg32(REG_BASE_host_interface+PB_padmode_pbusb,0x80000000);
            break;
    }
}

/* this function will check validity of the address range 
   if it is valid it will return 0. otherwise -1
*/
static int check_addr_range( void* addr )
{
#if defined (CONFIG_MIPS)
    if ((((u32)addr) < KSEG0) || (((u32)addr) >= KSEG2))
        return -1;
#elif defined (CONFIG_ARM)
    /*TODO: need to implement for ARM later */
#else
#error NAND driver: Unsupported architecture!
#endif

    return 0;
}

/**
 * tangox_read_byte -  read one byte from the chip
 * @mtd:    MTD device structure
 *
 *  read function for 8bit buswidth
 */
static u_char tangox_read_byte(struct mtd_info *mtd)
{
    struct nand_chip *this = mtd->priv;
    
    tangox_set_padmode( PAD_MODE_PB );
    
    return RD_HOST_REG8((RMuint32)this->IO_ADDR_R + SMP8XXX_REG_DATA);
}


/**
 * tangox_write_buf -  write buffer to chip
 * @mtd:    MTD device structure
 * @buf:    data buffer
 * @len:    number of bytes to write
 *
 *  write function for 8bit buswidth
 */
static void tangox_write_buf(struct mtd_info *mtd, const u_char *buf, int len)
{

    int i;
    struct nand_chip *this = (struct nand_chip *)mtd->priv;

#ifdef USE_AUTOPIO
    unsigned int cs = ((struct chip_private *)this->priv)->cs;
    unsigned long g_mbus_reg = 0;
    dma_addr_t dma_addr;

    //if ((in_atomic()) || (len <= mtd->oobsize))
    if ( len <= mtd->oobsize )
        goto pio;
    else if ( check_addr_range( (void*)buf) )    
        goto pio;
    else if (em86xx_mbus_alloc_dma(SBOX_IDEFLASH, 0, &g_mbus_reg, NULL, 0))
        goto pio;

    /* check and set pad mode */
    tangox_set_padmode( PAD_MODE_MLC );
    WR_HOST_REG32( chx_mem[cs] + MLC2_BADBLOCK_OFFSET,   0xffffffff );
    WR_HOST_REG32( chx_mem[cs] + MLC2_BADBLOCK_OFFSET+4, 0xffffffff );
    WR_HOST_REG32( chx_mem[cs] + MLC2_BADBLOCK_OFFSET+8, 0xffffffff );

    // alignment
    {
        ulong aligned_buf;
        ulong aligned_end;
        
        aligned_buf  = (ulong)buf & ~0x1f;
        aligned_end  = ((ulong)buf + len + 31) & ~0x1f;
        
        //printf( "+buf: 0x%x, len: 0x%x, al_buf: 0x%x, al_end: 0x%x\n", 
        //    (unsigned int)buf, len, (unsigned int)aligned_buf, aligned_end );
        
        flush_dcache_range( (ulong)aligned_buf, aligned_end ); 
    }


    //dma_addr = dma_map_single(NULL, (void *)buf, len, DMA_TO_DEVICE);
    dma_addr = (dma_addr_t)DMA_ADDR( (void*)buf );

    gbus_write_reg32(REG_BASE_host_interface + PB_automode_control + 4, 0);
    gbus_write_reg32(REG_BASE_host_interface + PB_automode_start_address, SMP8XXX_REG_DATA);
    /* 22:nand 17:8bit width 16:DRAM to PB len:number of PB accesses */
    gbus_write_reg32(REG_BASE_host_interface + PB_automode_control, (cs << 24) | (2 << 22) | (1 << 17) | (0 << 16) | len);
    
    //em86xx_mbus_setup_dma(g_mbus_reg, dma_addr, len, pbi_mbus_intr, NULL, 1);
    em86xx_mbus_setup_dma(g_mbus_reg, dma_addr, len, NULL, NULL, 1);

    ///wait_event_interruptible(mbus_wq, mbus_done != 0);
    while (gbus_read_reg32(REG_BASE_host_interface + PB_automode_control) & 0xffff)
        ; /* wait for AUTOPIO completion */
    ///mbus_done = 0;
    
    em86xx_mbus_free_dma(g_mbus_reg, SBOX_IDEFLASH);
    ///dma_unmap_single(NULL, dma_addr, len, DMA_TO_DEVICE);

    goto done;

pio:
#endif
    /* set pad mode to pb */
    tangox_set_padmode( PAD_MODE_PB );
    
    for (i = 0; i < len; i++) 
        WR_HOST_REG8((RMuint32)this->IO_ADDR_W + SMP8XXX_REG_DATA, buf[i]);

#ifdef USE_AUTOPIO
done:
#endif
    return;
}


/**
 * tangox_read_buf -  read chip data into buffer
 * @mtd:    MTD device structure
 * @buf:    buffer to store data
 * @len:    number of bytes to read
 *
 *  read function for 8bit buswith
 */
static void tangox_read_buf(struct mtd_info *mtd, u_char *buf, int len)
{
    int i;
    struct nand_chip *this = mtd->priv;


#ifdef USE_AUTOPIO
    unsigned int cs = ((struct chip_private *)this->priv)->cs;
    unsigned long g_mbus_reg = 0;
    dma_addr_t dma_addr;

    debug("len: %d, mtd->oobsize:%d\n", len, mtd->oobsize );

    //if ((in_atomic()) || (len <= mtd->oobsize))
    if ( len <= mtd->oobsize )
        goto pio;
    else if ( check_addr_range( (void*)buf) )
        goto pio;
    else if (em86xx_mbus_alloc_dma(SBOX_IDEFLASH, 1, &g_mbus_reg, NULL, 0))
        goto pio;

    /* check and set pad mode */
    tangox_set_padmode( PAD_MODE_MLC );

    // TODO: need to convert the input address to DMAable address
    ///dma_addr = dma_map_single(NULL, (void *)buf, len, DMA_FROM_DEVICE);
    dma_addr = (dma_addr_t)DMA_ADDR( (void*)buf );
    
    debug("RX DMA buff: 0x%x\n", dma_addr);

    gbus_write_reg32(REG_BASE_host_interface + PB_automode_control + 4, 0);
    gbus_write_reg32(REG_BASE_host_interface + PB_automode_start_address, SMP8XXX_REG_DATA);
    /* 22:nand 17:8bit width 16:DRAM to PB len:number of PB accesses */
    gbus_write_reg32(REG_BASE_host_interface + PB_automode_control, (cs << 24) | (2 << 22) | (1 << 17) | (1 << 16) | len);

    //em86xx_mbus_setup_dma(g_mbus_reg, dma_addr, len, pbi_mbus_intr, NULL, 1);
    em86xx_mbus_setup_dma(g_mbus_reg, dma_addr, len, NULL, NULL, 1);

    //wait_event_interruptible(mbus_wq, mbus_done != 0);
    while (gbus_read_reg32(REG_BASE_host_interface + PB_automode_control) & 0xffff)
        ; /* wait for AUTOPIO completion */
    ///mbus_done = 0;

    em86xx_mbus_free_dma(g_mbus_reg, SBOX_IDEFLASH);
    ///dma_unmap_single(NULL, dma_addr, len, DMA_FROM_DEVICE);

    /* when loading address is KSEG1 and MTD internal buffer is used, 
       part of data is not visible after DMA transfer. 
       so let's invalidate the d-cache */
    {
        ulong aligned_buf;
        ulong aligned_end;
        
        aligned_buf  = (ulong)buf & ~0x1f;
        aligned_end  = ((ulong)buf + len + 31) & ~0x1f;
        
        //printf( "-buf: 0x%x, len: 0x%x, al_buf: 0x%x, al_end: 0x%x\n", 
        //    (unsigned int)buf, len, (unsigned int)aligned_buf, aligned_end );
        
        invalidate_dcache_range( (ulong)aligned_buf, aligned_end ); 
    }
    goto done;
pio:
#endif
    
    /* set pad mode to pb */
    tangox_set_padmode( PAD_MODE_PB );
    
    for (i = 0; i < len; i++) 
        buf[i] = RD_HOST_REG8((RMuint32)this->IO_ADDR_R + SMP8XXX_REG_DATA);

#ifdef USE_AUTOPIO
done:
#endif
    return;

}

static void tangox_nand_bug(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct mtd_ecc_stats ecc_stats = mtd->ecc_stats;
	unsigned int cs = ((struct chip_private *)chip->priv)->cs;
	printf("cs = %d, options = 0x%x\n", cs, chip->options);
	printf("ecc stats: corrected = %d, failed = %d, badblocks = %d, bbtblocks = %d\n",
		ecc_stats.corrected, ecc_stats.failed, ecc_stats.badblocks, ecc_stats.bbtblocks);
	//printf("to be resolved, here's the call-stack: \n");
	//dump_stack();
}

static int tangox_nand_bug_calculate(struct mtd_info *mtd, const uint8_t *dat, uint8_t *ecc_code)
{
	tangox_nand_bug(mtd);
	return 0;	/* should have no need to calculate */
}

static int tangox_nand_bug_correct(struct mtd_info *mtd, uint8_t *dat, uint8_t *read_ecc, uint8_t *calc_ecc)
{
	tangox_nand_bug(mtd);
	return 0;	/* should have no need to correct */
}


/*
 * Function for register configurations
 */
static int tangox_packet_config(struct mtd_info *mtd, struct nand_chip *chip, int len)
{
	unsigned int cs = ((struct chip_private *)chip->priv)->cs;
	unsigned int csel = cs << 24;
	int ret = -1;

    debug("tangox_packet_config - len: %d\n", len);

    if ( nand_ctrler == MLC2_NAND_CTRLER ) {
        
        /* 2K packet */
        switch(len)
        {
            case 512:
                ret = 0;
                break;
            case 2048:
                WR_HOST_REG32(XFER_CFG(chx_reg[cs]), 0x00010204 | csel);//xfer
                WR_HOST_REG32(PACKET_0_CFG(chx_reg[cs]), 0x0404000E);	//packet_0
                WR_HOST_REG32(PACKET_N_CFG(chx_reg[cs]), 0x0400000E);	//packet_N
                WR_HOST_REG32(BAD_BLOCK_CFG(chx_reg[cs]), 0x08000006);	//bad_block
                ret = 0;
                break;
            case 4096:
                WR_HOST_REG32(XFER_CFG(chx_reg[cs]), 0x00010404 | csel);//xfer
                WR_HOST_REG32(PACKET_0_CFG(chx_reg[cs]), 0x0404001b);	//packet_0
                WR_HOST_REG32(PACKET_N_CFG(chx_reg[cs]), 0x0400001b);	//packet_N
                WR_HOST_REG32(BAD_BLOCK_CFG(chx_reg[cs]), 0x10000006);	//bad_block
                ret = 0;
                break;
            case 8192:
                WR_HOST_REG32(XFER_CFG(chx_reg[cs]), 0x00010804 | csel);//xfer
                WR_HOST_REG32(PACKET_0_CFG(chx_reg[cs]), 0x0404001c);	//packet_0
                WR_HOST_REG32(PACKET_N_CFG(chx_reg[cs]), 0x0400001c);	//packet_N
                WR_HOST_REG32(BAD_BLOCK_CFG(chx_reg[cs]), 0x10000006);	//bad_block
                ret = 0;
                break;
        }
    }
    else {

        switch (len) 
        {
            case 512:	/* 4 bit ECC per 512B packet */
                // May not be supported by current nand filesystems 
                WR_HOST_REG32(XFER_CFG(chx_reg[cs]), 0x10010104 | csel);//xfer
                WR_HOST_REG32(PACKET_0_CFG(chx_reg[cs]), 0x02040004);	//packet_0
                WR_HOST_REG32(BAD_BLOCK_CFG(chx_reg[cs]), 0x02000002);	//bad_block
                ret = 0;
                break;

            case 2048:	/* 8 bit ECC per 512 packet */
                WR_HOST_REG32(XFER_CFG(chx_reg[cs]), 0x00010404 | csel);//xfer
                WR_HOST_REG32(PACKET_0_CFG(chx_reg[cs]), 0x02040008);	//packet_0
                WR_HOST_REG32(PACKET_N_CFG(chx_reg[cs]), 0x02000008);	//packet_N
                WR_HOST_REG32(BAD_BLOCK_CFG(chx_reg[cs]), 0x08000006);	//bad_block
                ret = 0;
                break;

            case 4096:	/* 9 or 16 bit ECC per 512 packet depends on OOB size */
                if ((mtd->oobsize >= 128) && (mtd->oobsize < 218)) {
                    WR_HOST_REG32(XFER_CFG(chx_reg[cs]), 0x00010804 | csel);//xfer
                    WR_HOST_REG32(PACKET_0_CFG(chx_reg[cs]), 0x02040009);	//packet_0
                    WR_HOST_REG32(PACKET_N_CFG(chx_reg[cs]), 0x02000009);	//packet_N
                    WR_HOST_REG32(BAD_BLOCK_CFG(chx_reg[cs]), 0x10000001);	//bad_block
                    ret = 0;
                } else if (mtd->oobsize >= 218) {
                    WR_HOST_REG32(XFER_CFG(chx_reg[cs]), 0x00010804 | csel);//xfer
                    WR_HOST_REG32(PACKET_0_CFG(chx_reg[cs]), 0x02040010);	//packet_0
                    WR_HOST_REG32(PACKET_N_CFG(chx_reg[cs]), 0x02000010);	//packet_N
                    WR_HOST_REG32(BAD_BLOCK_CFG(chx_reg[cs]), 0x10000001);	//bad_block
                    ret = 0;
                }
                break;

            case 8192:	/* 16 bit ECC per 512 packet */
                //packet number 16 exceeds amount of reserved bits -
                //spec is wrong and is being changed ...
                WR_HOST_REG32(XFER_CFG(chx_reg[cs]), 0x00011004 | csel);//xfer
                WR_HOST_REG32(PACKET_0_CFG(chx_reg[cs]), 0x02040010);	//packet_0
                WR_HOST_REG32(PACKET_N_CFG(chx_reg[cs]), 0x02000010);	//packet_N
                WR_HOST_REG32(BAD_BLOCK_CFG(chx_reg[cs]), 0x20000001);	//bad_block
                ret = 0;
                break;
        }
    } // end of PB and MLC nand controller
    
	if (ret != 0)
		printf("unsupported Packet Config on CS%d.\n", cs);

	return ret;
}

/* Check ECC correction validity
 * Return 0 if not valid */
static int tangox_validecc(struct mtd_info *mtd)
{
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	unsigned int cs = ((struct chip_private *)chip->priv)->cs;
	unsigned int code = RD_HOST_REG32(chx_mem[cs] + MLC_ECCREPORT_OFFSET) & 0xffff;

	if (((code & 0x8080) != 0x8080) && (code != 0)) { /* (code == 0) is most likey blank page */
		//if (printk_ratelimit())
		//	printk(KERN_WARNING "%s: ecc error detected (code=0x%x)\n", smp_nand_devname, code);
        printf( "ecc error detected (code=0x%x)\n", code);
		return 0;
	} 
	return 1;
}

/**
 * tangox_read_page_hwecc - hardware ecc based page write function
 * @mtd:	MTD device info structure
 * @chip:	nand chip info structure
 * @buf:	buffer to store read data
 * @page:	page number
 */
static int tangox_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buffer, int oob_required, int page)
{
	unsigned int cs = ((struct chip_private *)chip->priv)->cs;
	int len = mtd->writesize;
	unsigned long g_mbus_reg = 0;
	uint8_t *buf = buffer;
	uint8_t *bbuf = ((struct chip_private *)chip->priv)->bbuf;
	dma_addr_t dma_addr;

    debug("tangox_read_page_hwecc\n");

    /* set pad muxing */
    tangox_set_padmode( PAD_MODE_MLC );

	//if ((in_atomic()) || (len <= mtd->oobsize))
    if ( len <= mtd->oobsize )
		return -EIO;
	else if (tangox_packet_config(mtd, chip, len) != 0) 
		return -EIO;

	if ( check_addr_range((void*)buf) )	
        buf = bbuf;	/* use bounce buffer */
    
	// Channel A used in soft for CS0 channel B used in soft for CS1
	if (em86xx_mbus_alloc_dma(sbox_tgt[cs], 1, &g_mbus_reg, NULL, 1) < 0)
		return -EIO;

	//dma_addr = dma_map_single(NULL, (void *)buf, len, DMA_FROM_DEVICE);
    dma_addr = (dma_addr_t)DMA_ADDR( (void*)buf );
    debug("dma_addr: 0x%08x\n", dma_addr );

	// poll ready status
	while ((RD_HOST_REG32(STATUS_REG(chx_reg[cs])) & 0x80000000) == 0)
		; /* unlikely it's not ready */

	// launch New Controller read command
	WR_HOST_REG32(FLASH_CMD(chx_reg[cs]), 0x1);

#ifdef USE_CTRLER_IRQ
	em86xx_mbus_setup_dma(g_mbus_reg, dma_addr, len, callbacks[cs], NULL, 1);
	wait_event_interruptible(*wqueues[cs], chx_mbus_done[cs] != 0);
#else
	em86xx_mbus_setup_dma(g_mbus_reg, dma_addr, len, NULL, NULL, 1);
#endif

	// poll ready status
	while ((RD_HOST_REG32(STATUS_REG(chx_reg[cs])) & 0x80000000) == 0)
		; /* wait for completion */
#ifdef USE_CTRLER_IRQ
	chx_mbus_done[cs] = 0;
#endif

	em86xx_mbus_free_dma(g_mbus_reg, sbox_tgt[cs]);
	//dma_unmap_single(NULL, dma_addr, len, DMA_FROM_DEVICE);

	if (buf == bbuf) {
		/* copy back */
		memcpy(buffer, buf, len);
	}

#if defined (CONFIG_MIPS)
    /* target address is in KSEG0 */
    if( check_addr_cacheable((void*)buf) ) {
       invalidate_dcache_range((ulong)buf, (ulong)(buf+len)); 
    }
#else
    {
        ulong aligned_buf;
        ulong aligned_end;
        
        aligned_buf  = (ulong)buf & ~0x1f;
        aligned_end  = ((ulong)buf + len + 31) & ~0x1f;
        
        //printf( "-buf: 0x%x, len: 0x%x, al_buf: 0x%x, al_end: 0x%x\n", 
        //    (unsigned int)buf, len, (unsigned int)aligned_buf, aligned_end );
        
        invalidate_dcache_range( (ulong)aligned_buf, aligned_end ); 
    }
#endif    
   

	if (!tangox_validecc(mtd)) {
		mtd->ecc_stats.failed++;
		return 1;
	}
	return 0;
}

/**
 * tangox_write_page_hwecc - hardware ecc based page write function
 * @mtd:	MTD device info structure
 * @chip:	nand chip info structure
 * @buf:	data buffer
 */
static int tangox_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buffer, int oob_required )
{
    
	unsigned int cs = ((struct chip_private *)chip->priv)->cs;
	int len = mtd->writesize;
	unsigned long g_mbus_reg = 0;
	uint8_t *buf = (uint8_t *)buffer;
	uint8_t *bbuf = ((struct chip_private *)chip->priv)->bbuf;
	dma_addr_t dma_addr;
    
    /* set pad muxing */
    tangox_set_padmode( PAD_MODE_MLC );
    WR_HOST_REG32( chx_mem[cs] + MLC2_BADBLOCK_OFFSET,   0xffffffff );
    WR_HOST_REG32( chx_mem[cs] + MLC2_BADBLOCK_OFFSET+4, 0xffffffff );
    WR_HOST_REG32( chx_mem[cs] + MLC2_BADBLOCK_OFFSET+8, 0xffffffff );

	//if ((in_atomic()) || (len <= mtd->oobsize)) {
    if ( len <= mtd->oobsize ) {
		tangox_nand_bug(mtd);
		return -1;/* TODO: -EIO? */;
	} else if (tangox_packet_config(mtd, chip, len) != 0) {
		tangox_nand_bug(mtd);
		return -1;/* TODO: -EIO? */;
	}

	if ( check_addr_range((void*)buf) ) {
		buf = bbuf;	/* use bounce buffer */
		memcpy(buf, buffer, len);
	}

	// Channel A used in soft for CS0 channel B used in soft for CS1
	if (em86xx_mbus_alloc_dma(sbox_tgt[cs], 0, &g_mbus_reg, NULL, 1) < 0) {
		tangox_nand_bug(mtd);
		return -1;/* TODO: -EIO? */;
	}

#if defined(CONFIG_MIPS)    
    /* write back before copy from sdram */
    if( check_addr_cacheable((void*)buf) ) {
       flush_dcache_range( (ulong)buf, (ulong)(buf+len) ); 
    }
#else
    {
        ulong aligned_buf;
        ulong aligned_end;
        
        aligned_buf  = (ulong)buf & ~0x1f;
        aligned_end  = ((ulong)buf + len + 31) & ~0x1f;

        //printf( "-buf: 0x%x, len: 0x%x, al_buf: 0x%x, al_end: 0x%x\n", 
        //    (unsigned int)buf, len, (unsigned int)aligned_buf, aligned_end );
        
        flush_dcache_range( (ulong)aligned_buf, aligned_end ); 
    }
#endif    

	//dma_addr = dma_map_single(NULL, (void *)buf, len, DMA_TO_DEVICE);
    dma_addr = (dma_addr_t)DMA_ADDR( (void*)buf );

	// poll ready status
	while ((RD_HOST_REG32(STATUS_REG(chx_reg[cs])) & 0x80000000) == 0)
		; /* unlikely it's not ready */

	// launch New Controller write command
	WR_HOST_REG32(FLASH_CMD(chx_reg[cs]), 0x2);

#ifdef USE_CTRLER_IRQ
	em86xx_mbus_setup_dma(g_mbus_reg, dma_addr, len, callbacks[cs], NULL, 1);
	wait_event_interruptible(*wqueues[cs], chx_mbus_done[cs] != 0);
#else
	em86xx_mbus_setup_dma(g_mbus_reg, dma_addr, len, NULL, NULL, 1);
#endif
	// poll ready status
	while ((RD_HOST_REG32(STATUS_REG(chx_reg[cs])) & 0x80000000) == 0)
		; /* wait for completion */
#ifdef USE_CTRLER_IRQ
	chx_mbus_done[cs] = 0;
#endif

	em86xx_mbus_free_dma(g_mbus_reg, sbox_tgt[cs]);
	//dma_unmap_single(NULL, dma_addr, len, DMA_TO_DEVICE);

	return 0;
}
/*
     Scheme 6
     size:  4       1024      27        993      6   31    27
          +----+------------+-----+------------+---+----+-----+
          |meta|   data0    | ECC |   dataN    |BB | .. | ECC |
          +----+------------+-----+------------+---+----+-----+
     pos: 0    4          1028  1055        2048  2054  2085  2112
          \__________ ___________/ \_____________ ___________/
                     V                           V
                  packet0                     packet1

     Scheme 10
     size:  4       1024      51       1024      51       1024      51      867        6    157   51      4
          +----+------------+-----+------------+-----+------------+-----+------------+----+-----+-----+--------+
          |meta|   data0    | ECC |   dataN    | ECC |   dataN    | ECC |   dataN    | BB | ..  | ECC |(Unused)|
          +----+------------+-----+------------+-----+------------+-----+------------+----+-----+-----+--------+
     pos: 0    4          1028  1079        2103   2154         3178   3229       4096   4102  4259  4310    4314
          \__________ ___________/ \________ _______/ \________ _______/ \_____________ _____________/
                     V                      V                  V                       V
                  packet0                packet1            packet2                 packet3
 */
static int tangox_read_page_raw(struct mtd_info *mtd,
	struct nand_chip *chip, uint8_t *buf, int oob_required, int page)
{
    struct nand_chip *this = (struct nand_chip *)mtd->priv;
    int i = 0;

    /* assume that read command is already inssued before get to here */
    tangox_set_padmode( PAD_MODE_PB );

    for (i = 0; i < mtd->writesize; i++)
        buf[i] = RD_HOST_REG8((RMuint32)this->IO_ADDR_R + SMP8XXX_REG_DATA);

    /* read data in oob range ( 2k: offset 2048 ~ 2112, 4k: ) */
    chip->cmdfunc(mtd, NAND_CMD_READOOB, 0x00, page);

    for (i = 0; i < mtd->oobsize; i++)
        chip->oob_poi[i] = RD_HOST_REG8((RMuint32)this->IO_ADDR_R + SMP8XXX_REG_DATA);

	return 0;
}

static int tangox_write_page_raw(struct mtd_info *mtd,
	struct nand_chip *chip, const uint8_t *buf, int oob_required)
{
    struct nand_chip *this = (struct nand_chip *)mtd->priv;
    int i = 0;

    /* assume that write command is already inssued before get to here */
    tangox_set_padmode( PAD_MODE_PB );

    for (i = 0; i < (mtd->writesize + mtd->oobsize); i++)
        WR_HOST_REG8((RMuint32)this->IO_ADDR_W + SMP8XXX_REG_DATA, buf[i]);

    return 0;
}

/**
 * tangox_verify_buf -  Verify chip data against buffer
 * @mtd:    MTD device structure
 * @buf:    buffer containing the data to compare
 * @len:    number of bytes to compare
 *
 *  verify function for 8bit buswith
 */
#if 0 //////////////////////////////////////////////////////////////
static int tangox_verify_buf(struct mtd_info *mtd, const u_char *buf, int len)
{
#ifdef USE_AUTOPIO
    u_char *tmpbuf = kmalloc(2048, GFP_KERNEL | GFP_DMA);   /* up to 2KB */
    int ret;
    
    debug("tangox_verify_buf\n");
    
    if (tmpbuf == NULL)
        return -ENOMEM;
    tangox_read_buf(mtd, tmpbuf, len);
    ret = (memcmp(buf, tmpbuf, len) == 0) ? 0 : -EIO;
    kfree(tmpbuf);
    return ret;
#else
    int i;
    struct nand_chip *this = mtd->priv;
    for (i = 0; i < len; i++) {
        if (buf[i] != RD_HOST_REG8((RMuint32)this->IO_ADDR_R + SMP8XXX_REG_DATA))
            return -EIO;
    }
    return 0;
#endif
}
#endif //////////////////////////////////////////////////////////////

/* tangox_nand_hwcontrol
 *
 * Issue command and address cycles to the chip
 */
static void tangox_nand_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
    register struct nand_chip *this = mtd->priv;
    
    /* set pad mode to pb */
    tangox_set_padmode( PAD_MODE_PB );
    
    if (cmd == NAND_CMD_NONE)
        return;

    if (ctrl & NAND_CLE)
        WR_HOST_REG8((RMuint32)this->IO_ADDR_W + SMP8XXX_REG_CMD, cmd);
    else
        WR_HOST_REG8((RMuint32)this->IO_ADDR_W + SMP8XXX_REG_ADDR, cmd);
}

/* tangox_nand_devready()
 *
 * returns 0 if the nand is busy, 1 if it is ready
 */
static int tangox_nand_devready(struct mtd_info *mtd)
{
    if (nand_ctrler != PB_NAND_CTRLER) {
        register struct nand_chip *chip = mtd->priv;
        unsigned int cs = ((struct chip_private *)chip->priv)->cs;

#if defined (CONFIG_8734_NAND_TIMING_WORKAROUND)
        udelay(10);
#endif        
        return ((RD_HOST_REG32(PB_CS_ctrl) & PB_IORDY) != 0) && 
            ((RD_HOST_REG32(STATUS_REG(chx_reg[cs])) & 0x80000000) != 0);
    } else
        return (RD_HOST_REG32(PB_CS_ctrl) & PB_IORDY) != 0;
}


/* ECC handling functions */
static inline int mu_count_bits(u32 v)
{
    int i, count;
    for (count = i = 0; (i < 32) && (v != 0); i++, v >>= 1)
        count += (v & 1);
    return count;
}


/* correct 512B packet */
static int ecc_correct_512(struct mtd_info *mtd, u_char *dat, u_char *read_ecc, u_char *calc_ecc)
{
    u32 mem, reg;
    mem = read_ecc[0] | ((read_ecc[1] & 0x0f) << 8) | ((read_ecc[1] & 0xf0) << 12) | (read_ecc[2] << 20);
    reg = calc_ecc[0] | ((calc_ecc[1] & 0x0f) << 8) | ((calc_ecc[1] & 0xf0) << 12) | (calc_ecc[2] << 20);
    if (likely(mem == reg))
        return 0;
    else {
        u16 pe, po, is_ecc_ff;
        is_ecc_ff = ((mem & 0x0fff0fff) == 0x0fff0fff);
        mem ^= reg;

        switch(mu_count_bits(mem)) {
            case 0:
                return 0;
            case 1:
                return -1;
            case 12:
                po = (u16)(mem & 0xffff);
                pe = (u16)((mem >> 16) & 0xffff);
                po = pe ^ po;
                if (po == 0x0fff) {
                    dat[pe >> 3] ^= (1 << (pe & 7));
                    return 1;   /* corrected data */
                } else 
                    return -1;  /* failed to correct */
            default:
                return (is_ecc_ff && (reg == 0)) ? 0 : -1;
        }
    }
    return -1;  /* should not be here */
}

/* correct 512B * 4 packets */
static int ecc_correct_2048(struct mtd_info *mtd, u_char *dat, u_char *read_ecc, u_char *calc_ecc)
{
    int ret0, ret1, ret2, ret3;
    
    ret0 = ecc_correct_512(mtd, dat, read_ecc, calc_ecc);
    ret1 = ecc_correct_512(mtd, dat + 512, read_ecc + 3, calc_ecc + 3);
    ret2 = ecc_correct_512(mtd, dat + 1024, read_ecc + 6, calc_ecc + 6);
    ret3 = ecc_correct_512(mtd, dat + 1536, read_ecc + 9, calc_ecc + 9);

    if ((ret0 < 0) || (ret1 < 0) || (ret2 << 0) || (ret3 << 0))
        return -1;
    else
        return ret0 + ret1 + ret2 + ret3;
}


/* ECC functions
 *
 * These allow the tangox to use the controller's ECC
 * generator block to ECC the data as it passes through]
*/
static int tangox_nand_calculate_ecc_512(struct mtd_info *mtd, const u_char *dat, u_char *ecc_code)
{
    ecc_code[0] = RD_HOST_REG8(PB_ECC_code0 + 0);
    ecc_code[1] = RD_HOST_REG8(PB_ECC_code0 + 1);
    ecc_code[2] = RD_HOST_REG8(PB_ECC_code0 + 2);

    //pr_debug("%s: returning ecc %02x%02x%02x\n", __func__,
    //   ecc_code[0], ecc_code[1], ecc_code[2]);

    return 0;
}

static int tangox_nand_calculate_ecc_2048(struct mtd_info *mtd, const u_char *dat, u_char *ecc_code)
{
    ecc_code[0] = RD_HOST_REG8(PB_ECC_code0 + 0);
    ecc_code[1] = RD_HOST_REG8(PB_ECC_code0 + 1);
    ecc_code[2] = RD_HOST_REG8(PB_ECC_code0 + 2);
    //pr_debug("%s: returning ecc %02x%02x%02x", __func__, ecc_code[0], ecc_code[1], ecc_code[2]);

    ecc_code[3] = RD_HOST_REG8(PB_ECC_code1 + 0);
    ecc_code[4] = RD_HOST_REG8(PB_ECC_code1 + 1);
    ecc_code[5] = RD_HOST_REG8(PB_ECC_code1 + 2);
    //pr_debug("%02x%02x%02x", ecc_code[3], ecc_code[4], ecc_code[5]);

    ecc_code[6] = RD_HOST_REG8(PB_ECC_code2 + 0);
    ecc_code[7] = RD_HOST_REG8(PB_ECC_code2 + 1);
    ecc_code[8] = RD_HOST_REG8(PB_ECC_code2 + 2);
    //pr_debug("%02x%02x%02x", ecc_code[6], ecc_code[7], ecc_code[8]);

    ecc_code[9] = RD_HOST_REG8(PB_ECC_code3 + 0);
    ecc_code[10] = RD_HOST_REG8(PB_ECC_code3 + 1);
    ecc_code[11] = RD_HOST_REG8(PB_ECC_code3 + 2);
    //pr_debug("%02x%02x%02x\n", ecc_code[9], ecc_code[10], ecc_code[11]);

    return 0;
}



static void tangox_nand_hwctl(struct mtd_info *mtd, int mode)
{
	register struct nand_chip *chip = mtd->priv;
	unsigned int cs = ((struct chip_private *)chip->priv)->cs;

   
	while ((RD_HOST_REG32(STATUS_REG(chx_reg[cs])) & 0x80000000) == 0)
		; /* unlikely it's not ready */
}

static void nand_command(struct mtd_info *mtd, unsigned int command, int column, int page_addr)
{
    register struct nand_chip *chip = mtd->priv;
    int ctrl = NAND_CTRL_CLE | NAND_CTRL_CHANGE;

    /*
     * Write out the command to the device.
     */
    if (command == NAND_CMD_SEQIN) {
        int readcmd;
        if (column >= mtd->writesize) {
            /* OOB area */
            column -= mtd->writesize;
            readcmd = NAND_CMD_READOOB;
        } else if (column < 256) {
            /* First 256 bytes --> READ0 */
            readcmd = NAND_CMD_READ0;
        } else {
            column -= 256;
            readcmd = NAND_CMD_READ1;
        }
        chip->cmd_ctrl(mtd, readcmd, ctrl);
        ctrl &= ~NAND_CTRL_CHANGE;
    }
    chip->cmd_ctrl(mtd, command, ctrl);

    /*
     * Address cycle, when necessary
     */
    ctrl = NAND_CTRL_ALE | NAND_CTRL_CHANGE;
    /* Serially input address */
    if (column != -1) {
        /* Adjust columns for 16 bit buswidth */
        if (chip->options & NAND_BUSWIDTH_16)
            column >>= 1;
        chip->cmd_ctrl(mtd, column, ctrl);
        ctrl &= ~NAND_CTRL_CHANGE;
    }
    if (page_addr != -1) {
        chip->cmd_ctrl(mtd, page_addr, ctrl);
        ctrl &= ~NAND_CTRL_CHANGE;
        chip->cmd_ctrl(mtd, page_addr >> 8, ctrl);
        /* One more address cycle for devices > 32MiB */
        if (chip->chipsize > (32 << 20))
            chip->cmd_ctrl(mtd, page_addr >> 16, ctrl);
    }
    chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

    /*
     * program and erase have their own busy handlers
     * status and sequential in needs no delay
     */
    switch (command) {

    case NAND_CMD_PAGEPROG:
    case NAND_CMD_ERASE1:
    case NAND_CMD_ERASE2:
    case NAND_CMD_SEQIN:
    case NAND_CMD_STATUS:
        return;

    case NAND_CMD_RESET:
        if (chip->dev_ready)
            break;
        udelay(chip->chip_delay);
        chip->cmd_ctrl(mtd, NAND_CMD_STATUS, NAND_CTRL_CLE | NAND_CTRL_CHANGE);
        chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
        while (!(chip->read_byte(mtd) & NAND_STATUS_READY)) ;
        return;

        /* This applies to read commands */
    default:
        /*
         * If we don't have access to the busy pin, we apply the given
         * command delay
         */
        if (!chip->dev_ready) {
            udelay(chip->chip_delay);
            return;
        }
    }
    /* Apply this short delay always to ensure that we do wait tWB in
     * any case on any machine. */
    ndelay(100);

    nand_wait_ready(mtd);
}



static void nand_command_lp(struct mtd_info *mtd, unsigned int command, int column, int page_addr)
{
    register struct nand_chip *chip = mtd->priv;

    /* Emulate NAND_CMD_READOOB */
    if (command == NAND_CMD_READOOB) {
        column += mtd->writesize;
        command = NAND_CMD_READ0;
    }

    /* Command latch cycle */
    chip->cmd_ctrl(mtd, command & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);

    if ((column != -1) || (page_addr != -1)) {
        int ctrl = NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE;

        /* Serially input address */
        if (column != -1) {
            /* Adjust columns for 16 bit buswidth */
            if (chip->options & NAND_BUSWIDTH_16)
                column >>= 1;
            chip->cmd_ctrl(mtd, column, ctrl);
            ctrl &= ~NAND_CTRL_CHANGE;
            chip->cmd_ctrl(mtd, column >> 8, ctrl);
        }
        if (page_addr != -1) {
            chip->cmd_ctrl(mtd, page_addr, ctrl);
            chip->cmd_ctrl(mtd, page_addr >> 8, NAND_NCE | NAND_ALE);
            /* One more address cycle for devices > 128MiB */
            if (chip->chipsize > (128 << 20))
                chip->cmd_ctrl(mtd, page_addr >> 16, NAND_NCE | NAND_ALE);
        }
    }
    chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

    /*
     * program and erase have their own busy handlers
     * status, sequential in, and deplete1 need no delay
     */
    switch (command) {

    case NAND_CMD_CACHEDPROG:
    case NAND_CMD_PAGEPROG:
    case NAND_CMD_ERASE1:
    case NAND_CMD_ERASE2:
    case NAND_CMD_SEQIN:
    case NAND_CMD_RNDIN:
    case NAND_CMD_STATUS:
    case NAND_CMD_DEPLETE1:
        return;

    /*
     * read error status commands require only a short delay
     */
    case NAND_CMD_STATUS_ERROR:
    case NAND_CMD_STATUS_ERROR0:
    case NAND_CMD_STATUS_ERROR1:
    case NAND_CMD_STATUS_ERROR2:
    case NAND_CMD_STATUS_ERROR3:
        udelay(chip->chip_delay);
        return;

    case NAND_CMD_RESET:
        if (chip->dev_ready)
            break;
        udelay(chip->chip_delay);
        chip->cmd_ctrl(mtd, NAND_CMD_STATUS, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
        chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
        while (!(chip->read_byte(mtd) & NAND_STATUS_READY))
            ;
        return;

    case NAND_CMD_RNDOUT:
        /* No ready / busy check necessary */
        chip->cmd_ctrl(mtd, NAND_CMD_RNDOUTSTART, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
        chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
        return;

    case NAND_CMD_READ0:
        chip->cmd_ctrl(mtd, NAND_CMD_READSTART, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
        chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

        /* This applies to read commands */
    default:
        /*
         * If we don't have access to the busy pin, we apply the given
         * command delay
         */
        if (!chip->dev_ready) {
            udelay(chip->chip_delay);
            return;
        }
    }

    /* Apply this short delay always to ensure that we do wait tWB in
     * any case on any machine. */
    ndelay(100);

    nand_wait_ready(mtd);
}

/**
 * tangox_command - Send command to NAND device
 * @mtd:    MTD device structure
 * @command:    the command to be sent
 * @column: the column address for this command, -1 if none
 * @page_addr:  the page address for this command, -1 if none
 *
 * Send command to NAND device. This function is used for small and large page
 * devices (256/512/2K/4K/8K Bytes per page)
 */
static void tangox_command(struct mtd_info *mtd, unsigned int command, int column, int page_addr)
{
    register struct nand_chip *chip = mtd->priv;
    unsigned int cs = ((struct chip_private *)chip->priv)->cs;

    switch(mtd->writesize) {
    case 512:
        /* 512B writesize may not be supported by current nand filesystems */
        nand_command(mtd, command, column, page_addr);
        break;
    case 2048:
    case 4096:
    case 8192:
        nand_command_lp(mtd, command, column, page_addr);
        break;
    default: /* very unlikely */
        tangox_nand_bug(mtd);
        break;
    }
    
    /* set New Controller address */
    WR_HOST_REG32(ADD_PAGE(chx_reg[cs]), page_addr); // page address
    WR_HOST_REG32(ADD_OFFSET(chx_reg[cs]), 0);   // offset always 0
}



/**
 * function to control hardware ecc generator.
 * Must only be provided if an hardware ECC is available
 */
static void tangox_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
    struct nand_chip *chip = mtd->priv;
    unsigned int cs = ((struct chip_private *)chip->priv)->cs;
       
    WR_HOST_REG32(PB_ECC_clear, 0x80000008 | cs);
}

static void tangox_select_chip(struct mtd_info *mtd, int chipnr)
{

#if 0 // temp   
//  static DEFINE_SPINLOCK(__mutex_lock);
    static int cs_cnt = 0; /* workaround asymetric select/de-select */
//  spin_lock_bh(&__mutex_lock);
    if (chipnr >= 0) {
        if (cs_cnt == 0) {
            if (tangox_mutex_lock(MUTEX_PBI)) {
                printk("%s:%d mutex acquisition failure.\n", __FILE__, __LINE__);   
            } else
                cs_cnt++;
        }
    } else {
        if (cs_cnt > 0) {
            cs_cnt--;
            tangox_mutex_unlock(MUTEX_PBI);
        }
    }
//  spin_unlock_bh(&__mutex_lock);
#else // temp for compilation

#endif  

}

static void tangox_set_nand_ctrler(void)
{
    unsigned long chip_id = (tangox_chip_id() >> 16) & 0xfffe;

    chx_reg[0] = MLC_CHA_REG;
    chx_reg[1] = MLC_CHB_REG;
    
    chx_mem[0] = MLC_CHA_MEM;
    chx_mem[1] = MLC_CHB_MEM;
    
    switch(chip_id) {
#ifndef CONFIG_TANGO3_SMP8656OTP
        case 0x8656:    /* OTP part uses old controller */
#endif
        case 0x8646:
            max_page_shift = 12; /* up to 4KB page */
        case 0x8670:
        case 0x8672:
        case 0x8674:
        case 0x8910:
        case 0x8920:
        case 0x8734:
        case 0x2400:
            nand_ctrler = MLC_NAND_CTRLER;
            break;
        case 0x8756:
        case 0x8758:
            nand_ctrler = MLC2_NAND_CTRLER;
            
            chx_reg[0] = MLC2_CHA_REG;
            chx_reg[1] = MLC2_CHB_REG;

            chx_mem[0] = MLC2_CHA_MEM;
            chx_mem[1] = MLC2_CHB_MEM;
            break;
        default:
            nand_ctrler = PB_NAND_CTRLER; /* use old controller */
            break;
    }
}

int board_nand_post_init(nand_info_t *mtds, struct nand_chip *nand )
{
    int writesize = mtds->writesize;

    // do we really need this?
    //mtds->priv = nand; 

    debug( "writesize: %d\n", writesize );
    
    if ( writesize == 0 )
        goto next;
    
    nand->ecc.mode     = NAND_ECC_HW;
    nand->ecc.steps    = 1;
    nand->ecc.hwctl    = tangox_nand_enable_hwecc;
    nand->select_chip  = tangox_select_chip;
       
    nand->ecc.read_page_raw   = tangox_read_page_raw;
    nand->ecc.write_page_raw  = tangox_write_page_raw;

    if (nand_ctrler != PB_NAND_CTRLER) {
        nand->ecc.write_page = tangox_write_page_hwecc;
        nand->ecc.read_page  = tangox_read_page_hwecc;
        nand->cmdfunc        = tangox_command;
        nand->ecc.calculate  = tangox_nand_bug_calculate;
        nand->ecc.correct    = tangox_nand_bug_correct;
        nand->ecc.hwctl      = tangox_nand_hwctl;
    }

    switch (writesize) {
        case 512:
            if (nand_ctrler == MLC2_NAND_CTRLER ) {
                
            }
            else if (nand_ctrler == MLC_NAND_CTRLER ) {
                nand->ecc.layout = &tangox_nand_mlc1_ecclayout512_16_4;
                nand->ecc.layout->oobfree[0].length = mtds->oobsize 
                                    - nand->ecc.layout->oobfree[0].offset;
                nand->ecc.strength = 4;
            } else {
                nand->ecc.calculate = tangox_nand_calculate_ecc_512;
                nand->ecc.correct   = ecc_correct_512;
                nand->ecc.bytes     = nand->ecc.total = 3;
                nand->ecc.layout    = &tangox_nand_pb_ecclayout512_16;
            }
            nand->ecc.size = 512;
            break;

        case 2048:
            if (nand_ctrler == MLC2_NAND_CTRLER ) {
                nand->ecc.layout =  &tangox_nand_mlc2_ecclayout2048_64_14;
                nand->ecc.layout->oobfree[0].length = 0;

                nand->ecc.strength = 14;
            }
            else if (nand_ctrler == MLC_NAND_CTRLER ) {
                nand->ecc.layout = &tangox_nand_mlc1_ecclayout2048_64_8;
                nand->ecc.layout->oobfree[0].length = mtds->oobsize 
                                    - nand->ecc.layout->oobfree[0].offset;
                nand->ecc.strength = 8;
            } else {
                nand->ecc.calculate = tangox_nand_calculate_ecc_2048;
                nand->ecc.correct   = ecc_correct_2048;
                nand->ecc.bytes     = nand->ecc.total = 12;
                nand->ecc.layout    = &tangox_nand_pb_ecclayout2048_64;
            }
            nand->ecc.size = 2048;
            break;  

        case 4096:  
            if (nand_ctrler == MLC2_NAND_CTRLER ) {
                if ( (mtds->oobsize >= 218) && (mtds->oobsize < 224) ) {
                    nand->ecc.layout = &tangox_nand_mlc2_ecclayout4096_218_27;
                    nand->ecc.layout->oobfree[0].length = mtds->oobsize
                                        - nand->ecc.layout->oobfree[0].offset;
                } else if ( (mtds->oobsize >= 224) && (mtds->oobsize < 232) ) {
                    nand->ecc.layout = &tangox_nand_mlc2_ecclayout4096_224_27;
                    nand->ecc.layout->oobfree[0].length = mtds->oobsize
                                        - nand->ecc.layout->oobfree[0].offset;
                } else if ( mtds->oobsize >= 232 ) {
                    nand->ecc.layout = &tangox_nand_mlc2_ecclayout4096_232_27;
                    nand->ecc.layout->oobfree[0].length = mtds->oobsize 
                                        - nand->ecc.layout->oobfree[0].offset;
                }
                nand->ecc.size = 4096;
                nand->ecc.strength = 27;
            }
            else if (nand_ctrler == MLC_NAND_CTRLER ) {
                if ((mtds->oobsize >= 128) && (mtds->oobsize < 218)) {
                    nand->ecc.layout = &tangox_nand_mlc1_ecclayout4096_128_9;
                    nand->ecc.layout->oobfree[0].length = mtds->oobsize 
                                        - nand->ecc.layout->oobfree[0].offset;
                    nand->ecc.strength = 9;
                } else if (mtds->oobsize >= 218) {
                    nand->ecc.layout = &tangox_nand_mlc1_ecclayout4096_218_16;
                    nand->ecc.layout->oobfree[0].length = mtds->oobsize 
                                        - nand->ecc.layout->oobfree[0].offset;
                    nand->ecc.strength = 16;
                } else {
                    printf( "Unsuppoted NAND...!\n");
                    goto next;
                }
                nand->ecc.size = 4096;
            } else
                goto next;
            break;

        case 8192: 
            if (nand_ctrler == MLC2_NAND_CTRLER ) {
                nand->ecc.layout = &tangox_nand_mlc2_ecclayout8192_448_28;
                nand->ecc.layout->oobfree[0].length = mtds->oobsize 
                                - nand->ecc.layout->oobfree[0].offset;
                nand->ecc.size = 8192;
                nand->ecc.strength = 28;
                
            }
            else if ( (nand_ctrler == MLC_NAND_CTRLER) && (max_page_shift >= 13) ) {
                nand->ecc.layout = &tangox_nand_mlc1_ecclayout8192_448_16;
                nand->ecc.layout->oobfree[0].length = mtds->oobsize 
                                - nand->ecc.layout->oobfree[0].offset;
                nand->ecc.size = 8192;
                nand->ecc.strength = 16;
            } else
                goto next;
            break;
        default:
            ///printf("Unsupported NAND...!\n");
            goto next;
    }

    return 0;
next:
    return -1;
}

int tangox_board_nand_init(struct nand_chip *nand)
{
    u32 mem_staddr;
    int cs = 0;
        
    /* find nand controller type */
    tangox_set_nand_ctrler();

    /* read configurations only once */ 
    if ( chip_cnt == 0 ){

        local_pb_cs_ctrl    = RD_HOST_REG8 ( PB_CS_ctrl   );
        local_pb_cs_config  = RD_HOST_REG32( PB_CS_config );
        local_pb_cs_config1 = RD_HOST_REG32( PB_CS_config1);
        
        //printf( "cs_ctrl: 0x%x\ncs_config: 0x%x\ncs_config1: 0x%x\n",
        //      local_pb_cs_ctrl, local_pb_cs_config, local_pb_cs_config1 );

        switch((local_pb_cs_ctrl >> 4) & 7) {
            case 0: cs_offset = 0x200;
                break;
            case 1: 
            case 2: cs_offset = 0x100;
                break;
            default:
                printf("No NAND flash is available (0x%x).\n", local_pb_cs_ctrl);
                return -EIO;
        }

        if( MAX_CS < 4 ){
            // how many nand devices we have?
            for (cs = 0; cs < MAX_CS; cs++) {
                if ((local_pb_cs_config >> (20 + cs)) & 1)
                    cs_avail[cs] = 1;
            }
        }else{
            for (cs = 0; cs < MAX_CS-4; cs++){
                if ((local_pb_cs_config1 >> (20 + cs)) & 1)
                    cs_avail[cs + 4] = 1;
            }
        }

    }

    if (cs_avail[chip_cnt] == 0)
        goto next;
    
    // TODO: need to find out what the below code is        
    if (nand_ctrler != PB_NAND_CTRLER) { /* bounce buffer may be needed with new controller */
        if ((chip_privs[chip_cnt].bbuf = (uint8_t *)NON_CACHED(malloc(NAND_MAX_PAGESIZE))) == NULL) {   /* up to 8KB */
            ///chip_privs[chip_cnt].bbuf = (uint8_t *)tangox_dma_address( (unsigned long)chip_privs[chip_cnt].bbuf );
            cs_avail[cs] = 0;
            goto next;
        }
    } else 
        chip_privs[chip_cnt].bbuf = NULL;
        
    chip_privs[chip_cnt].cs = chip_cnt;
    
    nand->priv = &chip_privs[chip_cnt];
    mem_staddr = chip_cnt * cs_offset;

    /* 30 us command delay time */
    nand->chip_delay   = 30;

    nand->ecc.mode     = NAND_ECC_SOFT;
    //nand->options      = NAND_NO_AUTOINCR | BBT_AUTO_REFRESH;
    //nand->options      = NAND_SKIP_BBTSCAN;
    nand->controller   = &tangox_hw_control;

    nand->read_byte    = tangox_read_byte;
    nand->read_buf     = tangox_read_buf;
    nand->write_buf    = tangox_write_buf;
    //nand->verify_buf   = tangox_verify_buf;

    nand->cmd_ctrl     = tangox_nand_hwcontrol;
    nand->dev_ready    = tangox_nand_devready;     

    nand->IO_ADDR_W    = nand->IO_ADDR_R = (void __iomem *)mem_staddr;
    
    // use default IO address

    tangox_set_padmode( PAD_MODE_PB );
    
    /* nand reset */
    WR_HOST_REG8((RMuint32)nand->IO_ADDR_W + SMP8XXX_REG_CMD, NAND_CMD_RESET);
    udelay(tReset);

    //printf("Checking NAND device on CS%d ..\n", chip_cnt);


    if (nand_ctrler != PB_NAND_CTRLER ) {
#define DEF_TIMING1	0x1b162c16	/* conservative timing1 */
#define DEF_TIMING2	0x120e1f58	/* conservative timing2 */
#define DEF_DEVCFG	0x35		/* default devcfg, may not be correct */

//taken from  ApplicationNote64MLC2 MLC schemes DEFAULT. boot-everywhere format
#define DEF_TIMING1_MLC2 0x4c4c261c	/* conservative timing1 */
#define DEF_TIMING2_MLC2 0x1c12194c	/* conservative timing2 */
#define DEF_DEVCFG_MLC2  0x061d1135 /* default devcfg, may not be correct */

        u32 timing1, timing2, devcfg;
        u32 def_timing1, def_timing2, def_devcfg;
        char buf[BUFSIZE];
        u32 dsize, params[10];

        if ( nand_ctrler == MLC_NAND_CTRLER ) {
            def_timing1 = DEF_TIMING1;
            def_timing2 = DEF_TIMING2;
            def_devcfg  = DEF_DEVCFG;
        }
        else {
            def_timing1 = DEF_TIMING1_MLC2;
            def_timing2 = DEF_TIMING2_MLC2;
            def_devcfg  = DEF_DEVCFG_MLC2;
        }

        memset( params, 0x00, sizeof(params) );

        /* read a.nandpart0 */
        sprintf(buf, NAND_PARAM, chip_cnt);
        dsize = sizeof(params);

        /* read nand param key first */
        if ((xenv_get((void *)xenv_addr, MAX_XENV_SIZE, buf, params, &dsize) < 0) && (dsize == 0)) {
            sprintf(buf, CS_TIMING1, chip_cnt);
            dsize = sizeof(u32);

            if ((xenv_get((void *)xenv_addr, MAX_XENV_SIZE, buf, &timing1, &dsize) < 0) || (dsize != sizeof(u32)))
                timing1 = (params[3] ? params[3] : def_timing1);

            sprintf(buf, CS_TIMING2, chip_cnt);
            dsize = sizeof(u32);

            if ((xenv_get((void *)xenv_addr, MAX_XENV_SIZE, buf, &timing2, &dsize) < 0) || (dsize != sizeof(u32)))
                timing2 = (params[4] ? params[4] : def_timing2);

            sprintf(buf, CS_DEVCFG, chip_cnt);
            dsize = sizeof(u32);

            if ((xenv_get((void *)xenv_addr, MAX_XENV_SIZE, buf, &devcfg, &dsize) < 0) || (dsize != sizeof(u32)))
                devcfg = (params[5] ? params[5] : def_devcfg);
        }
        else {
            timing1 = params[3];
            timing2 = params[4];
            devcfg  = params[5];
        }

        WR_HOST_REG32(DEVICE_CFG(chx_reg[chip_cnt]), devcfg);
        WR_HOST_REG32(TIMING1(chx_reg[chip_cnt]), timing1);
        WR_HOST_REG32(TIMING2(chx_reg[chip_cnt]), timing2);
    }

    chip_cnt++;

    return 0;

next:
    cs_avail[chip_cnt] = 0;
    chip_cnt++;
    //continue;
    printk("No NAND flash is detected.\n");
    return -EIO;

}

#if 0
int load_mtd_partiotion ( struct mtd_info *mtd, int cs )
{
     /* let's read partition info from xenv and export it to env variable */
    char xenv_key[128];
    char part_name[128];
    char env_buffer[256];
    char env_temp[128];
    //char *env_ptr = NULL;

    //int  cs = maxchips - 1;
    int  num_parts = 0;
    int  val, sizex, i, ret;
    //u32  dsize, rsvd_blks;
    //u64  rsvd_sz, psz, poff;

    unsigned int part_offset[MAX_CS];
    unsigned int part_size[MAX_CS];


    /* get reserved partition info */
    sprintf( xenv_key, CS_RESERVED, cs);
    sizex = 4;
    ret = xenv_get( (void *)xenv_addr, 
                     MAX_XENV_SIZE, 
                     xenv_key, 
                     &val, 
                     (void *)&sizex);
            
    if (( ret == RM_OK) && (sizex == 4)) {
        debug("cs%d reserved size: 0x%x\n", cs, val );
        //debug("tot size: %d, mtd size: %d\n", val * mtd->erasesize, mtd->size );
        //rsvd_sz   = min( mtd->erasesize * (u64)rsvd_blks, tangox_mtds[cs].size);
        //mtd->size = rsvd_sz;

    }
    else {
        printf("cannot find the key: %s\n", xenv_key );
        return -1;
    }
 

    sprintf( xenv_key, CS_PARTS, cs );          

    /* number of partition */
    sizex = 4;
    ret = xenv_get( (void *)xenv_addr, 
                     MAX_XENV_SIZE, 
                     xenv_key, 
                     &val, 
                     (void *)&sizex);
            
    if (( ret == RM_OK) && (sizex == 4)) {
        debug("num partitions: 0x%x\n", val );
        if ( val > MAX_PARTITIONS )
            num_parts = MAX_PARTITIONS;
        else
            num_parts = val;
    }
    else // if NAND flash layout is sigma nand lib format, they will return here.
        return -1;

    /* prepare mtdids env param */
    //env_ptr = getenv( ENV_MTD_IDS );
    val = strlen( env_mtdids );

    if ( val != 0 ) {
        /* add to existing env param */
        //printf("env_ptr: %s\n", env_ptr );
        strcpy( env_buffer, env_mtdids);
        sprintf( env_temp, ",nand%d="ENV_NAND_NAME, cs, cs );
        strcat( env_buffer, env_temp );
    }
    else {
        /* wew will creare new env param */
        sprintf( env_buffer, "nand%d="ENV_NAND_NAME, cs, cs );
    }

    debug( "env_buffer: %s\n", env_buffer );
    ///setenv( ENV_MTD_IDS, env_buffer );
    strcpy( env_mtdids, env_buffer );

    /* prepare mtdparts env param */
    val = strlen( env_mtdparts );
    
    if ( val != 0 ) {
        strcpy( env_buffer, env_mtdparts);
        sprintf( env_temp, ENV_NAND_NAME":", cs );
        strcat( env_buffer, env_temp );

    }
    else {
        sprintf( env_buffer,"mtdparts="ENV_NAND_NAME":", cs );
    }
    ///setenv( ENV_MTD_PARTS, env_buffer);
    strcpy( env_mtdparts, env_buffer );

    /* per partition process */ 
    for ( i = 1; i < num_parts+1; i++ ) {
        
        //read part offset and size
        /* read offset first */
        sprintf( xenv_key, CS_PART_OFFSET, cs, i );
    
        sizex = 4;
        ret = xenv_get( (void *)xenv_addr, 
                         MAX_XENV_SIZE, 
                         xenv_key, 
                         &val, 
                         (void *)&sizex);
            
        if (( ret == RM_OK) && (sizex == 4)) {
            debug("(part%d)offset: 0x%x\n", i, val );
            part_offset[i-1] = val;
        }
        else
            return -1;        
       
        /* read size of partition */
        sprintf( xenv_key, CS_PART_SIZE, cs, i );
    
        sizex = 4;
        ret = xenv_get( (void *)xenv_addr, 
                         MAX_XENV_SIZE, 
                         xenv_key, 
                         &val, 
                         (void *)&sizex);
            
        if (( ret == RM_OK) && (sizex == 4)) {
            debug("(part%d)size: 0x%x\n", i, val );
            part_size[i-1] = val;
        }
        else
            return -1;        
     
        /* read partition name */  
        sprintf( xenv_key, CS_PART_NAME, cs, i );
        memset( part_name, 0x00, sizeof(part_name) );
 
        sizex = sizeof(part_name);
        ret = xenv_get( (void *)xenv_addr, 
                         MAX_XENV_SIZE, 
                         xenv_key, 
                         part_name, 
                         (void *)&sizex);
            
        if (( ret == RM_OK) && (sizex > 0)) {
            debug("(part%d)name: %s\n", i, part_name );
            
        }
        else
            return -1;        
    
        /* export mtd partition table to u-boot env */
        if ( i == num_parts )
            sprintf( env_temp,"0x%08x(%s)", part_size[i-1], part_name );
        else
            sprintf( env_temp,"0x%08x(%s),", part_size[i-1], part_name );

        //strcat( env_buffer, env_temp );
        strcat( env_mtdparts, env_temp );

    } // end of for loop

    debug("env_mtdparts: %s\n", env_mtdparts );
    ///setenv( ENV_MTD_PARTS, env_buffer );

    return 0;
 
}
#endif

int tangox_nand_scan(struct mtd_info *mtd, struct nand_chip *nand, int maxchips, int chip_idx )
{
   	int ret;

	ret = tangox_nand_scan_ident(mtd, chip_idx /*maxchips*/, NULL);

	if (!ret) {
        /* we should set hw ecc related stuff before call scan tail  */     
        ret = board_nand_post_init(mtd, nand);
   
        if ( !ret ) {
    		ret = nand_scan_tail(mtd);
#if 0            
            if( !ret ) {
                /* populate mtd partition table */
                load_mtd_partiotion( mtd, chip_idx );
            }
#endif
        }
    }

	return ret;
}


static void nand_init_chip(int i)
{
	struct mtd_info *mtd   = &nand_info[i];
	struct nand_chip *nand = &nand_chip[i];
	//ulong base_addr        = base_address[i];
	int maxchips           = CONFIG_SYS_NAND_MAX_CHIPS;

	if (maxchips < 1)
		maxchips = 1;

	mtd->priv = nand;
	//nand->IO_ADDR_R = nand->IO_ADDR_W = (void  __iomem *)base_addr;

	if (tangox_board_nand_init(nand))
		return;
    //int tangox_nand_scan(struct mtd_info *mtd, struct nand_chip *nand, int maxchips, int chip_idx )
	if (tangox_nand_scan(mtd, nand, maxchips, i))
		return;

	nand_register(i);
}

/* nand self init routine */
void board_nand_init(void)
{
    int i;

    /* init chip and register to mtd */
    for (i = 0; i < CONFIG_SYS_MAX_NAND_DEVICE; i++)
        nand_init_chip(i);
}


/* the env_reloc() is called after the NAND driver init.
   this may cause memroy corruption becuase setenv(), getenv() will
   access memory before relocation though the u-boot itself has been
   relocated already. 

   we may place the env_reloc() before nand driver init, but it may not be
   safe to the other boards sharing the same board init routine.
   
*/
void tangox_nand_export_mtdinfo(void)
{
    // let's export the mtd information to u-boot env
    setenv( ENV_MTD_IDS, env_mtdids );
    setenv( ENV_MTD_PARTS, env_mtdparts ); 
}

