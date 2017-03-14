#include <command.h>
#include <common.h>
#include <asm/io.h>
#include <asm/arch/setup.h>
#include "./inc/rsa.h"
#include "./inc/sha2.h"
#include "./inc/bignum.h"
#include "./inc/bn_mul.h"


//#define SIG_DEBUG 1
#ifdef SIG_DEBUG
#define PDEBUG(fmt, args...) printf(fmt, ## args)
#else
#define PDEBUG(fmt, args...)
#endif
#define PERROR(fmt, args...) printf(fmt, ## args)

#define HASHLEN  32     //in bytes
#define SIGNLEN  256    //signature len in bytes
#define KEY_SIG_BUF_LEN 0x600 //signuate and key buffer length
#define KEY_LEN  256

static void print_mem(char *desc, unsigned char *buf, int len)
{
#ifdef SIG_DEBUG
    int i;
    int j = 0;
    PDEBUG("start print %s\n", desc);
    for (i = 0; i < len;  i ++)
    {
        PDEBUG("%02x ", buf[i]);
        j ++;
        if (j == 16)
        {
            j = 0;
            PDEBUG("\n");
        }
    }
    PDEBUG("end print %s\n", desc);
#endif
}

/*PUBKEY for Vizio P111*/
static unsigned char vizio_product_key[256] = {0x94,0x0E,0x5A,0xCF,0x18,0x3B,0xF4,0xB5,0x10,0x7D,0xC6,0x53,0x11,0x46,0x88,0x98,0xD0,0x30,0xBF,0xF8,0x98,0xE6,0xB1,0x7E,0xE5,0xFC,0x7A,0x4E,0xC3,0x57,0x56,0xFD,0x6B,0x6F,0x4E,0x21,0xF1,0x8D,0x47,0xCB,0x7D,0x43,0xAB,0x92,0x29,0x61,0xBE,0x3F,0x8D,0xFD,0x23,0xEE,0x52,0x96,0x46,0x51,0x33,0xA3,0x39,0x99,0xF4,0x2E,0x36,0xF6,0x96,0x35,0x4D,0x37,0x39,0x9A,0x0A,0xD8,0xA1,0xAD,0xE8,0xD1,0x66,0xDD,0x19,0xEA,0x93,0xB2,0xF8,0x1E,0x66,0xF0,0xE4,0x9A,0x84,0xB3,0x35,0x00,0x41,0xF5,0xD7,0xAC,0xC5,0xEF,0xD4,0x12,0xBB,0x0F,0xB2,0x36,0xD7,0x69,0x0C,0x35,0x94,0xBE,0xD6,0x91,0xAE,0x9E,0x3D,0x8E,0x5B,0xBE,0x32,0x4F,0xC2,0xF9,0x96,0x25,0x0F,0x4C,0x9A,0xDA,0x1F,0x07,0x84,0x9B,0x99,0x0B,0x9C,0xD9,0x44,0xEE,0x82,0xED,0x97,0xD4,0xD0,0x99,0x10,0x66,0x6B,0x61,0xA4,0x54,0x0B,0x04,0xD9,0x5E,0xD0,0x53,0x70,0x89,0x3E,0x40,0x7C,0x5D,0x97,0x3F,0xC0,0xC2,0x35,0x69,0xCC,0x14,0x67,0x3A,0xB6,0x1E,0x8A,0x94,0xFE,0xA5,0xCF,0x41,0xF4,0xC0,0xD8,0x53,0xC6,0xB8,0x00,0x06,0x15,0xC1,0x57,0xAB,0x2F,0x8D,0xAC,0x55,0xF9,0x0E,0xE3,0x24,0xC5,0xF3,0x30,0xC6,0xDB,0x91,0xB2,0x3A,0x3C,0xA8,0x3C,0x17,0x81,0x20,0x23,0x07,0x82,0x34,0x4D,0xA9,0x99,0xB2,0x61,0x75,0x5C,0x29,0xF6,0xA7,0x67,0x30,0x7F,0x60,0x3D,0x4E,0x14,0x46,0x5A,0xE2,0xDB,0xD8,0xD0,0xF8,0xFF,0xB2,0x62,0x58,0x5F,0xFD,0x57,0xE3,0xA8,0x45,0x89,0x44,0x2D,0x63};

enum {
	PREBOOT_KEY=1,
	UBOOT_KEY,
	KERNEL_KEY,
	APP_KEY,
	INVALID_KEY
};

#define PUBLIC_KEY_NBYTES	0x100

#ifndef CONFIG_ARM64

#define PUBLIC_KEY_DDR_ENTRY    0xA000
#define PUBLIC_KEY_PREBOOT_OFS	0x0
#define PUBLIC_KEY_UBOOT_OFS	0x100
#define PUBLIC_KEY_KERNEL_OFS	0x200
#define PUBLIC_KEY_APP_OFS	0x300

