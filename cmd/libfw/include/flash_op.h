#ifndef __FLASH_OP_H__
#define __FLASH_OP_H__
#if !defined(__KERNEL__)
# define  __user        /* nothing */
# include <mtd/mtd-user.h>
#endif

#define NAND_DEV	"/dev/mtd0"
#define EMMC_DEV	"/dev/mmcblk0"
#define OTP_STR		"BOOT_FROM_ROM: "
enum {
	FW_FTYP_EMMC = 0,
	FW_FTYP_NAND = 1,
	FW_FTYP_SPI = 2,
	FW_FTYP_UNKNOWN = 3,
};

enum {
	FOP_RGN_MAIN = 0,	/*Main storage region*/
	FOP_RGN_BOOT = 1,	/*Bootloader storage region */
};

struct flash_op {
	uint32_t flash_type;
	uint32_t region;
	int fd;
#ifdef __UBOOT__
	uint64_t part_base;
	uint64_t cur;
	struct mmc *mmc;
	void *priv;
#endif
	int32_t(*open)(struct flash_op *, const char *);
	int32_t(*update_bootloader)(struct flash_op *, const char *);
	void (*close)(struct flash_op *);
	int64_t (*read)(struct flash_op *, void *, uint64_t);
	int64_t (*write)(struct flash_op *, void *, uint64_t);
	int64_t (*seek)(struct flash_op *, uint64_t, int32_t);
};


extern struct flash_op *flash_op_open(const char *dev, uint32_t flash_typ);
extern void flash_op_close(struct flash_op *op);
extern int64_t flash_op_read(struct flash_op *op, void *buff, uint64_t sz);
extern int64_t flash_op_write(struct flash_op *op, void *buff, uint64_t sz);
extern int64_t flash_op_seek(struct flash_op *op, uint64_t offs, int32_t whence);
extern int32_t flash_op_update_bootloader(const char *image, uint32_t flash_typ);

#endif /* __FLASH_OP_H__ */
