#include <fw_core.h>

#define MSG_EX_MAGIC	(('1'<<24)|('g'<<16)|('s'<<8)|('m'))
struct msg_ctx {
	struct list_head msgs;
};

struct msg_bdy {
	struct list_head list;
	uint32_t bdy_len;
	uint32_t name_len;
	void *name_buff;
	uint32_t data_len;
	void *data_buff;
};

struct extension_op msg_exop;


#define for_each_msg(_msg_, _ctx_)			\
	list_for_each_entry(_msg_, &_ctx_->msgs, list)

static void msgex_free_content(struct msg_bdy *msg)
{
	if (msg->name_buff) {
		free(msg->name_buff);
		msg->name_buff = NULL;
	}

	if (msg->data_buff) {
		free(msg->data_buff);
		msg->data_buff = NULL;
	}
	return;
}

static struct msg_ctx *msgex_ctx_alloc(void)
{
	struct msg_ctx *ctx =(struct msg_ctx*)malloc(sizeof(struct msg_ctx));
	memset((void*)ctx, 0, sizeof(struct msg_ctx));
	INIT_LIST_HEAD(&ctx->msgs);

	return ctx;
}

static void msgex_ctx_release(struct msg_ctx *mctx)
{
	struct msg_bdy *msg = NULL;
	if (mctx == NULL)
		return;

	while(!list_empty(&mctx->msgs)) {

		msg = list_first_entry(&mctx->msgs, struct msg_bdy, list);
		msgex_free_content(msg);
		list_del(&msg->list);
		free(msg);
		msg = NULL;
	}

	free(mctx);

	return;
}

static struct msg_bdy *msgex_search_msg_by_name(struct msg_ctx *mctx, const char *name)
{
	struct msg_bdy *msg = NULL;
	uint32_t n_len = strlen(name);

	for_each_msg(msg, mctx) {
		if (strlen((char *)msg->name_buff) != n_len)
			continue;

		if(!strncmp(name, (char *)msg->name_buff, n_len)) {
			return msg;
		}
	}

	return NULL;
}

static struct msg_bdy *msgex_alloc_msg(void)
{
	struct msg_bdy *msg = malloc(sizeof(struct msg_bdy));
	memset((void *)msg, 0, sizeof(struct msg_bdy));
	return msg;
}

static void msgex_free_msg(struct msg_bdy *msg)
{
	if (msg->name_buff) {
		free(msg->name_buff);
		msg->name_buff = NULL;
	}

	if (msg->data_buff) {
		free(msg->data_buff);
		msg->data_buff = NULL;
	}
	free(msg);
	return;
}

static int32_t msgex_add_msg(struct msg_ctx *mctx, struct msg_bdy *msg)
{
	list_add_tail(&msg->list, &mctx->msgs);
	return 0;
}

static int32_t msgex_del_msg(struct msg_ctx *mctx, struct msg_bdy *msg)
{
	list_del(&msg->list);
	return 0;
}

static int32_t msgex_set(struct msg_ctx *mctx, const char *name, const char *value)
{
	struct msg_bdy *msg = NULL;
	uint32_t n_len = 0, v_len = 0;

	if (name == NULL || mctx == NULL) {
		return -EINVAL;
	}

	msg = msgex_search_msg_by_name(mctx, name);
	if (msg)
		goto set_value;

	/* No such message exist, create a new one */
	msg = msgex_alloc_msg();
	if (!msg)
		return -ENOMEM;

	msgex_add_msg(mctx, msg);

#define ALIGNED_4(_ptr_)	({	\
	if ((_ptr_) % 4) {		\
		(_ptr_) += 4;		\
		(_ptr_) &= ~(4-1);	\
	}				\
	_ptr_;				\
})

	/* Setup msg name, due to we are new message */
	n_len = strlen(name) + 1;
	n_len = ALIGNED_4(n_len);

	msg->name_len = n_len;
	msg->name_buff = (void *)malloc(n_len);
	if (!msg->name_buff) {
	/* Out of memory ? */
		goto fail;
	}
	memset(msg->name_buff, 0, n_len);

	strncpy((char *)msg->name_buff, name, strlen(name));
	
