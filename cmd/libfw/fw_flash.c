#include <fw_core.h>

extern struct flash_op emmc_op;

struct flash_op *flash_ops[] = {
	&emmc_op,
};

struct flash_op *flash_op_open(const char *dev, uint32_t flash_typ)
{
	int i, ret;
	struct flash_op *op = NULL;

	fw_debug("In %s, dev:%s, flash_typ:%d\n", __func__, dev, flash_typ);
	for (i=0; i<ARRAY_SIZE(flash_ops); i++) {
		fw_debug("%s:flash_ops[%d]->flash_type:%d\n",
				 __func__, i, flash_ops[i]->flash_type);

		if (flash_ops[i]->flash_type== flash_typ) {
			op = flash_ops[i];
			break;
		}
	}

	if (op) {
		ret = op->open(op, dev);
		if (ret < 0)
			return NULL;
	}
	return op;
}

int32_t flash_op_update_bootloader(const char *image, uint32_t flash_typ)
{
	int i, ret;
	struct flash_op *op = NULL;

	fw_debug("In %s, flash_typ:%d\n", __func__, flash_typ);
	for (i=0; i<ARRAY_SIZE(flash_ops); i++) {
		fw_debug("%s:flash_ops[%d]->flash_type:%d\n",
				 __func__, i, flash_ops[i]->flash_type);

		if (flash_ops[i]->flash_type== flash_typ) {
			op = flash_ops[i];
			break;
		}
	}

	if (op) {
		ret = op->update_bootloader(op, image);
		if (ret < 0)
			return -ENODEV;
		return 0;
	}

	return -ENODEV;
}

void flash_op_close(struct flash_op *op)
{
	fw_debug("In %s\n", __func__);
	op->close(op);
	return;
}

int64_t flash_op_read(struct flash_op *op, void *buff, uint64_t sz)
{
	fw_debug("In %s\n", __func__);
	return op->read(op, buff, sz);
}

int64_t flash_op_write(struct flash_op *op, void *buff, uint64_t sz)
{
	fw_debug("In %s\n", __func__);
	return op->write(op, buff, sz);
}

int64_t flash_op_seek(struct flash_op *op, uint64_t offs, int32_t whence)
{
	fw_debug("In %s\n", __func__);
	return op->seek(op, offs, whence);
}

