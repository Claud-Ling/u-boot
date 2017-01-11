//#define DEBUG
#include <fw_core.h>

extern struct extension_op *find_ext_op(uint32_t magic);

static inline char * num_to_gmk(unsigned int num, char *gmkStr, int base)
{
	char magnitude[2] = {'\0', '\0'};
	unsigned int tmpVal = num;
	if (!(num & 0x3fffffff)) {
		tmpVal = num >> 30;
		magnitude[0] = 'G';
	} else if (!(num & 0xfffff)) {
		tmpVal = num >> 20;
		magnitude[0] = 'M';
	} else if(!(num & 0x3ff)) {
		tmpVal = num >> 10;
		magnitude[0] = 'K';
	}
	if(base == 10)
		sprintf(gmkStr, "%d%s", tmpVal, tmpVal ? magnitude : "");
	else if(base == 16)
		sprintf(gmkStr, "%x%s", tmpVal, tmpVal ? magnitude : "");
	else
	{
		assert(0);
		return NULL;
	}
	return gmkStr;
}

int32_t fw_vol_set_abilities(struct fw_vol *vol, fw_ability ability)
{
	local_set_bit(ability, (void *)&vol->info->ablities);
	return 0;
}

int32_t fw_vol_clr_abilities(struct fw_vol *vol, fw_ability ability)
{
	local_clear_bit(ability, (void *)&vol->info->ablities);
	return 0;
}


int32_t fw_vol_set_abl_active(struct fw_vol *vol, fw_abl_active active)
{
	local_set_bit(active, (void *)&vol->info->active_ablities);
	return 0;
}

int32_t fw_vol_clr_abl_active(struct fw_vol *vol, fw_abl_active active)
{
	local_clear_bit(active, (void *)&vol->info->active_ablities);
	return 0;
}

int32_t fw_vol_tst_abilities(struct fw_vol *vol, fw_abl_active active)
{
	return local_test_bit(active, (void *)&vol->info->ablities);
}

int32_t fw_vol_tst_abl_active(struct fw_vol *vol, fw_abl_active active)
{
	return local_test_bit(active, (void *)&vol->info->active_ablities);
}

void fw_vol_set_fs(struct fw_vol *vol, const char *fs)
{

	struct vol_info *info = vol->info;
	strncpy(&info->fs[0], fs, (sizeof(info->fs)-1));
	return;
}

void fw_vol_set_signature_en(struct fw_vol *vol)
{
	struct fw_part *part = NULL;
	struct list_head *pos = NULL;

	list_for_each(pos, &vol->parts) {
		part = list_entry(pos, struct fw_part, list);
		part->info->need_sig = 1;

	}

	return;
}

struct fw_ctx *fw_ctx_alloc(void)
{
	struct fw_ctx *ctx = (struct fw_ctx *)malloc(sizeof(struct fw_ctx));
	memset((void*)ctx, 0, sizeof(struct fw_ctx));
	INIT_LIST_HEAD(&ctx->vols);
	return ctx;
}

void fw_vol_free_parts(struct fw_vol *vol)
{
	struct fw_part *part = NULL;

	if (vol == NULL )
		return;

	while(!list_empty(&vol->parts)) {

		part = list_first_entry(&vol->parts, struct fw_part, list);
		list_del(&part->list);
		free(part->info);
		part->info = NULL;
		free(part);
		part = NULL;
	}

	return;
}

void fw_ctx_free(struct fw_ctx *ctx)
{
	int i;
	struct fw_vol *vol = NULL;
	struct extension_op *exop = NULL;
	if (ctx == NULL)
		return;

	/* Release extensions */
	for(i=0; i<ctx->nr_exts; i++) {
		exop = find_ext_op(ctx->exts[i].magic);
		exop->release(ctx, &ctx->exts[i]);
	}

	free(ctx->exts);
	ctx->exts = NULL;
	ctx->nr_exts = 0;

	while(!list_empty(&ctx->vols)) {

		vol = list_first_entry(&ctx->vols, struct fw_vol, list);
		fw_vol_free_parts(vol);
		list_del(&vol->list);
		free(vol->info);
		vol->info = NULL;
		free(vol);
		vol = NULL;
	}
	return;
}

struct fw_vol *fw_get_vol_by_name(struct fw_ctx *ctx, const char *name)
{

	struct fw_vol *fw_vol = NULL;
	struct list_head *pos = NULL;

	list_for_each(pos, &ctx->vols) {
		fw_vol = list_entry(pos, struct fw_vol, list);
		fw_debug("%s: fw_vol->info->name:%s, name:%s, len:%d\n", __func__,
			&fw_vol->info->name[0], name, sizeof(fw_vol->info->name));

		if(!strncmp((const char *)&fw_vol->info->name[0], name,
						(sizeof(fw_vol->info->name)-1))) {
			fw_debug("%s: Got volume!\n", __func__);
			goto match;
		}
	}

	return NULL;
match:
	return fw_vol;
}

int32_t fw_add_volume(struct fw_ctx *ctx, struct fw_vol *vol)
{
	struct fw_vol *vol_exist = NULL;

	if (!vol || !ctx || !strlen((const char *)vol->info->name)) {
		return -EINVAL;
	}

	vol_exist = fw_get_vol_by_name(ctx, &vol->info->name[0]);

	if (vol_exist) {
		return -EEXIST;
	}

	vol->ctx = ctx;
	list_add_tail(&vol->list, &ctx->vols);
	return 0;
}


int32_t fw_vol_set_depdent(struct fw_vol *vol, struct fw_vol *dep)
{
	local_set_bit(dep->info->index, (void *)&vol->info->dep_vol);
	return 0;
}

struct fw_vol *fw_get_vol_by_id(struct fw_ctx *ctx, uint32_t idx)
{

	struct fw_vol *vol = NULL;
	struct list_head *pos = NULL;

	list_for_each(pos, &ctx->vols) {
		vol = list_entry(pos, struct fw_vol, list);
		fw_debug("%s: fw_vol->info->index:%d, idx:%d\n", __func__,
								vol->info->index, idx);
		if(vol->info->index == idx) {
			fw_debug("%s: Got vol!\n", __func__);
			goto match;
		}
	}

