#include <fw_core.h>
#define VCHAIN_EX_MAGIC	(('1'<<24)|('a'<<16)|('h'<<8)|('c'))


struct vchain_ctx {
	struct list_head chains;
};

struct vchain {
	struct list_head list;
	char name[32];
	uint64_t parent;
	uint64_t child;
};

struct extension_op vchain_exop;

#define for_each_vchain(_vchain_, _ctx_)				\
	list_for_each_entry(_vchain_, &_ctx_->chains, list)

#define FW_CTX_TO_VCTX(fctx)	({					\
	struct vchain_ctx *_vctx_ = NULL;				\
	struct extension *_e_ = NULL;					\
	int __i = 0;							\
	for (__i=0; __i<fctx->nr_exts; __i++) {				\
		if (fctx->exts[__i].magic == VCHAIN_EX_MAGIC) {		\
			_e_ = &fctx->exts[__i];				\
			break;						\
		}							\
	}								\
	if (_e_)							\
		_vctx_ =(struct vchain_ctx *)_e_->ext_ctx;		\
									\
	_vctx_;								\
})

#define FW_CTX_TO_EXT(fctx)	({					\
	struct extension *_e_ = NULL;					\
	int __i = 0;							\
	for (__i=0; __i<fctx->nr_exts; __i++) {				\
		if (fctx->exts[__i].magic == VCHAIN_EX_MAGIC) {		\
			_e_ = &fctx->exts[__i];				\
			break;						\
		}							\
	}								\
	_e_;								\
})

static struct vchain_ctx * vchain_ctx_alloc(void)
{
	struct vchain_ctx *vctx = NULL;

	vctx = (struct vchain_ctx *)malloc(sizeof(struct vchain_ctx));
	if (!vctx)
		return NULL;

	memset((void *)vctx, 0, sizeof(struct vchain_ctx));
	INIT_LIST_HEAD(&vctx->chains);

	return vctx;
}

static int32_t vchain_add_chain(struct vchain_ctx *vctx, struct vchain *vch)
{
	struct vchain *vc = NULL;

	for_each_vchain(vc, vctx) {
		if (strcmp(vc->name, vch->name))
			return -EEXIST;
	}

	list_add_tail(&vch->list, &vctx->chains);
	return 0;
}

static struct vchain *get_vchain_by_name(struct vchain_ctx *vctx, const char *name)
{
	struct vchain *vc = NULL;

	for_each_vchain(vc, vctx) {
		if (strncmp(vc->name, name, strlen(vc->name)) == 0)
			return vc;
	}

	return NULL;
}

int32_t __create_vchain(struct fw_ctx *ctx, const char *name, uint32_t parent_id)
{
	int ret = 0;
	struct extension *ext = NULL;
	struct vchain *vc = NULL;

	if (ctx == NULL || name == NULL)
		return -EINVAL;

	ext = FW_CTX_TO_EXT(ctx);
	/* No message extension exist, need create this */
	if (ext == NULL) {
		ext = fw_register_extension(ctx, VCHAIN_EX_MAGIC);
		if (!ext)
			goto fail;
		/* Allocate bare context */
		ext->ext_ctx = (void *)vchain_ctx_alloc();
		if (ext->ext_ctx == NULL)
			goto fail;

	}
	vc = (struct vchain *)malloc(sizeof(struct vchain));
	if (vc == NULL) {
		goto fail;
	}
	memset((void *)vc, 0, sizeof(struct vchain));
	strncpy(&vc->name[0], name, (sizeof(vc->name)-1));
	local_set_bit(parent_id, (unsigned long *)&vc->parent);

	ret = vchain_add_chain((struct vchain_ctx *)ext->ext_ctx, vc);
	if (ret) {
		/* Must be the new chain already exist, free it and return failed */
		free(vc);
	}

	return ret;
fail:
	return -ENOMEM;
}

int32_t __vchain_bind_vol(struct fw_ctx *ctx, const char *name, int32_t vol_id)
{
	struct vchain *vc = NULL;
	struct vchain_ctx *vctx = NULL;

	if (ctx == NULL || name == NULL)
		return -EINVAL;

	vctx = FW_CTX_TO_VCTX(ctx);
	if (vctx == NULL)
		return -ENODEV;

	vc = get_vchain_by_name(vctx, name);
	if (vc == NULL)
		return -ENODEV;

	local_set_bit(vol_id, (unsigned long *)&vc->child);

	return 0;
}

