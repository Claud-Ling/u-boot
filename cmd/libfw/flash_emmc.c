//#define DEBUG
#include <fw_core.h>
#include <mmc.h>
#if !defined(__KERNEL__)
/*
 * Macro for define per read/write block size.
 */
#define RDWR_BUFF_LEN	(4096)

static int read_extcsd(int fd, __u8 *ext_csd)
{
	int ret = 0;
	struct mmc_ioc_cmd idata;
	memset(&idata, 0, sizeof(idata));
	memset(ext_csd, 0, sizeof(__u8) * 512);
	idata.write_flag = 0;
	idata.opcode = MMC_SEND_EXT_CSD;
	idata.arg = 0;
	idata.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	idata.blksz = 512;
	idata.blocks = 1;
	mmc_ioc_cmd_set_data(idata, ext_csd);

	ret = ioctl(fd, MMC_IOC_CMD, &idata);
	if (ret)
		perror("ioctl");

	return ret;
}

static int write_extcsd_value(int fd, __u8 index, __u8 value)
{
	int ret = 0;
	struct mmc_ioc_cmd idata;

	memset(&idata, 0, sizeof(idata));
	idata.write_flag = 1;
	idata.opcode = MMC_SWITCH;
	idata.arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
			(index << 16) |
			(value << 8) |
			EXT_CSD_CMD_SET_NORMAL;
	idata.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;

	ret = ioctl(fd, MMC_IOC_CMD, &idata);
	if (ret)
		perror("ioctl");

	return ret;
}