	return NULL;
match:
	return vol;
}

struct fw_vol *fw_get_vol_by_ability_enabled(struct fw_ctx *ctx,
						fw_ability ab, fw_abl_active act)
{

	struct fw_vol *vol = NULL;
	struct list_head *pos = NULL;

	list_for_each(pos, &ctx->vols) {
		vol = list_entry(pos, struct fw_vol, list);
		if(fw_vol_tst_abilities(vol, ab) &&
			fw_vol_tst_abl_active(vol, act)) {
			fw_debug("%s: Got vol with ability:%d"
					" acitve_abl:%d\n", __func__, ab, act);
			goto match;
		}
	}

	return NULL;
match:
	return vol;
}

struct fw_vol *fw_get_dep_vol(struct fw_ctx *ctx, struct fw_vol *vol)
{
	unsigned long nr = 0;
	if (vol->info->dep_vol == 0) {
		return NULL;
	}

	/*
	 *TODO: How about have multi depend vols
	 */
	nr = find_first_bit((unsigned long *)&vol->info->dep_vol,
							sizeof(vol->info->dep_vol));

	return fw_get_vol_by_id(ctx, nr);
}

struct fw_part * fw_vol_get_part_by_id(struct fw_vol *vol, uint32_t id)
{
	struct list_head *pos = NULL;
	struct fw_part *part = NULL;

	list_for_each(pos, &vol->parts) {
		part = list_entry(pos, struct fw_part, list);
		if (part->info->index == id)
			goto match;
	}

	return NULL;
match:
	return part;

}

struct fw_part * fw_vol_get_active_part(struct fw_vol *vol)
{
	struct fw_part *part = NULL;
	struct fw_vol *parent_vol = NULL;
	struct fw_ctx *ctx = vol->ctx;
	uint32_t active_part = vol->info->active_part;
	int32_t parent_id = -1;

	if (ctx == NULL) {
		fw_err("%s: volume context point is NULL\n",__func__);
		goto skip_parent;
	}

	parent_id = __vchain_get_parent_id(ctx, vol->info->index);
	/* This volume may not be bind as a chain */
	if (parent_id < 0)
		goto skip_parent;

	parent_vol = fw_get_vol_by_id(ctx, parent_id);
	/* Active part id honor to parent selection */
	if (parent_vol) {
		active_part = parent_vol->info->active_part;
	}

	/* The volume chain is incorrect, rollback*/
	if (parent_vol && active_part >= vol->info->nr_parts) {
		fw_err("%s: volume chain is incorrect, honor volume active part\n", __func__);
		active_part = vol->info->active_part;
	}

skip_parent:
	part = fw_vol_get_part_by_id(vol, active_part);

	return part;
}

void fw_vol_set_active_part(struct fw_vol *vol, struct fw_part *part)
{
	vol->info->active_part = part->info->index;
	return;
}

/*
 * Follow Round-robin rules
 */
struct fw_part *fw_vol_get_inactive_part(struct fw_vol *vol)
{
	struct fw_vol *parent_vol = NULL;
	uint32_t inactive_id = 0, active_part = 0;
	struct list_head *pos = NULL;
	struct fw_part *part = NULL;
	struct fw_ctx *ctx = vol->ctx;
	int32_t parent_id = -1;

	active_part = vol->info->active_part;

	if (ctx == NULL) {
		fw_err("%s: volume context point is NULL\n",__func__);
		goto skip_parent;
	}

	parent_id = __vchain_get_parent_id(ctx, vol->info->index);
	/* This volume may not be bind as a chain */
	if (parent_id < 0)
		goto skip_parent;

	parent_vol = fw_get_vol_by_id(ctx, parent_id);
	/* Active part id honor to parent selection */
	if (!parent_vol) {
		goto skip_parent;
	}

	/* Use parent 'active_part' selection */
	if (vol->info->nr_parts == parent_vol->info->nr_parts) {
		active_part = parent_vol->info->active_part;
	}

skip_parent:
	(active_part + 1) < vol->info->nr_parts ?
		(inactive_id = (active_part + 1)) : (inactive_id = 0);

	list_for_each(pos, &vol->parts) {
		part = list_entry(pos, struct fw_part, list);
		if (part->info->index == inactive_id)
			goto match;
	}

	return NULL;
match:
	return part;
}

static struct fw_part *fw_alloc_part(void)
{
	struct fw_part *part = NULL;
	struct part_info *info = NULL;


	info = (struct part_info *) malloc(sizeof(struct part_info));
	fw_debug("%s: malloc part_info(%d bytes) @%p ", __func__, sizeof(struct part_info), info);
	if (!info) {
		fw_debug("FAILED \n");
		return NULL;
	}
	fw_debug("Success \n");

	part = (struct fw_part *)malloc(sizeof(struct fw_part));
	fw_debug("%s: malloc fw_part(%d bytes) @%p ", __func__, sizeof(struct fw_part), part);
	if (!part) {
		fw_debug("FAILED \n");
		free(info);
		return NULL;
	}
	fw_debug("Success \n");

	memset((void *)info, 0, sizeof(struct part_info));
	memset((void *)part, 0, sizeof(struct fw_part));

	part->info = info;
	part->info->magic = PART_INFO_VER(1,0);

	return part;
}

static void fw_part_set_index(struct fw_part *part, uint32_t idx)
{
	part->info->index = idx;
	return;
}

static void fw_part_set_name(struct fw_part *part, const char *name)
{
	struct part_info *info = part->info;
	strncpy(&info->name[0], name, (sizeof(info->name)-1));

	return;
}

struct fw_vol * fw_alloc_vol(struct fw_ctx *ctx, const char *name,  uint64_t sz, uint32_t nr_parts)
{
	uint32_t i = 0;
	struct vol_info *info = NULL;
	struct fw_vol *vol = NULL;
	struct fw_part *part = NULL;