set_value:
	/* No value set, remove this message */
	if (value == NULL) {
		msgex_del_msg(mctx, msg);
		msgex_free_msg(msg);
		msg = NULL;
		return 0;
	}


	if (msg->data_buff) {
		fw_debug("%s:change msg(%s)\n",__func__,(char *)msg->name_buff);
		fw_debug("%s:value(old): %s\n",__func__,(char *)msg->data_buff);
		fw_debug("%s:to\n",__func__);
		free(msg->data_buff);
		msg->data_buff = NULL;
	}

	v_len = strlen(value) + 1;
	v_len= ALIGNED_4(v_len);

	msg->data_len = v_len;
	msg->data_buff = (void *)malloc(v_len);
	if (!msg->data_buff) {
	/* Out of memory ? */
		goto fail;
	}

	memset(msg->data_buff, 0, v_len);
	strncpy((char *)msg->data_buff, value, strlen(value));
	fw_debug("%s:set value(new): %s\n",__func__,(char *)msg->data_buff);
	
	msg->bdy_len = msg->data_len + msg->name_len + sizeof(msg->data_len) + sizeof(msg->name_len);

	return 0;

fail:
	msgex_del_msg(mctx, msg);
	msgex_free_msg(msg);
	return -ENOMEM;
}

int32_t fw_msg_set(struct fw_ctx *ctx, const char *name, const char *value)
{
	int ret = -1, i;
	struct extension *ext = NULL;

	if (ctx == NULL || name == NULL)
		return -EINVAL;

	for (i=0; i<ctx->nr_exts; i++) {
		if (ctx->exts[i].magic == MSG_EX_MAGIC) {
			ext = &ctx->exts[i];
			break;
		}
	}

	/* No message extension exist, need create this */
	if (ext == NULL) {
		ext = fw_register_extension(ctx, MSG_EX_MAGIC);
		if (!ext)
			goto fail;
		/* Allocate bare context */
		ext->ext_ctx = msgex_ctx_alloc();

	}

	ret = msgex_set((struct msg_ctx *)ext->ext_ctx, name, value);
	return ret;
fail:
	return -ENOMEM;
}

char * fw_msg_get(struct fw_ctx *ctx, const char *name)
{
	int i;
	char *value = NULL;
	struct extension *ext = NULL;
	struct msg_bdy *msg = NULL;

	if (name == NULL || ctx == NULL) {
		return NULL;
	}

	for (i=0; i<ctx->nr_exts; i++) {
		if (ctx->exts[i].magic == MSG_EX_MAGIC) {
			ext = &ctx->exts[i];
			break;
		}
	}

	/* No such extension for current context, obviously no messages */
	if (ext == NULL) {
		return NULL;
	}
	fw_debug("%s:Got extension ctx\n",__func__);

	msg = msgex_search_msg_by_name((struct msg_ctx *)ext->ext_ctx, name);
	if (!msg) {
		return NULL;
	}
	fw_debug("%s:Got msg(%p):(%p)\n",__func__, msg->name_buff, msg->data_buff);

	value = strdup((char *)msg->data_buff);

	return value;
}

static void msg_ex_parse(struct fw_ctx *ctx, void *buff)
{
	int i = 0;
	struct msg_ctx *mctx = NULL;
	struct ext_hdr *hdr = (struct ext_hdr *)buff;

	for (i=0; i<ctx->nr_exts; i++) {
		if (ctx->exts[i].magic == hdr->magic)
			break;
	}

	mctx = msgex_ctx_alloc();

	struct msg_bdy *msg = NULL;
	void *start = (void *)&hdr->data[0];
	void *pos = NULL;
	uint32_t cnt = 0;
	pos = start;

	while (cnt < hdr->len) {
		/* Allocate msg */
		msg = (struct msg_bdy *)malloc(sizeof(struct msg_bdy));

		/* Get msg total len */
		msg->bdy_len = *(uint32_t *)pos;

		/* Seek pos to key len*/
		pos = (void *)((unsigned long)pos+sizeof(msg->bdy_len));
		cnt += sizeof(msg->bdy_len);

		/*Get key len */
		msg->name_len = *(uint32_t *)pos;
		/* Seek pos to key data */
		pos = (void *)((unsigned long)pos+sizeof(msg->name_len));
		cnt += sizeof(msg->name_len);

		/* Allocate key buff */
		msg->name_buff = (void *)malloc(msg->name_len);
		memcpy(msg->name_buff, pos, msg->name_len);

		/* Seek pos to data len*/
		pos = (void *)((unsigned long)pos+msg->name_len);
		cnt += msg->name_len;

		/* Get data len */
		msg->data_len = *(uint32_t *)pos;
		/* Seek pos to data buff */
		pos = (void *)((unsigned long)pos+sizeof(msg->data_len));
		cnt += sizeof(msg->data_len);

		/* Allocate data buff */
		msg->data_buff = (void *)malloc(msg->data_len);
		memcpy(msg->data_buff, pos, msg->data_len);

		/* Seek pos to end of data buff, next msg*/
		pos = (void *)((unsigned long)pos+msg->data_len);
		cnt += msg->data_len;

		/* Add msg to context */
		msgex_add_msg(mctx, msg);
		msg = NULL;

	};


	ctx->exts[i].ext_ctx = (void *)mctx;
	return;
}

