#ifndef __MONZA_NAND_H__
#define __MONZA_NAND_H__

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <asm/arch/reg_io.h>

#define BASE                 (CONFIG_PLF_MMIO_BASE2 + 0x2000)  /* here goes nand reg base from top config */

#define NAND_BOOTROM_R_TIMG  (BASE + 0x00000000) /* boot rom read timing control register */
#define NAND_FCR             (BASE + 0x00000004) /* flash rom control register */
#define NAND_DCR             (BASE + 0x00000008) /* data control register */
#define NAND_TCR             (BASE + 0x0000000c) /* timing control register */
#define NAND_SYN_MODE0       (BASE + 0x00000010) /* sync mode control 0 register */
#define NAND_SYN_MODE1	     (BASE + 0x00000014) /* sync mode control 1 register */
#define NAND_SPI_MODE_CMD    (BASE + 0x00000018) /* sync mode control 2 register */
#define NAND_OP_MODES        (BASE + 0x0000001c) /* */

#define SHIFT_WORK_MODE(a)   ((a) & 0x0000000f) << 26)
#define SHIFT_RD_FIN_SW(a)   ((a) & 0x00000001) << 25)
#define SHIFT_WR_FIN_SW(a)   ((a) & 0x00000001) << 24)
#define SHIFT_RD_BACK_SW(a)  ((a) & 0x000000ff) << 15)
#define SHIFT_CS_HI_TIME(a)  ((a) & 0x000000ff) << 8 )
#define SHIFT_SPI_TX_SW(a)   ((a) & 0x000000ff)      )

#define NAND_SPI_DLY_CTRL    (BASE + 0x0000020) /* SPI mode clock delay control register */
#define NAND_SPI_TIMG_CTRL0  (BASE + 0x0000024) /* SPI mode timing control register 0 */
#define NAND_SPI_TIMG_CTRL1  (BASE + 0x0000028) /* SPI mode timing control register 1 */
#define NAND_BASE_ADDR0      (BASE + 0x000002c) /* base address register 0 */
#define NAND_BASE_ADDR1      (BASE + 0x0000030) /* base address register 1 */
#define NAND_BURST_TIMG      (BASE + 0x0000034) /* flashrom burst timing control register */
#define NAND_BASE_ADDR2      (BASE + 0x0000038) /* base address register 2 */
#define NAND_BASE_ADDR3      (BASE + 0x000003c) /* base address register 3 */
#define NAND_BASE_ADDR4      (BASE + 0x0000040) /* base address register 4 */

#define NAND_ONE_DATA_ECC0   (BASE + 0x00000044) /* onenand flash data spcae ecc registe 0 */
#define NAND_ONE_DATA_ECC1   (BASE + 0x00000048) /* onenand flash data spcae ecc registe 1 */
#define NAND_ONE_DATA_ECC2   (BASE + 0x0000004c) /* onenand flash data spcae ecc registe 2 */
#define NAND_ONE_SPR_ECC0    (BASE + 0x00000050) /* onenand flash spare space ecc register 0 */
#define NAND_ONE_SPR_ECC1    (BASE + 0x00000054) /* onenand flash spare space ecc register 1 */

#define NAND_CTRL            (BASE + 0x00000058) /* nand flash control register */
#define NAND_TIMG            (BASE + 0x0000005c) /* nand flash timing register */

/*SX7 new nand controller*/
#define NAND_CCR             (BASE + 0x000000A0) /* Command list control Register */

/* ---------------------------------------------------------------------
   address hole in the middle
   ---------------------------------------------------------------------*/

#define NAND_MLC_ECC_CTRL            (BASE + 0x00000070) /* MLC ECC control register */
#define NAND_MLC_ESS_STATUS          (BASE + 0x00000074) /* MLC ECC status register */
#define NAND_MLC_ECC_DATA            (BASE + 0x00000078) /* MLC ECC data register */
#define NAND_MLC_ECC_ENC_DATA1       (BASE + 0x0000007c) /* MLC ECC encoder data register 1*/
#define NAND_MLC_ECC_DEC_ERR_POS0    (BASE + 0x00000080) /* MLC ECC decoder error position egister 0*/
#define NAND_MLC_ECC_DEC_ERR_POS1    (BASE + 0x00000084) /* MLC ECC decoder error position egister 1*/

#define NAND_FDMA_SRC_ADDR           (0x00000088) /*FDMA SOURCE ADDR*/
#define NAND_FDMA_DEST_ADDR          (0x0000008c) /*FDMA DESTINATION ADDR*/
#define NAND_FDMA_INT_ADDR           (0x00000090) /*FDMA INTERRUPT ADDR*/
#define NAND_FDMA_LEN_ADDR           (0x00000094) /*FDMA LENGTH ADDR */

/* ---------------------------------------------------------------------
 *    ECC related register
 * ---------------------------------------------------------------------*/
#define NAND_ECC_CTRL                (BASE + 0x00000200) /* ECC control register */


#define MLC_ECC_REG_CONTROL     (BASE + 0x00000070) /* MLC ECC control register */
#define MLC_ECC_REG_STATUS      (BASE + 0x00000074) /* MLC ECC status register */