	info = (struct vol_info *) malloc(sizeof(struct vol_info));
	fw_debug("%s: malloc vol_info(%d bytes) @%p ", __func__, sizeof(struct vol_info), info);
	if (!info) {
		fw_debug("FAILED \n");
		return NULL;
	}
	fw_debug("Success \n");

	vol = (struct fw_vol *) malloc(sizeof(struct fw_vol));
	fw_debug("%s: malloc fw_vol(%d bytes) @%p ", __func__, sizeof(struct fw_vol), vol);
	if (!vol) {
		fw_debug("FAILED \n");
		free(info);
		return NULL;
	}
	fw_debug("Success \n");

	memset((void *)info, 0, sizeof(struct vol_info));
	memset((void *)vol, 0, sizeof(struct fw_vol));

	strncpy(&info->name[0], name, (sizeof(info->name)-1));
	vol->info = info;

	INIT_LIST_HEAD(&vol->parts);

	/* Get index & flash start addr via ctx */
	vol->info->magic = VOL_INFO_VER(1, 0);
	vol->info->index = ctx->max_vol_idx++;
	vol->info->start = ctx->free_space_addr;
	vol->info->sz_perpart = sz;
	vol->info->nr_parts = nr_parts;

	/* Update free space start address */
	ctx->free_space_addr += sz * nr_parts;
	fw_debug("%s: Alloc vol(%s):\n", __func__, name);
	fw_debug("%s:\tindex:\t%d\n", __func__, vol->info->index);
	fw_debug("%s:\tstart:\t%x\n", __func__, vol->info->start);
	fw_debug("%s:\tsize per part:\t%x\n", __func__, vol->info->sz_perpart);
	fw_debug("%s:\tnr parts:%d\n", __func__, vol->info->nr_parts);

	for(i=0; i<nr_parts; i++) {
		char tmp_name[64] =  { 0 };
		part = fw_alloc_part();
		if (part == NULL) {
			fw_err("%s: Can't alloc part%d for vol(%s)\n",
						 __func__, i, name);
			goto fail;
		}

		list_add_tail(&part->list, &vol->parts);

		snprintf(&tmp_name[0], sizeof(tmp_name), "%s%d", name, i);
		fw_part_set_name(part, tmp_name);

		fw_part_set_index(part, i);

		part->info->start = (vol->info->start + i*sz);
		part->info->size = sz;
		fw_debug("%s:\tCreat part:%s\n", __func__, tmp_name);
		fw_debug("%s:\t\tstart:%x\n", __func__, part->info->start);
		fw_debug("%s:\t\tsize:%x\n", __func__, part->info->size);
	}


	return vol;

fail:
	return NULL;
}

uint32_t fw_vol_info_fill(struct fw_ctx *ctx, void **inf)
{
	uint32_t total_sz = 0, vol_sz = 0;
	struct list_head *vol_pos = NULL, *part_pos = NULL;
	struct fw_vol *vol = NULL;
	void *info = NULL, *cur = NULL;
	struct vol_head *vol_head = NULL;
	struct fw_part *part = NULL;

	list_for_each(vol_pos, &ctx->vols) {
		vol_sz = 0;
		vol = list_entry(vol_pos, struct fw_vol, list);
		fw_debug("%s: vol = %p, vol->info = %p\n", __func__, vol, vol->info);
		fw_debug("%s: Calc vol(%s) size.\n", __func__, vol->info->name);
		vol_sz += sizeof(struct vol_info);
		vol_sz += (sizeof(struct part_info) * vol->info->nr_parts);
		fw_debug("%s:\tvol size: %08x\n", __func__, vol_sz);
		fw_debug("%s:\tpos: %p, ctx->vols: %p\n", __func__, vol_pos, &ctx->vols);
		total_sz += vol_sz;
	}

	total_sz += sizeof(struct vol_head);


	info = (void *)malloc(total_sz);
	memset(info, 0, total_sz);

	vol_head  = (struct vol_head *)info;
	cur = (void *)((unsigned long)vol_head + sizeof(struct vol_head));

	list_for_each(vol_pos, &ctx->vols) {
		vol = list_entry(vol_pos, struct fw_vol, list);

		memcpy(cur, (void *)vol->info, sizeof(struct vol_info));

		cur += sizeof(struct vol_info);

		list_for_each(part_pos, &vol->parts) {
			part = list_entry(part_pos, struct fw_part, list);

			memcpy(cur, (void *)part->info, sizeof(struct part_info));

			cur += sizeof(struct part_info);
		}
		vol_head->n_entry++;
	}
	vol_head->magic = VOL_HEAD_VER(1, 0);

	*inf = info;

	return total_sz;
}

