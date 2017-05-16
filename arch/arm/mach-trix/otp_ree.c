#include <common.h>
#include <errno.h>
#include <linux/compat.h>
#include <asm/arch/setup.h>

/******************************************************************/
/*                     Fuse Data Map Start                        */
/******************************************************************/
#define FUSE_OFS_FC_0		0x004	/*Fuction Control 0*/
#define FUSE_OFS_FC_1		0x008	/*Fuction Control 1*/
#define FUSE_OFS_FC_2		0x00c	/*Fuction Control 2
					 * bit [0]	SEC_BOOT_FROM_ROM
					 * bit [1]	SEC_BOOT_EN
					 * bit [6:2]	BOOT_VAL_USER_ID
					 * bit [12]	NEW_NAND_CTRL_SEL
					 */
#define FUSE_FC2_BOOT_ROM	BIT(0)
#define FUSE_FC2_SEC_BOOT_EN	BIT(1)
#define FUSE_FC2_USER_ID_SHIFT	2
#define FUSE_FC2_USER_ID_MASK	(0x1f << FUSE_FC2_USER_ID_SHIFT)
#define FUSE_FC2_NEW_NAND_SEL	BIT(12)

#define FUSE_OFS_FC_3		0x010	/*Fuction Control 3
					 * bit [0]	SEC_DIS
					 * bit [1]	BOOT_FROM_ROM_DIS
					 */
#define FUSE_OFS_DIE_ID_0	0x038	/*DIE_ID_0
					 * bit [7:0]	DESIGN_ID (0x00-A0; 0x01-A1; 0x02-A2; 0x03-A3; 0x04-A4;...0x05-A5
					 */

#define FUSE_OFS_RSA_PUB_KEY	CONFIG_FUSE_OFS_RSA_PUB_KEY

/******************************************************************/
/*                     Fuse Data Map End                          */
/******************************************************************/

/*
 * get security boot state from OTP
 * return value
 * 	false	-  security boot disabled (default)
 * 	true	-  security boot enabled
 */
bool otp_get_security_boot_state(void)
{
	uint32_t val;
	BUG_ON(secure_otp_get_fuse_mirror(FUSE_OFS_FC_2, &val, NULL) != 0);
	return (val & FUSE_FC2_SEC_BOOT_EN) ? true : false;
}

/*
 * get new NAND selection state from OTP
 * return value
 * 	false	-  legacy NAND controller (default)
 * 	true	-  new NAND controller
 */
bool otp_get_new_nand_sel_state(void)
{
	uint32_t val;
	BUG_ON(secure_otp_get_fuse_mirror(FUSE_OFS_FC_2, &val, NULL) != 0);
	return (val & FUSE_FC2_NEW_NAND_SEL) ? true : false;
}

/*
 * get index value of current using RSA public key
 * return value
 * 	0       -  use OTP RSA public key
 * 	1 ~ 16  -  index to ROM embedded RSA public key that is in use
 * 	<0	-  error
 */
int otp_get_rsa_key_index(void)
{
	uint32_t val;
	BUG_ON(secure_otp_get_fuse_mirror(FUSE_OFS_FC_2, &val, NULL) != 0);
	return (int)((val && FUSE_FC2_USER_ID_MASK) >> FUSE_FC2_USER_ID_SHIFT);
}

/*
 * read RSA public key from OTP
 * inputs:
 * 	buf     -  point to a buffer
 * 	nbytes  -  length of buffer, it's at least OTP_RSA_KEY_NBYTES long
 * return value:
 * 	0 on success. Otherwise non-zero
 */
uint32_t otp_get_rsa_key(uint32_t *buf, uint32_t nbytes)
{
	return secure_otp_get_fuse_array(FUSE_OFS_RSA_PUB_KEY, buf, &nbytes, NULL);
}

/*
 * get value of bits [<s>+<nbits>-1:<s>] of fuse <ofs>
 * inputs:
 * 	ofs     -  specify fuse offset
 * 	s	-  start bit (begin at 0)
 * 	nb	-  number of bits to get against
 * outputs:
 * 	ptr	-  pointer of buffer to load fuse value on success
 * return value:
 * 	0 on success with value of specified bits stored in buffer
 * 	pointed by <ptr>. Otherwise errors.
 */
#ifdef GET_BITS
# undef GET_BITS
#endif
#define GET_BITS(v, s, n) (((v) & (((1 << (n)) - 1) << (s))) >> (s))
int otp_get_fuse_bits(const uint32_t ofs, const unsigned s, const unsigned nbits, uint32_t *ptr)
{
	int ret;
	uint32_t tmp;

	if (s > 31 || nbits > 31 || ptr == NULL) {
		error("invalid parameter! s %d nb %d\n", s, nbits);
		return -EINVAL;
	}

	if ((ret = secure_otp_get_fuse_mirror(ofs, &tmp, NULL)) == 0){
		debug("get fuse value %#x (ofs %#x)\n", tmp, ofs);
		*ptr = GET_BITS(tmp, s, nbits);
	} else {
		error("failed to get fuse value at ofs %#x\n", ofs);
	}
	return ret;
}