#define MLC_ECC_CONTROL_DECODER_CLEAR    0x08
#define MLC_ECC_CONTROL_DECODER_START    0x04
#define MLC_ECC_CONTROL_ENCODER_CLEAR    0x02
#define MLC_ECC_CONTROL_ENCODER_START    0x01


#define REG_BCH_ECC_BASE                (BASE + 0x00000200) /* ECC control register */
#define REG_BCH_ECC_MAP(x)              (REG_BCH_ECC_BASE + x)

#define REG_BCH_ECC_CONTROL             REG_BCH_ECC_MAP(0x00)
#define REG_BCH_ECC_STATUS              REG_BCH_ECC_MAP(0x04)

#define REG_BCH_ECC_DATA_0              REG_BCH_ECC_MAP(0x10)
#define REG_BCH_ECC_DATA_1              REG_BCH_ECC_MAP(0x14)
#define REG_BCH_ECC_DATA_2              REG_BCH_ECC_MAP(0x18)
#define REG_BCH_ECC_DATA_3              REG_BCH_ECC_MAP(0x1C)
#define REG_BCH_ECC_DATA_4              REG_BCH_ECC_MAP(0x20)
#define REG_BCH_ECC_DATA_5              REG_BCH_ECC_MAP(0x24)
#define REG_BCH_ECC_DATA_6              REG_BCH_ECC_MAP(0x28)
#define REG_BCH_ECC_DATA_7              REG_BCH_ECC_MAP(0x2C)
#define REG_BCH_ECC_DATA_8              REG_BCH_ECC_MAP(0x30)
#define REG_BCH_ECC_DATA_9              REG_BCH_ECC_MAP(0x34)
#define REG_BCH_ECC_DATA_a              REG_BCH_ECC_MAP(0x38)
#define REG_BCH_ECC_DATA_b              REG_BCH_ECC_MAP(0x3C)

#ifndef CONFIG_SIGMA_SOC_UXLB
# define REG_BCH_ECC_CORRECTION_0        REG_BCH_ECC_MAP(0x60)
#else
# define REG_BCH_ECC_CORRECTION_0        REG_BCH_ECC_MAP(0x50)
#endif
#define REG_BCH_ECC_INT_SET_ENABLE      REG_BCH_ECC_MAP(0xDC)


/*
 * ECC_CONTROL
 */
#define ECC_CONTROL_CLEAR               (1<<8)
#define ECC_CONTROL_PARITY_MODE         (1<<6)      // O:DECODE, 1:ENCODE
#define ECC_CONTROL_MODE_OPERATION      (3<<4)      // 00:4 BITS ECC, 01:8 BITS ECC
#define ECC_CONTROL_MODE_OPERATION_4BITS      (3<<4)      // 00:4 BITS ECC, 01:8 BITS ECC
#define ECC_CONTROL_MODE_OPERATION_8BITS      (1<<4)      // 00:4 BITS ECC, 01:8 BITS ECC
#define ECC_CONTROL_PAYLOAD_SIZE        (3<<2)      // 01:512 BYTES
#define ECC_CONTROL_PAYLOAD_SIZE_512B   (1<<2)      // 01:512 BYTES
#define ECC_CONTROL_ENABLE_NFC          (1<<1)
#define ECC_CONTROL_ENABLE_CALC         (1<<0)

/*
 * ECC_STATUS
 */
#define ECC_STATUS_NUM_CORRECTIONS      (0x3f<<20)
#define ECC_STATUS_ERASED_PAGE          (1<<19)
#define ECC_STATUS_UNCORRECTABLE_ERR    (1<<18)
#define ECC_STATUS_CORRECTION_VALID     (1<<17)
#define ECC_STATUS_PARITY_VALID         (1<<16)
#define ECC_STATUS_WORD_COUNT           (0xffff<<0)

#define MONZA_ECC_BUSY_WAIT_TIMEOUT (1 * CONFIG_SYS_HZ)

#define MONZA_DMA_TRANS_TIMEOUT     (2 * CONFIG_SYS_HZ)
/* ---------------------------------------------------------------------
   ECC related register end
   ---------------------------------------------------------------------*/

#define NAND_SW_MODE    0
#define NAND_HW_MODE    0x44

#define DMAUNIT 0x400
enum nand_io {
        MONZA_NAND_PIO = 0,      /* PIO mode*/
        MONZA_NAND_DMA,         /* enabled FDMA mode */
};

enum nand_host {
	MONZA_NAND_OLD = 0,
	MONZA_NAND_NEW,
};

struct monza_nand_info{
        struct nand_chip            nand;

        enum nand_io                xfer_type;
	enum nand_host		    nand_ctrler;  //nand host controller

        unsigned char               *data_buff_virt;
        dma_addr_t                  data_buff_phys;
        unsigned int                *dma_int_descr_virt;
        dma_addr_t                  dma_int_descr_phys;

/*
 *  max number of correctible bits per ECC step
 *  the member will be added nand_chip->ecc.strength of u-boot.2014-4
 */
	int                         ecc_strength;
};

/*
 * this macro allows us to convert from an MTD structure to our own
 * device context (monza) structure.
 */
#define mtd_to_monza(mtd) (((struct nand_chip *)mtd->priv)->priv)

#endif