static int inline get_public_key_offset(int keytype)
{
	int offset = 0;
	switch (keytype) {
	case PREBOOT_KEY:
		offset = PUBLIC_KEY_PREBOOT_OFS;
		break;
	case UBOOT_KEY:
		offset = PUBLIC_KEY_UBOOT_OFS;
		break;
	case KERNEL_KEY:
		offset = PUBLIC_KEY_KERNEL_OFS;
		break;
	case APP_KEY:
		offset = PUBLIC_KEY_APP_OFS;
		break;
	default:
		printf("Can not recognize key type!!\n");
	}
	return offset;
}

static int otp_get_rsa_pub_key(void *key, int len)
{
	assert(len >= PUBLIC_KEY_NBYTES);
	memcpy(key,
		(void*)(PUBLIC_KEY_DDR_ENTRY + get_public_key_offset(PREBOOT_KEY)),
		PUBLIC_KEY_NBYTES);
	return 0;
}
#else /*CONFIG_ARM64*/
static int otp_get_rsa_pub_key(void *key, int len)
{
	printf ("TODO: get rsa pub key from otp\n");
	return -1;
}
#endif /*CONFIG_ARM64*/

/*
 * Judge the bootloader is signed by vizio key or not
 * purpose: disable console by running a vizio signed image.
 *
 * return : 1-bootloader signed by vizio,   0 - others
 */
static int is_vizio_signed_img(void)
{
	unsigned char key[PUBLIC_KEY_NBYTES];
	int ret = 0;

	ret = otp_get_rsa_pub_key(key, sizeof(key));
	if (ret != 0) {
		//printf("failed to get rsa pub key from otp, ret=%d\n", ret);
		return ret;
	}

	/*Vizio should inform us if they have new key...*/
	if(!memcmp(key, vizio_product_key, PUBLIC_KEY_NBYTES))
	{
		printf("valid Vizio signed image\n");
		ret = 1;
	}

	return ret;
}

/*
 *return : 1-bootloader Vizio products,   0 - others
 */
int is_vizio_products(void)
{
	char *s = getenv("security_check");
	if (!s)
		return is_vizio_signed_img();
	else
		return !!('y' == *s);
}

int is_secure_enable(void)
{
	bool security = false;
	char *s = NULL;

	security = otp_get_security_boot_state();
	PERROR("Secure boot is %s\n", (security == true) ? "true" : "false");

	/*
	 * we check the uboot env "security_check" for debug mode:
	 * 1.if security_check=y,authenticating is force enable
	 * 2.if security_check=n,authenticating is force disable
	 * 3.depending on OTP register and Signed Key if security_check is NULL.
	 */
	s = getenv("security_check");
	if(!s)
		return (security == true) ? 1 : 0;
	else
		return ('y'==*s)?1:0;
}

int authenticate_image(void *data, int len, void *key, void *sig)
{
	int err = -1;
	sha2_context ctx;
	rsa_context rsa;
	unsigned char sha_result[HASHLEN];

	if(!is_secure_enable())
	{
		PERROR( "secure is not enabled\n");
		return 0;
	}

	/*
	 *	do hash
	 */
	PERROR( "do hash at 0x%x,imagesize=0x%x Bytes\n",(int)data,len);
	sha2_starts(&ctx, 0);
	sha2_update(&ctx,data,len);
	sha2_finish(&ctx,sha_result);

	/*
	 *	RSA verify
	 */
	rsa_init( &rsa, RSA_PKCS_V15, 0, NULL, NULL );
	mpi_read_binary(&rsa.N, key, KEY_LEN);
	mpi_read_string(&rsa.E, 16,"010001"/*E_buf*/);

	print_mem("-------- public key -------", key, KEY_LEN);
	print_mem("-------- signature -------", sig, SIGNLEN);
	print_mem("-------- hash result ------", sha_result, HASHLEN);

	rsa.len = ( mpi_msb( &rsa.N ) + 7 ) >> 3;
	if( ( rsa_pkcs1_verify( &rsa, RSA_PUBLIC, RSA_SHA256,
                                HASHLEN, sha_result, sig) ) != 0 )
	{
		PERROR( "rsa_pkcs1_verify failed \n");
		rsa_free( &rsa );
		err = 3;
		goto FAIL;
	}

	PERROR( "rsa_pkcs1_verify success\n");
	rsa_free(&rsa);

	return 0;

FAIL:
	return err;
}

int security_check(unsigned char *image_addr, unsigned int length)
{
	int ret;
	unsigned char *sig;
	unsigned char *key;

	if(!is_secure_enable())
	{
		PERROR( "secure is not enabled\n");
		return 0;
	}

	sig = image_addr + length;
	key = (unsigned char*)(PUBLIC_KEY_DDR_ENTRY + PUBLIC_KEY_KERNEL_OFS);

	ret = authenticate_image(image_addr, length, key, sig);
	if (ret != 0)
	{
		printf("\r\n##########Kernel RSA verify fail!############\r\n");
		while(1);	/*Never return......*/
	}

	return 0;
}

int signature_check(void *data, int len, int key_type, void *signature)
{
	unsigned char *key;

	key = (unsigned char *)(PUBLIC_KEY_DDR_ENTRY + get_public_key_offset(key_type));
	return authenticate_image(data, len, key, signature);
}