int32_t fw_fill_info(struct fw_ctx *ctx, void **inf)
{
	int i;
	uint32_t vol_info_sz = 0, total_sz = 0, bdy_entry_sz = 0;
	uint32_t frst_bdy_off, n_bdy_off;
	void *vol_info = NULL, *pos = NULL;
	struct extension_op *exop = NULL;
	struct fw_head *head = NULL;
	void *info = NULL;


	vol_info_sz = fw_vol_info_fill(ctx, &vol_info);
	total_sz += vol_info_sz;

	if (vol_info == NULL) {
		goto fail;
	}


	for (i=0; i<ctx->nr_exts; i++) {
		exop = find_ext_op(ctx->exts[i].magic);
		ctx->exts[i].info_sz = exop->fill(ctx, &ctx->exts[i].info_data);
		total_sz += ctx->exts[i].info_sz;
	}

	total_sz += sizeof(struct fw_head);
	bdy_entry_sz = (sizeof(uint32_t) * (ctx->nr_exts + 1));
	total_sz += bdy_entry_sz;

	info = (void *)malloc(total_sz);
	if (info == NULL) {
		goto fail1;
	}
	memset(info, 0, total_sz);

	head = (struct fw_head *)info;

	pos = (void *)((unsigned long)info + sizeof(struct fw_head)+ bdy_entry_sz);

	/* First body is volume info */
	frst_bdy_off = (uint32_t)((unsigned long)pos - (unsigned long)info);
	head->bdy[0] = frst_bdy_off;

	/* Store volume info to firmware info */
	memcpy(pos, vol_info, vol_info_sz);
	free(vol_info);
	vol_info = NULL;
	pos = (void *)((unsigned long)pos + vol_info_sz);


	/* Due extension, if we have */
	for (i=0; i<ctx->nr_exts; i++) {
		n_bdy_off = (uint32_t)((unsigned long)pos - (unsigned long)info);
		head->bdy[i+1] = n_bdy_off;

		/* No data for this extension */
		if (ctx->exts[i].info_data == NULL) {
			continue;
		}

		memcpy(pos, ctx->exts[i].info_data, ctx->exts[i].info_sz);
		pos = (void *)((unsigned long)pos + ctx->exts[i].info_sz);
		free(ctx->exts[i].info_data);
		ctx->exts[i].info_data = NULL;
		ctx->exts[i].info_sz = 0;
	}


	head->magic = FW_INFO_VER(1, 0);
	head->n_bdy = 1 + ctx->nr_exts;
	head->valid = ctx->valid;
	head->len = (unsigned long)pos - (unsigned long)head->bdy;
	head->crc = crc32(0, (void *)head->bdy, head->len);
	head->head_crc = crc32(0, (void *)head,
				(sizeof(struct fw_head)-sizeof(uint32_t)));

	*inf = info;
	return total_sz;

fail1:
	for (i=0; i<ctx->nr_exts; i++) {
		if (ctx->exts[i].info_data != NULL) {
			free(ctx->exts[i].info_data);
			ctx->exts[i].info_data = NULL;
			ctx->exts[i].info_sz = 0;
		}
	}

	free(vol_info);
fail:
	return -ENOMEM;
}

static int32_t info_valid(void *info)
{
	struct fw_head *head = info;
	uint32_t crc_check = 0;

	if (info == NULL) {
		return 0;
	}

	crc_check = crc32(0, (void *)head->bdy, head->len);
	fw_debug("%s: crc_check:%08x, head->crc:%08x, "
			"head->len:%d, head->valid:%d\n",
				__func__, crc_check, head->crc, head->len, head->valid);

	if (crc_check != head->crc || head->len == 0) {
		fw_debug("%s: crc check failed\n", __func__);
		return 0;
	}

	return 1;
}

static uint64_t info_save_offs = 0x0;
static void *get_active_info(void *info1, void *info2)
{
	void *info = NULL;

	if (!info_valid(info1) && !info_valid(info2)) {
		return NULL;
	} else if(info_valid(info1) && !info_valid(info2)) {
		info = info1;
		goto out;
	} else if(!info_valid(info1) && info_valid(info2)) {
		info = info2;
		goto out;
	}

	/*info1 & info2 both valid case */
	if (INFO_TO_VALID_NUM(info1) == INFO_MAX_VALID_NUM &&
					 INFO_TO_VALID_NUM(info2) == 0) {
			info = info2;

	} else if (INFO_TO_VALID_NUM(info1) == 0 &&
			INFO_TO_VALID_NUM(info2) == INFO_MAX_VALID_NUM) {
			info = info1;

	} else if (INFO_TO_VALID_NUM(info1) > INFO_TO_VALID_NUM(info2)) {
			info = info1;
	} else {
			info = info2;
	}

out:
	(info == info1) ? (info_save_offs = INFO_PART_SZ) : (info_save_offs = 0x0);
	return info;
}

struct fw_ctx * fw_parse_info(void *info)
{
	struct list_head *pos = NULL;
	struct fw_part *part = NULL;
	void *cur = NULL;
	uint32_t i = 0, sz = 0;
	void *data = NULL;
	uint32_t magic = 0;
	unsigned long  end_of_info = 0;
	struct extension_op *exop = NULL;
	struct vol_head *vol_head = NULL;
	struct fw_head *head = info;


	/* Volume info always located at fist body */
	vol_head = (struct vol_head *)((*(uint32_t *)&head->bdy[0]) + (unsigned long)info);

	end_of_info = (unsigned long)head->len + (unsigned long)&head->bdy[0];

	if (!info_valid(info))
		return NULL;

	struct fw_ctx *ctx = fw_ctx_alloc();

	/* Let context recoder valid number */
	ctx->valid = head->valid;

	cur =(void *)((unsigned long)vol_head + sizeof(struct vol_head));

	for (i=0; i<vol_head->n_entry; i++) {
		void *p_info = (void *)((unsigned long)cur + sizeof(struct vol_info));
		struct vol_info *vol_info = (struct vol_info *)cur;

		struct fw_vol *vol = fw_alloc_vol(ctx,
				vol_info->name, vol_info->sz_perpart, vol_info->nr_parts);

		memcpy((void *)vol->info, (void *)vol_info, sizeof(struct vol_info));
		list_for_each(pos, &vol->parts) {
			part = list_entry(pos, struct fw_part, list);

			/*Restore part_info in volume */
			memcpy((void *)part->info, p_info, sizeof(struct part_info));

			/* Seek p_info to next part_info entry */
			p_info = (void *)((unsigned long)p_info
							+ sizeof(struct part_info));

		}

		/*Seek 'cur' to next vol_info entry */
		cur = (void *)((unsigned long)cur + sizeof(struct vol_info) +
					vol_info->nr_parts * sizeof(struct part_info));

		/* Add vol to ctx for restore */
		fw_add_volume(ctx, vol);

		/* We got wrong firmware info, but with valid CRC ?*/
		if ((unsigned long)cur > end_of_info) {
			return NULL;
		}
	}

	/* Check have extentions */
	ctx->nr_exts = head->n_bdy - 1;
	if (ctx->nr_exts == 0 || ctx->nr_exts < 0) {
		ctx->nr_exts = 0;
		ctx->exts = NULL;
		goto skip_extension;
	}

	/* Allocate extension context */
	sz = (sizeof(struct extension)*ctx->nr_exts);
	ctx->exts = (struct extension *)malloc(sz);
	if (ctx->exts == NULL) {
		return NULL;
	}

