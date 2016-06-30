#ifndef _NAND_BCH_ECC_H_
#define _NAND_BCH_ECC_H_

#include <linux/mtd/mtd.h>

void bch_encode_start(struct nand_chip *chip);
int bch_encode_end(unsigned int ecc[],struct nand_chip *chip);
void bch_decode_start(unsigned int ecc[],struct nand_chip *chip);
int bch_decode_end_correct(struct mtd_info *mtd,unsigned char* pBuff,struct nand_chip *chip);

#endif // _NAND_BCH_ECC_H_