int32_t __vchain_get_parent_id(struct fw_ctx *ctx, int32_t child)
{
	int32_t parent = -ENODEV;
	struct vchain_ctx *vctx = NULL;
	struct vchain *vc = NULL;

	vctx = FW_CTX_TO_VCTX(ctx);

	if (!vctx)
		return parent;

	for_each_vchain(vc, vctx) {
		if (local_test_bit(child, (unsigned long *)&vc->child) == 0)
			continue;

		parent = find_first_bit((unsigned long *)&vc->parent, sizeof(vc->parent));
		break;
	}

	return parent;

}

int32_t __vchain_del(struct fw_ctx *ctx, const char *name)
{
	struct vchain_ctx *vctx = NULL;
	struct vchain *vc = NULL;

	vctx = FW_CTX_TO_VCTX(ctx);
	if (vctx == NULL || name == NULL)
		return -EINVAL;

	vc = get_vchain_by_name(vctx, name);

	if (vc == NULL)
		return -EINVAL;

	list_del(&vc->list);

	free(vc);
	vc = NULL;

	return 0;
}

static void vchain_parse(struct fw_ctx *ctx, void *buff)
{
	struct extension *ext = NULL;
	struct vchain_ctx *vctx = NULL;
	struct vchain *vc = NULL;
	struct ext_hdr *hdr = (struct ext_hdr *)buff;

	vctx = vchain_ctx_alloc();
	/* Out of memory */
	if (!vctx) {
		return;
	}

	ext = FW_CTX_TO_EXT(ctx);
	if (ext == NULL) {
		free(vctx);
		return;
	}

	void *start = (void *)&hdr->data[0];
	void *pos = NULL;
	uint32_t cnt = 0;
	pos = start;

	while (cnt < hdr->len) {
		vc = (struct vchain *)malloc(sizeof(struct vchain));

		/* Out of memory */
		if (vc == NULL) {
			break;
		}

		memset((void *)vc, 0, sizeof(struct vchain));
		memcpy(&vc->name[0], pos, (sizeof(struct vchain)- sizeof(vc->list)));

		pos = (void *)((unsigned long)pos+
					(sizeof(struct vchain)- sizeof(vc->list)));

		cnt += (sizeof(struct vchain)- sizeof(vc->list));
		vchain_add_chain(vctx, vc);
	}

	ext->ext_ctx = (void *)vctx;
	return;
}

static int32_t vchain_fill(struct fw_ctx *ctx, void **buff)
{
	int len = 0;
	void *info = NULL, *pos = NULL, *start = NULL;
	struct ext_hdr *hdr = NULL;
	struct vchain_ctx *vctx = NULL;
	struct vchain *vc = NULL;

	vctx = FW_CTX_TO_VCTX(ctx);
	/*
	 * No context exist
	 * Tell fw core layer, no buff for this extension
	 */
	if (vctx == NULL) {
		*buff = NULL;
		return 0;
	}

	len += sizeof(struct ext_hdr);
	for_each_vchain(vc, vctx) {
		len += (sizeof(struct vchain)- sizeof(vc->list));
	}

	info = (void *)malloc(len);
	if (info == NULL) {
		*buff = NULL;
		return 0;
	}
	memset(info, 0, len);

	hdr = (struct ext_hdr *)info;
	hdr->magic = VCHAIN_EX_MAGIC;
	hdr->len = len - sizeof(struct ext_hdr);

	start = (void *)((unsigned long)info + sizeof(struct ext_hdr));
	pos = start;

	for_each_vchain(vc, vctx) {
		memcpy(pos, &vc->name[0], (sizeof(struct vchain)- sizeof(vc->list)));
		pos = (void *)((unsigned long)pos+
					(sizeof(struct vchain)- sizeof(vc->list)));

		if ((unsigned long)pos > ((unsigned long)start + len)) {
			printf("%s:ERROR: out of buff range, "
				"pos:%p, start:%p, len:%d\n", __func__, pos, start, len);
			break;
		}
	}

	*buff = info;

	return len;

}

static void vchain_release_chains(struct vchain_ctx *vctx)
{
	struct vchain *vc = NULL;
	while(!list_empty(&vctx->chains)) {
		vc = list_first_entry(&vctx->chains, struct vchain, list);
		list_del(&vc->list);
		free(vc);
		vc = NULL;
	}
	return;
}

static void vchain_ctx_release(struct vchain_ctx *vctx)
{
	if (vctx == NULL)
		return;
	vchain_release_chains(vctx);
	free(vctx);
	return;
}