static int32_t msg_ex_fill(struct fw_ctx *ctx, void **buff)
{
	int i;
	int32_t len = 0;
	void *info = NULL, *pos = NULL, *start = NULL;
	struct ext_hdr *hdr = NULL;
	struct msg_ctx *mctx = NULL;
	struct msg_bdy *msg = NULL;

	for (i=0; i<ctx->nr_exts; i++) {
		if (ctx->exts[i].magic == MSG_EX_MAGIC)
			break;
	}

	mctx = (struct msg_ctx *)ctx->exts[i].ext_ctx;


	len += sizeof(struct ext_hdr);
	for_each_msg(msg, mctx) {
		len += msg->bdy_len;
		len += sizeof(msg->bdy_len);
	}

	info = (void *)malloc(len);
	memset(info, 0, len);

	hdr = (struct ext_hdr *)info;
	hdr->magic = MSG_EX_MAGIC;
	hdr->len = len - sizeof(struct ext_hdr);

	start = (void *)((unsigned long)info + sizeof(struct ext_hdr));
	pos = start;

	for_each_msg(msg, mctx) {

		/* Save mssage body len */
		*(uint32_t *)pos = msg->bdy_len;

		/* Seek pos to key len*/
		pos = (void *)((unsigned long)pos+sizeof(msg->bdy_len));

		/* Save key len */
		*(uint32_t *)pos = msg->name_len;
		/* Seek pos to key data */
		pos = (void *)((unsigned long)pos+sizeof(msg->name_len));
		/*save key buff*/
		memcpy(pos, msg->name_buff, msg->name_len);

		/* Seek pos to data len*/
		pos = (void *)((unsigned long)pos+msg->name_len);
		/*Save data len */
		*(uint32_t *)pos = msg->data_len;

		/* Seek pos to data buff */
		pos = (void *)((unsigned long)pos+sizeof(msg->data_len));
		/* Save data buff */
		memcpy(pos, msg->data_buff, msg->data_len);

		/* Seek pos to end of data buff, next msg*/
		pos = (void *)((unsigned long)pos+msg->data_len);

		if ((unsigned long)pos > ((unsigned long)start + len)) {
			printf("%s:ERROR: out of buff range, "
					"pos:%p, start:%p, len:%d\n", __func__, pos, start, len);
			break;
		}
	}

	*buff = info;

	return len;

}

static void msg_ex_release(struct fw_ctx *ctx, struct extension *in_ext)
{
	int i;
	struct extension *ext = NULL;
	struct msg_ctx *mctx = NULL;

	for (i=0; i<ctx->nr_exts; i++) {
		if (ctx->exts[i].magic == MSG_EX_MAGIC) {
			ext = &ctx->exts[i];
			break;
		}
	}

	/* Nothing need to do */
	if (ext == NULL || ext->ext_ctx == NULL) {
		return;
	}

	mctx = (struct msg_ctx *)ext->ext_ctx;

	msgex_ctx_release(mctx);
	mctx = NULL;
	ext->ext_ctx = NULL;

	return;
}

static void msg_ex_dump(struct fw_ctx *ctx)
{
	int i;
	struct extension *ext = NULL;
	struct msg_ctx *mctx = NULL;
	struct msg_bdy *msg = NULL;

	for (i=0; i<ctx->nr_exts; i++) {
		if (ctx->exts[i].magic == MSG_EX_MAGIC) {
			ext = &ctx->exts[i];
			break;
		}
	}
	printf("=== message extension dump ===\n");
	/* Nothing need to do */
	if (ext == NULL || ext->ext_ctx == NULL) {
		return;
	}
	mctx = (struct msg_ctx *)ext->ext_ctx;

	for_each_msg(msg, mctx) {
		printf("\n\tname:\t%s\n", (char *)msg->name_buff);
		printf("\tmessage:\t%s\n", (char *)msg->data_buff);
	}

	printf("\n=== message extension end ===\n");
}

struct extension_op msg_exop = {
	.magic = MSG_EX_MAGIC,
	.parse = msg_ex_parse,
	.fill = msg_ex_fill,
	.release = msg_ex_release,
	.dump = msg_ex_dump,
};