	memset((void*)ctx->exts, 0, sz);

	for(i=0; i<ctx->nr_exts; i++) {
		data = (void *)(head->bdy[i+1] + (unsigned long)info);
		magic = *(uint32_t *)data;
		/* Get a context slot, make its magic num equ exts' magic */
		ctx->exts[i].magic = magic;

		exop = find_ext_op(magic);
		if (exop == NULL) {
		/* Can't handle such extensions, skip */
			continue;
		}

		/* Let extension construct its context */
		exop->parse(ctx, data);
	}

skip_extension:

	return ctx;

}

struct extension *fw_register_extension(struct fw_ctx *ctx, uint32_t magic)
{
	struct extension *ext = NULL;
	void *dest = NULL;
	uint32_t sz = 0;
	uint32_t nr_exts = (ctx->nr_exts + 1);


	sz = (sizeof(struct extension)*nr_exts);
	dest = (void *)malloc(sz);
	/* Out of memory ? */
	if (dest == NULL) {
		return NULL;
	}
	memset(dest, 0, sz);

	memcpy(dest, (void *)ctx->exts, (sizeof(struct extension) * ctx->nr_exts));

	free(ctx->exts);
	ctx->exts = dest;
	ctx->nr_exts = nr_exts;

	ext = &ctx->exts[nr_exts - 1];
	ext->magic = magic;

	return ext;

}

int32_t fw_ctx_to_info(struct fw_ctx *ctx, void **info)
{
	int32_t info_sz = fw_fill_info(ctx, info);

	fw_debug("%s: Got info_sz = %08x\n", __func__, info_sz);
	return info_sz;
}

static int32_t load_data_from_flash(char *dev, void *buff, uint64_t at, uint64_t sz)
{
	int ret = -1;
	int64_t rd_sz = 0;
	int flash = fdetecter_main_storage_typ();

	struct flash_op *op = (struct flash_op *)flash_op_open(dev, flash);

	ret = flash_op_seek(op, at, SEEK_SET);
	if (ret < 0) {
		fw_debug("%s: seek to (%0llx) failed\n", __func__, at);
		goto failed;
	}

	rd_sz = flash_op_read(op, buff, (uint64_t)sz);
	if (rd_sz != sz) {
		fw_debug("%s: load data failed\n", __func__);
		goto failed;
	}


	flash_op_close(op);
	return 0;
failed:
	flash_op_close(op);
	return -ENODEV;
}

static int32_t check_fw_head_crc(struct fw_head *head)
{
	uint32_t crc_check = crc32(0, (void *)head, (sizeof(struct fw_head)-sizeof(uint32_t)));
	return ((crc_check == head->head_crc) ? 0 : -1);
}

static int32_t load_info_from_flash(void **inf1, void **inf2)
{
	int ret = -1;
	char *dev = NULL;
	struct fw_head *head = NULL;
	char buff[512];
	uint64_t info_base = 0;
	uint64_t len = 0;
	void *info1 = NULL;
	void *info2 = NULL;

	int main_flash = fdetecter_main_storage_typ();

	if (main_flash == FW_FTYP_EMMC) {
		dev = EMMC_DEV;
		info_base = EMMC_FW_INFO_BASE;
	} else {
		dev = NAND_DEV;
		info_base = NAND_FW_INFO_BASE;
	}

	head = (struct fw_head *)&buff[0];
	fw_debug("%s: load info1\n", __func__);

	/* Tentatively probe firmware info size, load 512byte(eMMC) block size*/
	memset(&buff[0], 0, sizeof(buff));
	ret = load_data_from_flash(dev, buff, info_base, sizeof(buff));
	if (ret) {
		goto load_info2;
	}

	if (check_fw_head_crc(head)) {
		goto load_info2;
	}

	len = sizeof(struct fw_head) + head->len;

	info1 = (void *)malloc(len);
	/* Out of memory? */
	if (info1 == NULL) {
		goto load_info2;
	}

	memset(info1, 0, len);
	ret = load_data_from_flash(dev, info1, info_base, len);
	if (ret) {
		free(info1);
		info1 = NULL;
	}

load_info2:
	memset(&buff[0], 0, sizeof(buff));
	ret = load_data_from_flash(dev, buff,
				(info_base+(uint64_t)INFO_PART_SZ), sizeof(buff));

	if (ret) {
		goto out;
	}

	if (check_fw_head_crc(head)) {
		goto out;
	}

	len = sizeof(struct fw_head) + head->len;

	info2 = (void *)malloc(len);
	/* Out of memory? */
	if (info2 == NULL) {
		goto out;
	}

	memset(info2, 0, len);
	ret = load_data_from_flash(dev, info2, (info_base+(uint64_t)INFO_PART_SZ), len);
	if (ret) {
		free(info2);
		info2 = NULL;
	}
out:
	*inf1 = info1;
	*inf2 = info2;
	return 0;

}

static int32_t save_info_to_flash(void *info, uint64_t sz)
{
	int ret = -1;
	char *dev = NULL;
	uint64_t info_base = 0;
	int64_t wr_sz = 0;
	int main_flash = fdetecter_main_storage_typ();

	if (main_flash == FW_FTYP_EMMC) {
#if !defined(__KERNEL__)
		if (!access("/dev/block/mmcblk0", F_OK)) {
			dev = "/dev/block/mmcblk0";
		} else {
			dev = EMMC_DEV;
		}
#elif defined(__UBOOT__)
		dev = EMMC_DEV;
#endif
		info_base = EMMC_FW_INFO_BASE;
	} else {
		dev = NAND_DEV;
		info_base = NAND_FW_INFO_BASE;
	}

	info_base += info_save_offs;
	/* Round to next firmware info on flash, ready for next time save */
	(info_save_offs > 0) ? (info_save_offs = 0): (info_save_offs = INFO_PART_SZ);

	fw_debug("%s:Write fw_info dev:%s, save to:%0llx\n", __func__, dev, info_base);
	struct flash_op *op = (struct flash_op *)flash_op_open(dev, main_flash);

	ret = flash_op_seek(op, info_base, SEEK_SET);
	if (ret < 0) {
		fw_debug("%s: flash_op_seek(%0llx) failed\n", __func__, info_base);
		goto failed;
	}

	fw_debug("%s: flash_op_seek(%0llx) success\n", __func__, info_base);
	wr_sz = flash_op_write(op, info, sz);
	if (wr_sz != sz) {
		fw_debug("%s: save info failed\n", __func__);
		goto failed;
	}
	fw_debug("%s: save info to flash success\n", __func__);


	flash_op_close(op);
	return 0;
failed:
	flash_op_close(op);
	return -ENODEV;
}