static void vchain_release(struct fw_ctx *ctx, struct extension *in_ext)
{
	struct extension *ext = NULL;
	struct vchain_ctx *vctx = NULL;

	ext = FW_CTX_TO_EXT(ctx);
	vctx = FW_CTX_TO_VCTX(ctx);
	/* Nothing need to do */
	if (ext == NULL || vctx == NULL) {
		return;
	}

	vchain_ctx_release(vctx);
	vctx = NULL;
	ext->ext_ctx = NULL;
	return;
}

static struct vchain *vchain_dup(struct vchain *vc)
{
	struct vchain *ovc = (struct vchain *)malloc(sizeof(struct vchain));
	if (ovc == NULL) {
		goto fail;
	}
	memset((void *)ovc, 0, sizeof(struct vchain));
	strncpy(&ovc->name[0], &vc->name[0], (sizeof(ovc->name)-1));
	ovc->parent = vc->parent;
	ovc->child = vc->child;

	return ovc;
fail:
	return NULL;
}

static struct vchain_ctx *vchain_ctx_dup(struct vchain_ctx *vctx)
{
	struct vchain *vc = NULL, *ovc = NULL;
	struct vchain_ctx *ovctx = vchain_ctx_alloc();

	if (ovctx == NULL)
		return NULL;

	for_each_vchain(vc, vctx) {
		ovc = vchain_dup(vc);
		if (ovc == NULL)
			goto fail;
		if (vchain_add_chain(ovctx, ovc))
			goto fail;
	}

	return ovctx;
fail:
	vchain_ctx_release(ovctx);
	return NULL;
}

//TODO: how to call this? board don't have this ext or image don't support this
void __vchain_merge(struct fw_ctx *dst_ctx, struct fw_ctx *src_ctx)
{
	struct extension *dst_ext = NULL;
	struct vchain_ctx *dst_vctx = NULL, *src_vctx= NULL;

	src_vctx = FW_CTX_TO_VCTX(src_ctx);

	dst_vctx = FW_CTX_TO_VCTX(dst_ctx);
	dst_ext = FW_CTX_TO_EXT(dst_ctx);

	/* Release old chain definition */
	if (dst_vctx) {
		vchain_release(dst_ctx, NULL);
		dst_vctx = NULL;
	}

	/* if New context no chain extention, so do nothing, just return */
	if (src_vctx == NULL) {
		goto out;
	}

	/*Now, new context have volume chain, we need copy that one by one*/

	/* Check do we have such extension */
	if (dst_ext == NULL) {
		dst_ext = fw_register_extension(dst_ctx, VCHAIN_EX_MAGIC);
		if (!dst_ext) {
			fw_err("%s: Register VCHAIN extension failed, lost volume chain extension\n",__func__);
			goto out;
		}
	}
	dst_ext->ext_ctx = (void *)vchain_ctx_dup(src_vctx);
	if (dst_ext->ext_ctx == NULL)
		fw_err("%s:Dup VCHAIN context failed,lost volume chain extension\n",__func__);
out:
	return;
}

static void vchain_dump(struct fw_ctx *ctx)
{
	int i;
	struct extension *ext = NULL;
	struct vchain_ctx *vctx = NULL;
	struct vchain *vc = NULL;

	for (i=0; i<ctx->nr_exts; i++) {
		if (ctx->exts[i].magic == VCHAIN_EX_MAGIC) {
			ext = &ctx->exts[i];
			break;
		}
	}

	/* Nothing need to do */
	if (ext == NULL || ext->ext_ctx == NULL) {
		return;
	}

	printf("=== volume chain extension dump ===\n");
	/* Nothing need to do */
	if (ext == NULL || ext->ext_ctx == NULL) {
		return;
	}
	vctx = (struct vchain_ctx *)ext->ext_ctx;

	for_each_vchain(vc, vctx) {
		printf("\n\tname:\t%s\n", vc->name);
		printf("\tparent:\t%llx\n", (long long unsigned int)vc->parent);
		printf("\tchild:\t%llx\n", (long long unsigned int)vc->child);
	}
	printf("\n=== volume chain extension end ===\n");

	return;
}

struct extension_op vchain_exop = {
	.magic = VCHAIN_EX_MAGIC,
	.parse = vchain_parse,
	.fill = vchain_fill,
	.release = vchain_release,
	.dump = vchain_dump
};