int get_boot_part_id(void)
{
	unsigned char ext_csd[512] = { 0 };
	int fd = -1, ret = -1;

	fd = open(EMMC_DEV, O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	ret = read_extcsd(fd, ext_csd);
	if (ret < 0) {
		close(fd);
		return -1;
	}
	close(fd);

	return (((ext_csd[EXT_CSD_PART_CONFIG]>>3) & 0x7) == \
					0x1 ? 0 : 1);
}

int set_boot_part_id(int bootid)
{
	unsigned char ext_csd[512] = { 0 };
	int fd = -1, ret = -1;
	unsigned char value = 0;

	fd = open(EMMC_DEV, O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	ret = read_extcsd(fd, ext_csd);
	if (ret < 0) {
		goto err1;
	}

	switch (bootid) {
	case 0:
		value |= (1 << 3);
		value &= ~(3 << 4);
		break;
	case 1:
		value |= (1 << 4);
		value &= ~(1 << 3);
		value &= ~(1 << 5);
		break;
	default:
		goto err1;
	}

	value |= EXT_CSD_PART_CONFIG_ACC_ACK;

	ret = write_extcsd_value(fd, EXT_CSD_PART_CONFIG, value);

	if (ret) {
		goto err1;
	}
	close(fd);
	return ret;
err1:
	close(fd);
	return -1;
}

static void bootpart_write_permision_en(void)
{
	char cmd[128] = { 0 };

	int bootid = get_boot_part_id();

	sprintf(cmd, "echo 0 > /sys/block/mmcblk0boot%d/force_ro", \
			(bootid == 0 ? 1:0));
	system(cmd);

	return;
}

static const char *get_boot_dev(void)
{
	int bootid = get_boot_part_id();

	if (bootid == 0) {
		if (!access("/dev/block/mmcblk0boot1", F_OK)) {
			return "/dev/block/mmcblk0boot1";
		} else {
			return "/dev/mmcblk0boot1";
		}
	} else {
		if (!access("/dev/block/mmcblk0boot0", F_OK)) {
			return "/dev/block/mmcblk0boot0";
		} else {
			return "/dev/mmcblk0boot0";
		}
	}

	return NULL;
}

static void switch_boot_part(void)
{
	int bootid = get_boot_part_id();

	set_boot_part_id((bootid == 0 ? 1:0));

	return;
}
#endif /* !__KERNEL__ */


#if defined(__UBOOT__)
static struct mmc *init_mmc_device(int dev, bool force_init)
{
	struct mmc *mmc;
	mmc = find_mmc_device(dev);
	if (!mmc) {
		printf("no mmc device at slot %x\n", dev);
		return NULL;
	}
	if (force_init)
		mmc->has_init = 0;
	if (mmc_init(mmc))
		return NULL;
	return mmc;
}

uint64_t dev_to_offs(const char *dev)
{
	struct fw_vol *vol = NULL;
	struct fw_ctx *ctx = NULL;
	struct fw_part *part = NULL;
	struct list_head *pos = NULL;
	char *p = NULL;
	uint32_t part_idx = 0, cnt = 0;
	if (strlen(dev) == strlen(EMMC_DEV) &&
				!strcmp(dev, EMMC_DEV)) {

		return 0;
	} else if (strlen(dev) > strlen(EMMC_DEV) &&
				!strncmp(dev, EMMC_DEV, strlen(EMMC_DEV))) {

		p = strrchr(dev, 'p');
		if (!p)
			return 0;
		p++;
		part_idx = simple_strtoul(p, NULL, 10);
		part_idx--;
	} else {
		return 0;
	}

	ctx = fw_get_board_ctx();

	if (!ctx)
		return 0;



	list_for_each(pos, &ctx->vols) {
		vol = list_entry(pos, struct fw_vol, list);

		cnt += vol->info->nr_parts;
		if (part_idx < cnt) {
		/*We got volume */
			break;
		}
	}

	/* Calc total parts before this vol */
	cnt -= vol->info->nr_parts;

	/*Calc part offset in this vol */
	part_idx = part_idx - cnt;

	part = fw_vol_get_part_by_id(vol, part_idx);
	if (!part) {
		fw_debug("%s: can't find part info for dev:%s, part_idx:%u cnt:%u\n", __func__, dev, part_idx, cnt);
		return 0;
	}

	return part->info->start;

}
#endif /* __UBOOT__ */

int32_t fop_emmc_open(struct flash_op *op, const char *dev)
{

#if !defined(__KERNEL__)
	fw_debug("In %s\n", __func__);
	op->fd = open(dev, O_RDWR);

	if (op->fd < 0) {
		return -ENODEV;
	}

	return 0;
#elif defined(__UBOOT__)
	/* u-boot case */
	op->part_base = dev_to_offs(dev);
	op->cur = op->part_base;
	op->mmc = init_mmc_device(0, false);
	return 0;

#else
	/*Pure kernel case */

#endif
}

void fop_emmc_close(struct flash_op *op)
{
#if !defined(__KERNEL__)
	fw_debug("In %s\n", __func__);
	close(op->fd);
	op->fd = 0;
	return;
#elif defined(__UBOOT__)
	/* u-boot case */
	return;
#else
	/*Pure kernel case */
#endif
}

int64_t fop_emmc_read(struct flash_op *op, void *buff, uint64_t sz)
{
	fw_debug("In %s, buff@%p, sz:%llx\n", __func__, buff, sz);
#if !defined(__KERNEL__)
	return read(op->fd, buff, sz);
#elif defined(__UBOOT__)
	/* u-boot case */
	char *remainder_buff[512] =  { 0 };
	void *pos = NULL;
	struct mmc *mmc = op->mmc;
	u32 blk = (op->cur / 512);
	u32 remainder_sz = sz % 512;
	u32 cnt = (sz / 512);
	u32 n = 0;

	fw_debug("%s: op->cur:%lld, cnt:%lld\n", __func__, op->cur, sz/512);
	n = mmc->block_dev.block_read(&mmc->block_dev, blk, cnt, buff);
	if (n != cnt)
		goto error;

	flush_cache((ulong)buff, cnt * 512);

	if (remainder_sz) {
		pos = (void *)((unsigned long)buff + cnt * 512);
		n = mmc->block_dev.block_read(&mmc->block_dev, blk+cnt, 1, (void *)remainder_buff);
		cnt +=1;
		if (n != 1) {
			goto error;
		}
		flush_cache((ulong)remainder_buff, n * 512);
		memcpy(pos, (void *)remainder_buff, remainder_sz);
	}

	op->cur += cnt*512;

	fw_debug("%s: %d blocks read: OK\n", __func__, cnt);
	return sz;

error:
	printf("%s: %d blocks read: ERROR\n", __func__, cnt);
	return -1;
#else
	/*Pure kernel case */
#endif
}

int64_t fop_emmc_write(struct flash_op *op, void *buff, uint64_t sz)
{
	fw_debug("In %s\n", __func__);
#if !defined(__KERNEL__)
	return write(op->fd, buff, sz);
#elif defined(__UBOOT__)
	/* u-boot case */
	struct mmc *mmc = op->mmc;
	u32 blk = (op->cur / 512);
	u32 cnt = (sz / 512);
	//TODO: Write size not 512byte align, any good idea?
	if (sz & (512-1)) {
		cnt += 1;
	}
	fw_debug("%s: op->cur:%lld, cnt:%lld\n", __func__, op->cur, sz/512);
	u32 n = 0;

	n = mmc->block_dev.block_write(&mmc->block_dev, blk, cnt, buff);

	op->cur += n*512;

	fw_debug("%s: %d blocks written: %s\n", __func__, n, (n == cnt) ? "OK" : "ERROR");
	return sz;
#else
	/*Pure kernel case */
#endif
}

int64_t fop_emmc_seek(struct flash_op *op, uint64_t offs, int whence)
{
	fw_debug("In %s\n", __func__);
#if !defined(__KERNEL__)
	return lseek(op->fd, offs, whence);
#elif defined(__UBOOT__)
	/* u-boot case */
	op->cur = (op->part_base + offs);
	return offs;
#else
	/*Pure kernel case */
#endif
}

int32_t fop_emmc_update_bootloader(struct flash_op *op, const char *image)
{
#if !defined(__KERNEL__)
	int fd = -1, rd_sz, wr_sz, ret = -1, len = RDWR_BUFF_LEN;
	const char *dev = NULL;
	char buff[RDWR_BUFF_LEN] = { 0 };

	fd = open(image , O_RDONLY);
	if(fd < 0) {
		goto failed;
	}

	bootpart_write_permision_en();
	dev = get_boot_dev();
	ret = op->open(op, dev);
	if (ret < 0) {
		goto failed1;
	}

	do {
		rd_sz = read(fd, buff, len);
		if (rd_sz < 0)
			break;

		wr_sz = op->write(op, buff, rd_sz);

	} while ((rd_sz > 0) && (wr_sz > 0));

	if (rd_sz<0 || wr_sz<0) {
		goto failed2;
	}

	close(fd);
	op->close(op);

	switch_boot_part();

	return 0;

failed2:
	op->close(op);
failed1:
	close(fd);
failed:
	return -ENODEV;

#else
	return 0;
#endif

}
struct flash_op emmc_op = {
	.flash_type = FW_FTYP_EMMC,
	.fd = 0,
	.open = fop_emmc_open,
	.update_bootloader = fop_emmc_update_bootloader,
	.close = fop_emmc_close,
	.read = fop_emmc_read,
	.write = fop_emmc_write,
	.seek = fop_emmc_seek,
};