struct fw_ctx *fw_ctx_init_from_flash(void)
{
	int ret = -1;
	uint32_t info_idx = 0;
	void *info1 = NULL;
	void *info2 = NULL;
	struct fw_ctx *ctx = NULL;

	ret = load_info_from_flash(&info1, &info2);
	if (ret)
		goto failed;

	void *info = get_active_info(info1, info2);
	if (!info) {
		goto failed;
	}

	(info == info1) ? (info_idx = 0) : (info_idx = 1);
	fw_debug("%s:Active info idx:%d name:info%d\n", __func__, info_idx, (info_idx+1));

	ctx = fw_parse_info(info);
	if (ctx == NULL) {
		goto failed;
	}
	ctx->idx = info_idx;

	if (info1)
		free(info1);
	if (info2)
		free(info2);
	info1 = NULL;
	info2 = NULL;
	return ctx;
failed:

	fw_debug("%s: Can't find any valid 'firmware info' on main flash\n",__func__);
	if (info1)
		free(info1);
	if (info2)
		free(info2);
	info1 = NULL;
	info2 = NULL;
	return NULL;
}

static struct fw_ctx *g_bd_ctx = NULL;
static struct fw_ctx *g_updater_ctx = NULL;

struct fw_ctx *fw_get_board_ctx(void)
{
	if (g_bd_ctx == NULL) {
		g_bd_ctx = fw_ctx_init_from_flash();
	}

	/*
	 * For updater specify a file save firmware info not flush to flash
	 */ 
	if (g_updater_ctx)
		return g_updater_ctx;

	return g_bd_ctx;
}

struct fw_ctx *fw_board_ctx_reinit(void)
{
	if (g_bd_ctx) {
		fw_ctx_free(g_bd_ctx);
		g_bd_ctx = NULL;
	}

	return fw_get_board_ctx();
}

int32_t fw_ctx_save_to_flash(struct fw_ctx *ctx)
{
	int32_t ret = -1;
	void *info = NULL;

	INC_VALID_COUNT(ctx->valid);
	uint32_t sz = fw_ctx_to_info(ctx, &info);
	if (info == NULL) {
		goto err;
	}

	ret = save_info_to_flash(info, sz);
	if (ret)
		goto err1;

	free(info);
	info = NULL;
	return 0;
err1:
	free(info);
	info = NULL;
err:
	return -ENODEV;
}

int32_t fw_ctx_force_save_to_flash(struct fw_ctx *ctx)
{
	int ret = -1;

	ret = fw_ctx_save_to_flash(ctx);
	fw_debug("%s: save firmware info to "
			"flash first bank! ret = %d\n", __func__, ret);
	if (ret)
		goto out;

	ret = fw_ctx_save_to_flash(ctx);
	fw_debug("%s: save firmware info to "
			"flash second bank! ret = %d\n", __func__, ret);

out:
	return ret;
}

#if !defined(__KERNEL__)
struct fw_ctx *fw_ctx_init_from_file(const char *file)
{
	struct fw_ctx *ctx = NULL;
	void *info = NULL;
	int ret = -1;
	struct stat info_st;
	ret = stat(file, &info_st);
	if (ret < 0) {
		return NULL;
	}

	if (info_st.st_size == 0) {
		return NULL;
	}

	info = (void *)malloc(info_st.st_size);
	fw_debug("%s: got info buff@%p size:%d\n", __func__, info, info_st.st_size);
	if (!info) {
		return NULL;
	}

	int fd = open(file, O_CREAT | O_RDWR, 0666);
	if (fd < 0) {
		goto fail;
	}

	ret = read(fd, info, info_st.st_size);
	if (ret != info_st.st_size) {
		goto fail1;
	}

	ctx = fw_parse_info(info);
	if (ctx == NULL) {
		goto fail1;
	}

	free(info);
	info = NULL;
	close(fd);

	return ctx;

fail1:
	close(fd);
fail:
	free(info);
	info = NULL;
	return NULL;
}

int32_t fw_ctx_save_to_file(struct fw_ctx *ctx, const char *file)
{
	void *info = NULL;
	truncate(file, 0);

	int fd = open(file, O_CREAT | O_RDWR, 0666);

	uint32_t sz = fw_ctx_to_info(ctx, &info);
	lseek(fd, 0 , SEEK_SET);
	write(fd, info, sz);
	close(fd);
	free(info);

	return 0;
}
#endif /*!define(__KERNEL__)*/

static char *id_to_dev(uint32_t part_idx)
{
	char str[32] = { 0 };
	int main_flash = fdetecter_main_storage_typ();

	/* Limited to 256 byter, considering u-boot case */
	char *dev =(char *)malloc(256);
	memset(dev, 0, 256);

	if (main_flash == FW_FTYP_EMMC) {
		/* eMMC part index start from '1' */
		part_idx +=1;
#if !defined(__KERNEL__)
		if (!access("/dev/block/mmcblk0", F_OK)) {
			strcat(dev, "/dev/block/mmcblk0p");
		} else {
			strcat(dev, "/dev/mmcblk0p");
		}
#else
		strcat(dev, "/dev/mmcblk0p");
#endif
	} else {
		strcat(dev, "/dev/mtd");
	}

	sprintf(str, "%d", part_idx);

	strcat(dev, str);

	return dev;
}

