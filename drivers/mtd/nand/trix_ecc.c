#include <common.h>

#include <malloc.h>
#include <linux/err.h>
//#include <linux/mtd/compat.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/nand_bch.h>
#include <asm/arch/reg_io.h>

#include "trix_nand.h"

void bch_encode_start(struct nand_chip *chip)
{
        struct monza_nand_info *info = container_of(chip,
                                               struct monza_nand_info, nand);

        unsigned int ecc_unit  = (chip->ecc.size >> 9) & 0x3; /*[3:2] 01-512bytes 10-1024bytes*/
        unsigned int ecc_level = info->ecc_strength & 0x1F;   /*[9:4] ecc level*/

        writel(0x403 + (ecc_unit<<2) + (ecc_level<<4) , REG_BCH_ECC_CONTROL);
        // enable the MLC encoder
        writel(0x101 ,MLC_ECC_REG_CONTROL);
}

int bch_encode_end(unsigned int ecc[],struct nand_chip *chip)
{
        /*struct monza_nand_info *info = container_of(chip,
                                                struct monza_nand_info, nand);*/
        int err = 0;
        int i;

        unsigned int times = (ALIGN(chip->ecc.bytes, 4) >> 2) - 1;
        unsigned int ecc_align = ALIGN(chip->ecc.bytes, 4) - chip->ecc.bytes;
        unsigned int ecc_align_mask[4] = {0xFFFFFFFF,0x00FFFFFF,0x0000FFFF,0x000000FF};

	unsigned long start ;
        unsigned long deadline = MONZA_ECC_BUSY_WAIT_TIMEOUT;
	
	start = get_timer(0);
        do{
                if(readl(REG_BCH_ECC_STATUS) & ECC_STATUS_PARITY_VALID)
                        break;
	}while (get_timer(start) < deadline);

        if (get_timer(start) > deadline)
        {
                printf("ecc encode timed out\n");
                err = -ETIMEDOUT;
                goto exit;
        }

        for(i = 0; i <= times; i ++)
                ecc[i] = readl( REG_BCH_ECC_DATA_0 + (i<<2) );

	ecc[times] &= ecc_align_mask[ecc_align];

exit:
    // clear MCL encoder
    writel(0x102 ,MLC_ECC_REG_CONTROL);

    return err;
}

#if 0
static int get_bitflip_count(unsigned int value)
{
        int count;

        for (count = 0; value; value >>= 1)
                 count += value & 1;

        return count;
}
#endif
/* Count the number of 0's in buff upto a max of max_bits */
static int count_written_bits(uint8_t *buff, int size, int max_bits)
{
        int k, written_bits = 0;

        for (k = 0; k < size; k++) {
                written_bits += hweight8(~buff[k]);
                if (written_bits > max_bits)
                        break;
        }

        return written_bits;
}

void bch_decode_start(unsigned int ecc[],struct nand_chip * chip)
{
        struct monza_nand_info *info = container_of(chip,
                                                struct monza_nand_info, nand);
    // enable the MLC decoder
        unsigned int i;
        unsigned int ecc_unit  = (chip->ecc.size >> 9) & 0x3; /*[3:2] 01-512bytes 10-1024bytes*/
        unsigned int ecc_level = info->ecc_strength & 0x1F;   /*[9:4] ecc level*/
        
	unsigned int times = (ALIGN(chip->ecc.bytes, 4) >> 2) - 1;
        unsigned int ecc_align = ALIGN(chip->ecc.bytes, 4) - chip->ecc.bytes;
        unsigned int ecc_align_mask[4] = {0xFFFFFFFF,0x00FFFFFF,0x0000FFFF,0x000000FF};
        
	ecc[times] &= ecc_align_mask[ecc_align];
	
	writel(0x3 + (ecc_unit<<2) + (ecc_level<<4) ,REG_BCH_ECC_CONTROL);

        writel(0x108 ,MLC_ECC_REG_CONTROL);
        writel(0x104 ,MLC_ECC_REG_CONTROL);

        for(i = 0; i <= times; i ++)
                writel(ecc[i] ,(REG_BCH_ECC_DATA_0 + (i<<2)));
}


