#ifndef __LIBFW_H__
#define __LIBFW_H__
#if !defined(__KERNEL__)
#include <stdint.h>
#endif

/************************************************/
/*		General related API		*/
/************************************************/

/*
 * descriptions:
 *    Set a volume have bootable ability
 *
 * parameters:
 *    ctx  [in]: firmware info context point
 *    name [in]: volume name
 *
 * return:
 *    Success: 0
 *    Failed: Negative value
 */
extern int32_t libfw_set_volume_bootable(void *ctx, const char *name);

/*
 * descriptions:
 *    Set a volume with bootable ability active
 *    	This make this volume as primary boot volume
 *
 * parameters:
 *    ctx  [in]: firmware info context point
 *    name [in]: volume name
 *
 * return:
 *    Success: 0
 *    Failed: Negative value
 */
extern int32_t libfw_set_volume_bootactive(void *ctx, const char *name);

/*
 * descriptions:
 *    Set a volume depent on another volume
 *
 * parameters:
 *    ctx  [in]: firmware info context point
 *    name [in]: name of volume need depend on another volume
 *    dep [in]: name of volume
 *
 * return:
 *    Success: 0
 *    Failed: Negative value
 */
extern int32_t libfw_volume_associate_dependent(void *ctx, const char *name, const char *dep);


/************************************************/
/*		Board related API		*/
/************************************************/

/*
 * descriptions:
 *    Get boot active volume name
 *
 * parameters:
 *    Non
 *
 * return:
 *    Success: valid string point for volume name
 *    Failed: NULL
 */
extern char *libfw_board_get_boot_volume(void);

/*
 * descriptions:
 *    Set board boot from specified volume
 *
 * parameters:
 *    volume [in]: volume name
 *
 * return:
 *    Success: 0
 *    Failed: Negative value
 */
extern int32_t libfw_board_set_boot_from(const char *volume);

/*
 * descriptions:
 *    Translation a volume name to its active part device node
 *
 * parameters:
 *    volume [in]: volume name
 *
 * return:
 *    Success: valid string  point for device node
 *    Failed: NULL
 */
extern char *libfw_board_volume_to_dev(const char *volume);

/*
 * descriptions:
 *    Get partition layout(as kernel command line) string buffer
 *   from firmware info stored in board
 *
 * parameters:
 *    Non
 *
 * return:
 *    Success: valid partition string  point
 *    Failed: NULL
 */
extern char *libfw_board_get_part_list(void);

/*
 * descriptions:
 *    Dump board firmware info content
 *    	if no valid firmware info in board flash, nothing can output
 *
 * parameters:
 *    Non
 *
 * return:
 *    Non
 */
extern void libfw_board_dump(void);





/************************************************/
/*		Updater related API		*/
/************************************************/

/*
 * descriptions:
 *    Init firmware info context for updater instance
 *
 * parameters:
 *    file [in]: firmware info file location
 *
 * return:
 *    Success: valid context point
 *    Failed: NULL
 */
extern void * libfw_updater_init(const char *file);

/*
 * descriptions:
 *    Deinit firmware info context
 *    	This will save the changes for context and firmware info
 *    will affect for next time boot up.
 *
 * parameters:
 *    ctx [in]: firmware info context for release
 *
 * return:
 *    Non
 */
extern void libfw_updater_deinit(void *ctx);

/*
 * descriptions:
 *    open a volume
 *
 * parameters:
 *    ctx [in]: firmware info context
 *    name [in]: name of volume which want to open
 *    flags [in]: specify volume content want to access
 *    		VOL_CONTENT_ACTIVE
 *    				Active data of volume, which typically
 *    			have valid data for this volume
 *
 *    		VOL_CONTENT_IDLE
 *    				Idle storage region of volume, the region
 *    			is not used by this volume can safely write.
 *
 * return:
 *    Success: 0
 *    Failed: Negative value.
 */
enum {
	VOL_CONTENT_ACTIVE = 0,
	VOL_CONTENT_IDLE = 1,
};
extern int32_t libfw_updater_open_volume(void *ctx, const char *name, uint32_t flags);

