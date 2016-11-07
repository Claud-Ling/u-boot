#include <fw_core.h>

/*
 * General API
 */

int32_t libfw_set_volume_bootable(void *ctx, const char *name)
{
	struct fw_vol *vol = NULL;

	vol = (struct fw_vol *)fw_get_vol_by_name((struct fw_ctx *)ctx, name);
	if (!vol) {
		goto err;
	}

	fw_vol_set_abilities(vol, FW_AB_BOOT);

	return 0;
err:
	return -ENODEV;

}

int32_t libfw_set_volume_bootactive(void *ctx, const char *name)
{
	struct fw_vol *vol = NULL;
	struct fw_vol *boot_vol = NULL;

	vol = (struct fw_vol *)fw_get_vol_by_name((struct fw_ctx *)ctx, name);
	if (!vol) {
		goto err;
	}

	if (!fw_vol_tst_abilities(vol, FW_AB_BOOT))
			return -EINVAL;

	/* Disable all of boot actived volume(Should only find one volume) */

	do {
		boot_vol = (struct fw_vol *)fw_get_vol_by_ability_enabled(
					(struct fw_ctx *)ctx, FW_AB_BOOT, FW_BOOT_ACT);

		if (!boot_vol)
			break;

		fw_vol_clr_abl_active(boot_vol, FW_BOOT_ACT);

	} while (boot_vol != NULL);



	fw_vol_set_abl_active(vol, FW_BOOT_ACT);

	return 0;

err:
	return -ENODEV;
}

int32_t libfw_volume_associate_dependent(void *ctx, const char *name, const char *dep)
{
	struct fw_vol *vol = NULL;
	struct fw_vol *dep_vol = NULL;


	vol = (struct fw_vol *)fw_get_vol_by_name((struct fw_ctx *)ctx, name);
	if (!vol) {
		goto err;
	}

	dep_vol = (struct fw_vol *)fw_get_vol_by_name((struct fw_ctx *)ctx, dep);
	if (!dep_vol) {
		goto err;
	}

	fw_vol_set_depdent(vol, dep_vol);

	return 0;
err:
	return -ENODEV;
}

/*
 * 'Board' related API
 */
int32_t libfw_board_set_boot_from(const char *volume)
{
	int ret = -ENODEV;
	struct fw_ctx *bd_ctx = (struct fw_ctx *)fw_get_board_ctx();
	if (bd_ctx == NULL)
		return -ENODEV;

	ret = libfw_set_volume_bootactive((void *)bd_ctx, volume);

	if (ret == 0) {
		fw_ctx_save_to_flash(bd_ctx);
		fw_board_ctx_reinit();

	}
	return ret;
}

char *libfw_board_get_boot_volume(void)
{
	char *name = NULL;
	struct fw_vol *boot = NULL;
	struct fw_ctx *bd_ctx = (struct fw_ctx *)fw_get_board_ctx();
	if (bd_ctx == NULL)
		return NULL;

	boot = fw_get_vol_by_ability_enabled(bd_ctx, FW_AB_BOOT, FW_BOOT_ACT);
	if (!boot) {
		return NULL;
	}

	name = (char *)malloc(sizeof(boot->info->name));
	if (!name)
		return NULL;
	memset(name, 0, sizeof(boot->info->name));
	memcpy((void *)name, (void *)&boot->info->name[0], sizeof(boot->info->name));

	return name;
}

char *libfw_board_volume_to_dev(const char *volume)
{
	char *dev = NULL;
	struct fw_ctx *bd_ctx = (struct fw_ctx *)fw_get_board_ctx();
	if (bd_ctx == NULL)
		return NULL;

	dev = volume_name_to_dev((void *)bd_ctx, volume, FW_VOL_CONTENT_INUSE);

	return dev;
}

char *libfw_board_get_part_list(void)
{
	struct fw_ctx *bd_ctx = (struct fw_ctx *)fw_get_board_ctx();
	if (bd_ctx == NULL)
		return NULL;

	return fw_get_part_list(bd_ctx);
}

void libfw_board_dump(void)
{
	struct fw_ctx *bd_ctx = (struct fw_ctx *)fw_get_board_ctx();
	if (bd_ctx == NULL)
		return;

	fw_dump(bd_ctx);
	return;
}
/*
 * 'Updater' related API
 */
int32_t libfw_updater_open_volume(void *ctx, const char *name, uint32_t flags)
{
	return fw_open_volume((struct fw_ctx *)ctx, name, flags);
}

int64_t libfw_updater_write_volume(void *ctx, void *buff, uint64_t buf_len)
{
	return fw_write_volume((struct fw_ctx *)ctx, buff, buf_len);
}

int64_t libfw_updater_read_volume(void *ctx, void *buff, uint64_t buf_len)
{
	return fw_read_volume((struct fw_ctx *)ctx, buff, buf_len);
}

void libfw_updater_close_volume(void *ctx)
{
	fw_close_volume((struct fw_ctx *)ctx);
	return;
}

#if !defined(__KERNEL__)
int32_t libfw_updater_update_bootloader(void *ctx, const char *image)
{
	return fw_update_bootloader((struct fw_ctx *)ctx, image);
}

void * libfw_updater_init(const char *file)
{
	return (void *)fw_init(file);
}

void libfw_updater_deinit(void *ctx)
{
	fw_deinit((struct fw_ctx *)ctx);
}
#endif


/*
 * Firmware info 'Builder' related API
 */
#if !defined(__KERNEL__)
void *libfw_builder_init(const char *file)
{
	if (file == NULL)
		return NULL;

	return (void *)fw_ctx_init_from_file(file);
}
#endif

void *libfw_builder_bare_init(void)
{
	return (void *)fw_ctx_alloc();
}

void libfw_builder_deinit(void *ctx)
{
	if (ctx == NULL)
		return;

	fw_ctx_free((struct fw_ctx *)ctx);
	return;
}

#if !defined(__KERNEL__)
int32_t libfw_builder_save_info(void *ctx, const char *file)
{
	if (ctx == NULL || file == NULL )
		return -EINVAL;

	return fw_ctx_save_to_file((struct fw_ctx *)ctx, file);
}
#endif

char *libfw_builder_get_part_list(void *ctx)
{
	if (ctx == NULL)
		return NULL;

	return fw_get_part_list((struct fw_ctx *)ctx);
}

int32_t libfw_builder_add_volume(void *ctx, const char *name,
			const char *fs, uint64_t sz, uint32_t nr_parts, uint32_t sig)
{
	int ret = -1;
	struct fw_vol *vol = NULL;

	vol = (struct fw_vol *)fw_alloc_vol(ctx, name, sz, nr_parts);
	if (!vol) {
		goto err;
	}

	fw_vol_set_fs(vol, fs);
	if (sig)
		fw_vol_set_signature_en(vol);


	ret = fw_add_volume(ctx, vol);

	return ret;
err:
	return -ENOMEM;
}

void libfw_builder_dump(void *ctx)
{
	fw_dump((struct fw_ctx *)ctx);
	return;
}