int bch_decode_end_correct(struct mtd_info *mtd,unsigned char* pBuff,struct nand_chip * chip)
{
	struct monza_nand_info *info = mtd_to_monza(mtd);
	
	int err = 0;
        unsigned int status;
        unsigned int bitnum;

        uint8_t *read_ecc = chip->buffers->ecccode;
        unsigned short* pwBuff;
        unsigned int dwCorrection;
        unsigned int correction, offset;

	unsigned long start ;
        unsigned long deadline = MONZA_ECC_BUSY_WAIT_TIMEOUT;

	start = get_timer(0);
	do{
                if(readl(REG_BCH_ECC_STATUS) & ECC_STATUS_CORRECTION_VALID)
                        break;
	}while (get_timer(start) < deadline);

        if ( get_timer(start) > deadline )
        {
                printf("ecc decode timed out\n");
                err = -ETIMEDOUT;
                goto exit;
        }

        //ecc check
        status = readl(REG_BCH_ECC_STATUS);
        if (unlikely(status & ECC_STATUS_UNCORRECTABLE_ERR))
        {
                /*
                 * This is a temporary erase check. A newly erased page read
                 * would result in an ecc error because the oob data is also
                 * erased to FF and the calculated ecc for an FF data is not
                 * FF..FF.
                 * This is a workaround to skip performing correction in case
                 * data is FF..FF
                 *
                 * Logic:
                 * For every page, each bit written as 0 is counted until these
                 * number of bits are greater than ecc level(8bits,24bits) (the maximum correction
                 * capability of ECC for each page + oob bytes)
                 */
                int bits_ecc = count_written_bits(read_ecc, chip->ecc.bytes, info->ecc_strength);
                int bits_data = count_written_bits(pBuff, chip->ecc.size, info->ecc_strength);
                if ((bits_ecc + bits_data) <= info->ecc_strength)
                {
                        if (bits_data){
                                memset(pBuff, 0xFF, chip->ecc.size);
                                mtd->ecc_stats.corrected += bits_ecc;
				pr_info("#erased page\n");
                        }
                        goto exit;
                }

                //err = -EIO;
                mtd->ecc_stats.failed++;
                printk("BCH: uncorrectable error,status 0x%x \n",status);
                goto exit;
        }

        bitnum = (status & ECC_STATUS_NUM_CORRECTIONS)>>20;
	if (bitnum != 0)
        {
		pr_info("\nCorrected ecc bit=%d\n",bitnum); 
                dwCorrection = REG_BCH_ECC_CORRECTION_0;
                pwBuff = (unsigned short*)pBuff;
                do
                {
                        correction = readl(dwCorrection);
                        /*
                         * A: in SX7 new nand controller,
                         * [27:16] is the error byte offset in the payload.
                         * payload = main + eccbytes
                         * 
                         * B: in SX7 old nand controller,
                         * [27:16] is the error halfword offset in the payload.
                         * payload = main
                         */ 
                        offset = (correction>>16)&0xFFF;
                        if ((correction == 0) || (offset >= chip->ecc.size) ) 
                        {
                                break;
                        }
			pr_info("offset 0x%x,raw 0x%02x XOR 0x%02x\n",offset,pBuff[offset],correction & 0xff);
                        if( likely(MONZA_NAND_NEW == info->nand_ctrler) )
				pBuff[offset] ^= (correction & 0xff);
			else if( MONZA_NAND_OLD == info->nand_ctrler)
				pwBuff[offset] ^= (correction & 0xffff);
				
                        dwCorrection += 4;
                /*
                 *bitflips occur,record the num of bitflips.
                 *register REG_BCH_ECC_CORRECTION_x[15:0] is the mask of the bit to correct.
                 *This is intended to be used as an XOR of the data read from nand device.
                 */
                 //mtd->ecc_stats.corrected += get_bitflip_count(correction & 0xffff);
                } while(1);
        }

exit:
    // clear MCL decode
    writel(0x108,MLC_ECC_REG_CONTROL);

    return err;
}