static uint32_t calc_vol_first_part_idx(struct fw_ctx *ctx, struct fw_vol *v)
{

	uint32_t idx = 0;
	struct list_head *vol_pos = NULL;
	struct fw_vol *vol = NULL;



	list_for_each(vol_pos, &ctx->vols) {
		vol = list_entry(vol_pos, struct fw_vol, list);
		if (vol == v) {
			break;
		}
		idx += vol->info->nr_parts;
	}

	fw_debug("%s: vol(%s) first part idx: %d\n", __func__, v->info->name, idx);
	return idx;

}

char *volume_name_to_dev(struct fw_ctx *ctx, const char *name, uint32_t flags)
{
	struct fw_vol *vol = NULL, *bd_vol = NULL;
	struct fw_part *part = NULL, *bd_part = NULL;
	struct fw_ctx *board_ctx = fw_get_board_ctx();

	uint32_t idx = 0;
	char *dev = NULL;

	vol = fw_get_vol_by_name(ctx, name);
	bd_vol = fw_get_vol_by_name(board_ctx, name);


	idx = calc_vol_first_part_idx(ctx, vol);

	if (flags == FW_VOL_CONTENT_INUSE) {
		bd_part = fw_vol_get_active_part(bd_vol);
	}else {
		bd_part = fw_vol_get_inactive_part(bd_vol);
		/* Update part_info to board */
		part = fw_vol_get_active_part(vol);
		bd_part->info->need_sig = part->info->need_sig;
		bd_part->info->valid_sz = part->info->valid_sz;
		memcpy((void *)bd_part->info->part_sig,
			 	(void *)part->info->part_sig, 256);

		memcpy((void *)bd_part->info->info_sig,
			 	(void *)part->info->info_sig, 256);

		/* Set free part as active part */
		fw_vol_set_active_part(bd_vol, bd_part);
	}

	/* Add volume internal part index offset */
	fw_debug("vol->name:\t%s\n",bd_vol->info->name);
	fw_debug("vol->fs:\t%s\n",bd_vol->info->fs);
	fw_debug("vol->index:\t%d\n",bd_vol->info->index);
	fw_debug("vol->dep_vol:\t%llx\n",bd_vol->info->dep_vol);
	fw_debug("vol->ablities:\t%08x\n",bd_vol->info->ablities);
	fw_debug("vol->active_ablities:\t%08x\n",bd_vol->info->active_ablities);
	fw_debug("vol->start:\t%llx\n",bd_vol->info->start);
	fw_debug("vol->sz_perpart:\t%llx\n",bd_vol->info->sz_perpart);
	fw_debug("vol->nr_parts:\t%08x\n",bd_vol->info->nr_parts);
	fw_debug("vol->active_part:\t%d\n",bd_vol->info->active_part);

	fw_debug("\tpart->name:\t%s\n", bd_part->info->name);
	fw_debug("\tpart->index:\t%d\n", bd_part->info->index);
	fw_debug("\tpart->need_sig:\t%d\n", bd_part->info->need_sig);
	fw_debug("\tpart->start:\t%llx\n", bd_part->info->start);
	fw_debug("\tpart->size:\t%llx\n", bd_part->info->size);
	fw_debug("\tpart->valid_sz:\t%llx\n", bd_part->info->valid_sz);
	fw_debug("\n");
	idx += bd_part->info->index;

	dev = id_to_dev(idx);
	fw_debug("%s: Got part(%s) for vol(%s)\n", __func__, dev, name);

	return dev;
}

int32_t fw_open_volume(struct fw_ctx *ctx, const char *name, uint32_t flags)
{
	char *dev = NULL;
	struct flash_op *op = NULL;
	uint32_t flash_typ = fdetecter_main_storage_typ();

	dev = volume_name_to_dev(ctx, name, flags);

	op = (struct flash_op *)flash_op_open(dev, flash_typ);

	free(dev);

	if (!op) {
		return -ENODEV;
	}
	ctx->fop = (void *)op;

	return 0;

}

int64_t fw_write_volume(struct fw_ctx  *ctx, void *buff, uint64_t len)
{
	struct flash_op *op = (struct flash_op *)ctx->fop;

	if (!op)
		return -ENODEV;

	return flash_op_write(op, buff, len);
}

int64_t fw_read_volume(struct fw_ctx *ctx, void *buff, uint64_t len)
{
	struct flash_op *op = (struct flash_op *)ctx->fop;

	if (!op)
		return -ENODEV;

	return flash_op_read(op, buff, len);
}

void fw_close_volume(struct fw_ctx  *ctx)
{
	struct flash_op *op = (struct flash_op *)ctx->fop;

	if (!op)
		return;

	flash_op_close(op);
	ctx->fop = NULL;

	return;
}
/* Partition list len, MAX to 8192 bytes, actually u-boot env only 4096 bytes
 * So, we can support partition list so long.
*/
#define PART_LIST_LEN	(8192)
char *fw_get_part_list(struct fw_ctx *ctx)
{
	struct list_head *vol_pos = NULL, *part_pos = NULL;
	struct fw_vol *vol = NULL;
	struct fw_part *part = NULL;

	char str[128] = { 0 };

	char *part_list = (char *)malloc(PART_LIST_LEN);
	memset((void *)part_list, 0, PART_LIST_LEN);


	list_for_each(vol_pos, &ctx->vols) {
		vol = list_entry(vol_pos, struct fw_vol, list);

		list_for_each(part_pos, &vol->parts) {
			part = list_entry(part_pos, struct fw_part, list);
			memset((void *)str, 0, sizeof(str));
			num_to_gmk(part->info->size, str, 10);
			strcat(part_list, str);
			memset((void *)str, 0, sizeof(str));
			sprintf(str, "(%s),", part->info->name);
			strcat(part_list, str);
		}
	}

	/*append free part*/
	memset((void *)str, 0, sizeof(str));
	sprintf(str, "-(free)");
	strcat(part_list, str);

	return part_list;

}