/*
 * descriptions:
 *    Write data to a volume
 *    		Typically, write operation should only can access volume 'IDLE' region
 *    	and write operation can change the 'IDLE' region into 'ACTIVE' statue after write
 *    	operation done.
 *
 * parameters:
 *    ctx [in]: firmware info context.
 *
 *    buff[in]: data want to write
 *
 *    buf_len[in]:data length
 *
 * return:
 *    Success: Practical bytes be writen down to volume
 *    Failed: Negative value
 */
extern int64_t libfw_updater_write_volume(void *ctx, void *buff, uint64_t buf_len);

/*
 * descriptions:
 *    Read data from a volume
 *
 * parameters:
 *    ctx [in]: firmware info context.
 *
 *    buff[out]: data want to read
 *
 *    buf_len[in]:data length
 *
 * return:
 *    Success: Practical bytes be read from volume
 *    Failed: Negative value
 */
extern int64_t libfw_updater_read_volume(void *ctx, void *buff, uint64_t buf_len);

/*
 * descriptions:
 *    Close a volume instance
 *
 * parameters:
 *    ctx [in]: firmware info context
 *
 * return:
 *    non
 */
extern void libfw_updater_close_volume(void *ctx);

/*
 * descriptions:
 *    Update bootloader
 *
 * parameters:
 *    ctx [in]: firmware info context.
 *
 *    image[in]: bootloader binary for update
 *
 * return:
 *    Success: 0
 *    Failed: Negative value
 */
extern int32_t libfw_updater_update_bootloader(void *ctx, const char *image);


/************************************************
	firmware info builder related API
************************************************/

/*
 * descriptions:
 *    Init firmware info context for builder instance
 *    This API mainly purpose is for already have firmware info binary
 *    user want to modify the firmware info, use this API to restore context
 *    for exist firmware info
 *
 * parameters:
 *    file [in]: firmware info file location
 *
 * return:
 *    Success: valid context point
 *    Failed: NULL
 */
extern void *libfw_builder_init(const char *file);

/*
 * descriptions:
 *    Init firmware info context from bare.
 *    This will allocate a bare firmware info context
 *
 * parameters:
 *    Non
 *
 * return:
 *    Success: valid context point
 *    Failed: NULL
 */
extern void *libfw_builder_bare_init(void);

/*
 * descriptions:
 *    Release firmware info context
 *
 * parameters:
 *    ctx [in]: firmware info context point
 *
 * return:
 *    Non
 */

extern void libfw_builder_deinit(void *ctx);

/*
 * descriptions:
 *    Save firmware info context to a binary file
 *
 * parameters:
 *    ctx  [in]: context point want to save as file
 *    file [in]: firmware info binary location
 *
 * return:
 *    Success: 0
 *    Failed: Negative value
 */
extern int32_t libfw_builder_save_info(void *ctx, const char *file);

/*
 * descriptions:
 *    Get partition layout(as kernel command line) string buffer
 *
 * parameters:
 *    ctx [in]: firmware info context
 *
 * return:
 *    Success: valid partition string  point
 *    Failed: NULL
 */
extern char *libfw_builder_get_part_list(void *ctx);

/*
 * descriptions:
 *    Add a volume to firmware info
 *
 * parameters:
 *    ctx [in]: firmware info context
 *    name [in]: volume name
 *    fs [in]: filesyste name for this volume
 *    sz [in]: volume size
 *    nr_parts [in]: Number of storage parts this volume have
 *    	at leasst should be '1'.
 *    	if user set to n, the practical size on flash for this volume
 *    	will be 'n * sz'
 *
 *    sig [in]: indicate this volume need signature check before using this volume
 *    		1: need signatrue check
 *    		0: no need signature check
 *
 * return:
 *    Success: 0
 *    Failed: Negative value
 */
extern int32_t libfw_builder_add_volume(void *ctx, const char *name,
			const char *fs, uint64_t sz, uint32_t nr_parts, uint32_t sig);

/*
 * descriptions:
 *    Dump firmware info content with specified context
 *
 * parameters:
 *    ctx [in]: firmware info context
 *
 * return:
 *    Non
 */
extern void libfw_builder_dump(void *ctx);


#endif /* __LIBFW_H__ */
