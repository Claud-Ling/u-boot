/*
 * bootparam.h
 * tony.he@sigmadesigns.com, April 23th 2014
 * This file defines bootparam structure
 */
#ifndef __BOOTPARAM_H__
#define __BOOTPARAM_H__

#define BOOTPARAM_POSITION	0xfffe0000 /*bootparam position*/
#define BOOTPARAM_FRM_SIZE 	32	/* sizeof struct bootparam_frame */

#define BOOTPARAM_TAIL_OFS	(BOOTPARAM_FRM_SIZE - 4)

#define BOOTPARAM_FLAGS_OFS	16
#define BOOTPARAM_REASON_OFS	12
#define BOOTPARAM_DEV_OFS	8
#define BOOTPARAM_MSG_OFS	4
#define BOOTPARAM_HEAD_OFS	0

/*tags*/
#define BOOTPARAM_HEAD_TAG	0xbeefc0de
#define BOOTPARAM_TAIL_TAG	0xaaaa5555

/*flags (total 32 bits)*/
#define FLAGS_SECURITY		(0x1 << 0)	/*security boot*/
#define FLAGS_MCU		(0x1 << 1)	/*boot mcu*/
#define FLAGS_CFG		(0x1 << 2)	/*board configure*/
#define FLAGS_SSC		(0x1 << 3)	/*with ssc patch*/
#define FLAGS_SMC		(0x1 << 4)	/*with smc*/
#define FLAGS_S2RAM		(0x1 << 5)	/*with s2ram*/

/*reset reasons*/
#define REASON_UNKNOWN		0
#define REASON_AC_OFF		1
#define REASON_DC_OFF		2

/*boot devices*/
#define DEV_UNKNOWN		0
#define DEV_SPI			1
#define DEV_NAND		2
#define DEV_EMMC		3

#ifndef __ASSEMBLY__
/*
 * bootparam frame structure
 * This structure defines boot param for uboot
 */
struct bootparam_frame{
	long data[(BOOTPARAM_FRM_SIZE) >> 2];
};

#define BOOTPARAM_TAIL	data[BOOTPARAM_TAIL_OFS >> 2]

#define BOOTPARAM_FLAGS	data[BOOTPARAM_FLAGS_OFS >> 2]
#define BOOTPARAM_REASON data[BOOTPARAM_REASON_OFS >> 2]
#define BOOTPARAM_DEV	data[BOOTPARAM_DEV_OFS >> 2]
#define BOOTPARAM_MSG	data[BOOTPARAM_MSG_OFS >> 2]
#define BOOTPARAM_HEAD	data[BOOTPARAM_HEAD_OFS >> 2]

extern struct bootparam_frame boot_params;
char* sx6_boot_message(void);
long sx6_boot_device(void);
long sx6_boot_reason(void);
long sx6_boot_flags(void);
#endif //__ASSEMBLY__

#endif