int32_t fw_ctx_check_conflict(struct fw_ctx *ctx1, struct fw_ctx *ctx2)
{
	if (ctx1 == NULL || ctx2 == NULL) {
		goto conflict;
	}

	char *p_list1 = fw_get_part_list(ctx1);
	char *p_list2 = fw_get_part_list(ctx2);

	if (strcmp(p_list1, p_list2)) {
		goto conflict1;
	}

	free(p_list1);
	free(p_list2);
	p_list1 = NULL;
	p_list2 = NULL;
	return 0;
conflict1:
	free(p_list1);
	free(p_list2);
	p_list1 = NULL;
	p_list2 = NULL;
conflict:
	return 1;
}

#if !defined(__KERNEL__)
int32_t fw_update_bootloader(struct fw_ctx *ctx, char *image)
{
	uint32_t flash = fdetecter_boot_storage_typ();
	return flash_op_update_bootloader(image, flash);
}

struct fw_ctx *fw_init(const char *file)
{
	int ret = -1;
	struct fw_ctx *bd_ctx = fw_get_board_ctx();
	struct fw_ctx *ctx = fw_ctx_init_from_file(file);

	if (!ctx) {
		return NULL;
	}
	if (fw_ctx_check_conflict(bd_ctx, ctx)) {
		printf("Firmware info on board conflict "
		"with package, force use firmware info from package!!\n");
		ret = fw_ctx_force_save_to_flash(ctx);
		fw_board_ctx_reinit();

		if (ret)
			goto err1;
	} else {
	/* Vchain extension merge, alwasy honnor updater's firmware info */
		__vchain_merge(bd_ctx, ctx);
	}


	return ctx;
err1:
	fw_ctx_free(ctx);
	return NULL;
}

int32_t fw_updater_ctx_restore(const char *file)
{
	struct fw_ctx *ctx = fw_ctx_init_from_file(file);

	/* Updater first initial case, not context file exist
	 * So, we need copy context from board(flash).
 	*/
	if (!ctx) {
		ctx = fw_get_board_ctx();
	}

	g_updater_ctx = ctx;

	/* Update valid count*/
	if (g_updater_ctx != g_bd_ctx) {
		g_updater_ctx->valid = g_bd_ctx->valid;
	}

	return 0;
}

int32_t fw_updater_ctx_save(const char *file)
{
	int32_t ret = -1;
	if (!g_updater_ctx)
		return 0;
	ret = fw_ctx_save_to_file(g_updater_ctx, file);
	if (g_updater_ctx != g_bd_ctx) {
		fw_ctx_free(g_updater_ctx);
	}
	g_updater_ctx = NULL;

	return ret;
}
#endif

void fw_deinit(struct fw_ctx *ctx)
{
	struct fw_ctx *bd_ctx = fw_get_board_ctx();
	fw_ctx_save_to_flash(bd_ctx);
	//fw_ctx_free(ctx);
}

void fw_dump(struct fw_ctx *ctx)
{
	int i;
	struct list_head *vol_pos = NULL, *part_pos = NULL;
	struct fw_vol *vol = NULL;
	struct fw_part *part = NULL;
	struct extension_op *exop = NULL;

	list_for_each(vol_pos, &ctx->vols) {
		vol = list_entry(vol_pos, struct fw_vol, list);
		fw_msg("vol->name:\t%s\n", vol->info->name);
		fw_msg("vol->fs:\t%s\n", vol->info->fs);
		fw_msg("vol->index:\t%d\n", vol->info->index);
		fw_msg("vol->dep_vol:\t%llx\n", (long long unsigned int)vol->info->dep_vol);
		fw_msg("vol->ablities:\t%08x\n", vol->info->ablities);
		fw_msg("vol->active_ablities:\t%08x\n", vol->info->active_ablities);
		fw_msg("vol->start:\t%llx\n", (long long unsigned int)vol->info->start);
		fw_msg("vol->sz_perpart:\t%llx\n", (long long unsigned int)vol->info->sz_perpart);
		fw_msg("vol->nr_parts:\t%08x\n", vol->info->nr_parts);
		fw_msg("vol->active_part:\t%d\n", vol->info->active_part);

		list_for_each(part_pos, &vol->parts) {
			part = list_entry(part_pos, struct fw_part, list);
			fw_msg("\tpart->name:\t%s\n", part->info->name);
			fw_msg("\tpart->index:\t%d\n", part->info->index);
			fw_msg("\tpart->need_sig:\t%d\n", part->info->need_sig);
			fw_msg("\tpart->start:\t%llx\n", (long long unsigned int)part->info->start);
			fw_msg("\tpart->size:\t%llx\n", (long long unsigned int)part->info->size);
			fw_msg("\tpart->valid_sz:\t%llx\n", (long long unsigned int)part->info->valid_sz);
			fw_msg("\n");
		}
		fw_msg("\n");
	}

	/* Dump extensions */
	for(i=0; i<ctx->nr_exts; i++) {
		exop = find_ext_op(ctx->exts[i].magic);
		exop->dump(ctx);
	}
	return;
}

int32_t fw_create_volume_chain(struct fw_ctx *ctx, const char *chain,
								const char *vol_name)
{
	struct fw_vol *vol = NULL;
	if (ctx == NULL || chain == NULL || vol_name == NULL)
		return -EINVAL;

	vol = fw_get_vol_by_name(ctx, vol_name);

	if (!vol)
		return -ENODEV;

	return __create_vchain(ctx, chain, vol->info->index);
}

int32_t fw_bind_volume_to_chain(struct fw_ctx *ctx, const char *chain,
								const char *vol_name)
{
	struct fw_vol *vol = NULL;
	if (ctx == NULL || chain == NULL || vol_name == NULL)
		return -EINVAL;

	vol = fw_get_vol_by_name(ctx, vol_name);

	if (!vol)
		return -ENODEV;

	return __vchain_bind_vol(ctx, chain, vol->info->index);
	
}

int32_t fw_del_chain(struct fw_ctx *ctx, const char *chain)
{
	if (ctx == NULL || chain == NULL)
		return -EINVAL;

	return __vchain_del(ctx, chain);
}


