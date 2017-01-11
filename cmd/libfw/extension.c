#include <fw_core.h>

extern struct extension_op msg_exop;
extern struct extension_op vchain_exop;
//static struct extension_op default_exop;
struct extension_op *extop_list[] = {
	&msg_exop,
	&vchain_exop,
	//&default_exop,
};


static void default_parse(struct fw_ctx *ctx, void *buff)
{
	int i = 0, len;
	void *info = NULL;
	struct ext_hdr *hdr = (struct ext_hdr *)buff;

	for (i=0; i<ctx->nr_exts; i++) {
		if (ctx->exts[i].magic == hdr->magic)
			break;
	}

	len = sizeof(struct ext_hdr) + hdr->len;
	info = malloc(len);
	memset(info, 0, len);

	memcpy(info, buff, len);

	ctx->exts[i].ext_ctx = info;
	ctx->exts[i].info_data = info;
	return;
}

static int32_t default_fill(struct fw_ctx *ctx, void **buff)
{
	int len;
	void *info = *buff;
	struct ext_hdr *hdr = (struct ext_hdr *)info;
	if (info == NULL) {
		printf("%s:Warnning: unknow extensions call default extension ops!\n", __func__);
		return 0;
	}
	
	len = sizeof(struct ext_hdr) + hdr->len;

	return len;
	
}

static void default_release(struct fw_ctx *ctx, struct extension *in_ext)
{
	if (in_ext == NULL || in_ext->ext_ctx == NULL) {
		return;
	}

	free(in_ext->ext_ctx);
	in_ext->ext_ctx = NULL;

	return;
}

static void default_dump(struct fw_ctx *ctx)
{
	return;
}

static struct extension_op default_exop = {
	.magic = ('t'<<24)|('f'<<16)|('e'<<8)|('d'),
	.parse = default_parse,
	.fill = default_fill,
	.release = default_release,
	.dump = default_dump
};

struct extension_op *find_ext_op(uint32_t magic)
{
	int i;
	for (i=0; i<ARRAY_SIZE(extop_list); i++) {

		if (extop_list[i]->magic == magic) {
			return extop_list[i];
		}
	}

	return &default_exop;
}
