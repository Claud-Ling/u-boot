/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <fs.h>
#include <fiptool.h>
#include <firmware_image_package.h>
#include <tbbr_config.h>

#define BLK_SIZE_ALIGN(s) ((((s - 1) >> 9) + 1) << 9)

#define pr_err(fmt...)  printf("error: "fmt)
#define pr_warn(fmt...) printf("warn:  "fmt)
#define pr_info(fmt...) printf("info:  "fmt)

static fip_image_t *fip_images[MAX_IMAGES];
static size_t nr_images;
static struct uuid uuid_null = { 0 };
static int fip_id;
static uint64_t fip_start[2] = {2 << 20, 4 << 20};
static void *image_addr;
static uint64_t image_len;

typedef struct flash_ops
{
	int (*load_fip)(void *addr, u32 ofs, u32 len);
	int (*write_fip)(void *addr, u32 ofs, u32 len);
}flash_ops_t;

#if defined CONFIG_TRIX_MMC
static int mmc_load_fip(void *addr, u32 ofs, u32 len)
{
	int ret = 0;
	int flag = 0;
	char cmd_buffer[64];

	sprintf(cmd_buffer, "mmc read %lx %x %x", (unsigned long)addr, ofs >> 9, len >> 9);
	ret = run_command(cmd_buffer, flag);
	return (ret == 0) ? len : 0;
}

static int mmc_write_fip(void *addr, u32 ofs, u32 len)
{
	int ret = 0;
	int flag = 0;
	char cmd_buffer[64];

	sprintf(cmd_buffer, "mmc write %lx %x %x", (unsigned long)addr, ofs >> 9, len >> 9);
	ret = run_command(cmd_buffer, flag);
	return (ret == 0) ? len : 0;
}
#elif defined CONFIG_TRIX_NAND
static int nand_load_fip(void *addr, u32 ofs, u32 len)
{
	int ret = 0;
	int flag = 0;
	char cmd_buffer[64];

	sprintf(cmd_buffer, "nand read %lx %x %x", (unsigned long)addr, ofs >> 9, len >> 9);
	ret = run_command(cmd_buffer, flag);
	return (ret == 0) ? len : 0;
}

static int nand_write_fip(void *addr, u32 ofs, u32 len)
{
	int ret = 0;
	int flag = 0;
	char cmd_buffer[64];

	sprintf(cmd_buffer, "nand write %lx %x %x", (unsigned long)addr, ofs >> 9, len >> 9);
	ret = run_command(cmd_buffer, flag);
	return (ret == 0) ? len : 0;
}
#endif

static flash_ops_t f_ops = {
#if defined CONFIG_TRIX_MMC
	mmc_load_fip,
	mmc_write_fip
#elif defined CONFIG_TRIX_NAND
	nand_load_fip,
	nand_write_fip
#endif
};

static uint64_t get_fip_size(void)
{
	fip_toc_header_t *toc_header;
	fip_toc_entry_t *toc_entry;
	uint64_t header_size, payload_size;
	int i;

	header_size = sizeof(fip_toc_header_t) + sizeof(fip_toc_entry_t) * MAX_IMAGES;
	header_size = BLK_SIZE_ALIGN(header_size);
	toc_header = (fip_toc_header_t *)malloc(header_size);
	if (toc_header == NULL)
	{
		pr_err("malloc %lld bytes for fip header failed\n", header_size);
		return -1;
	}
	f_ops.load_fip(toc_header, fip_start[fip_id], header_size);

	toc_entry = (fip_toc_entry_t *)(toc_header + 1);
	payload_size = 0;
	for (i = 0; i < MAX_IMAGES; i++)
	{
		if(memcmp(&toc_entry->uuid, &uuid_null, sizeof(struct uuid)) != 0)
		{
			payload_size += toc_entry->size;
			toc_entry++;
		}
		else
			break;
	}
	free(toc_header);
	return (sizeof(fip_toc_header_t) + sizeof(fip_toc_entry_t) * (i + 1) + payload_size);
}

static void add_image(fip_image_t *image)
{
	if (nr_images + 1 > MAX_IMAGES)
		pr_err("Too many images\n");
	fip_images[nr_images++] = image;
}

static void free_image(fip_image_t *image)
{
	if(image->buffer != (char *)image_addr)
		free(image->buffer);
	free(image);
}

static void replace_image(fip_image_t *image_dst, fip_image_t *image_src)
{
	int i;

	for (i = 0; i < nr_images; i++) {
		if (fip_images[i] == image_dst) {
			free_image(fip_images[i]);
			fip_images[i] = image_src;
			break;
		}
	}
	assert(i != nr_images);
}

static void free_images(void)
{
	toc_entry_t *toc_entry = toc_entries;
	int i;
	for (i = 0; i < nr_images; i++) {
		free_image(fip_images[i]);
		fip_images[i] = NULL;
	}
	nr_images = 0;

	for (; toc_entry->cmdline_name != NULL; toc_entry++)
	{
		toc_entry->image = NULL;
		toc_entry->action = 0;
		toc_entry->action_arg = NULL;
	}
}

static toc_entry_t *get_entry_lookup_from_uuid(const struct uuid *uuid)
{
	toc_entry_t *toc_entry = toc_entries;

	for (; toc_entry->cmdline_name != NULL; toc_entry++)
		if (memcmp(&toc_entry->uuid, uuid, sizeof(struct uuid)) == 0)
			return toc_entry;
	return NULL;
}

static int parse_fip(u32 ofs, fip_toc_header_t *toc_header_out)
{
	char *buf, *bufend;
	fip_toc_header_t *toc_header;
	fip_toc_entry_t *toc_entry;
	fip_image_t *image;
	int terminated = 0;
	uint64_t fip_size, buf_size;

	if((fip_size = get_fip_size()) < 0)
	{
		pr_err("get fip size failed\n");
		return -1;
	}
	buf_size = BLK_SIZE_ALIGN(fip_size);
	buf = (char *)malloc(buf_size);
	if(buf == NULL)
	{
		pr_err("malloc %lld bytes for fip failed\n", buf_size);
		return -1;
	}

	if(!f_ops.load_fip(buf, ofs, buf_size))
	{
		pr_err("load fip from mmc failed\n");
		goto error;
	}

	bufend = buf + fip_size;
	toc_header = (fip_toc_header_t *)buf;
	toc_entry = (fip_toc_entry_t *)(toc_header + 1);

	if (toc_header->name != TOC_HEADER_NAME)
	{
		pr_err("wrong FIP file\n");
		goto error;
	}

	/* Return the ToC header if the caller wants it. */
	if (toc_header_out != NULL)
		*toc_header_out = *toc_header;

	/* Walk through each ToC entry in the file. */
	while ((char *)toc_entry + sizeof(*toc_entry) - 1 < bufend) {
		/* Found the ToC terminator, we are done. */
		if (memcmp(&toc_entry->uuid, &uuid_null, sizeof(struct uuid)) == 0) {
			terminated = 1;
			break;
		}

		/*
		 * Build a new image out of the ToC entry and add it to the
		 * table of images.
		 */
		image = malloc(sizeof(fip_image_t));
		if (image == NULL)
		{
			pr_err("malloc %d bytes for image structure failed\n", sizeof(fip_image_t));
			goto error;
		}

		memcpy(&image->uuid, &toc_entry->uuid, sizeof(struct uuid));
		image->buffer = malloc(toc_entry->size);
		if (image->buffer == NULL)
		{
			pr_err("malloc %lld bytes for image buffer failed\n", toc_entry->size);
			goto error;
		}
		/* Overflow checks before memory copy. */
		if (toc_entry->size > (uint64_t)-1 - toc_entry->offset_address)
		{
			pr_err("FIP is corrupted\n");
			goto error;
		}
		if (toc_entry->size + toc_entry->offset_address > fip_size)
		{
			pr_err("FIP is corrupted\n");
			goto error;
		}
		memcpy(image->buffer, buf + toc_entry->offset_address,
		    toc_entry->size);
		image->size = toc_entry->size;

		image->toc_entry = get_entry_lookup_from_uuid(&toc_entry->uuid);
		if (image->toc_entry == NULL) {
			add_image(image);
			toc_entry++;
			continue;
		}

		assert(image->toc_entry->image == NULL);
		/* Link backpointer from lookup entry. */
		image->toc_entry->image = image;
		add_image(image);

		toc_entry++;
	}

	if (terminated == 0)
	{
		pr_err("FIP does not have a ToC terminator entry\n");
		goto error;
	}

	free(buf);
	return 0;

error:
	free(buf);
	return -1;
}

static fip_image_t *read_image_from_file(toc_entry_t *toc_entry, char *file_name)
{
	const char *ifname = "usb";
	const char *dev_part = "0";
	int fstype = 1;
	char *buf;
	fip_image_t *image;
	loff_t len;
	loff_t file_size;

	if(fs_set_blk_dev(ifname, dev_part, fstype))
		return NULL;
	if(fs_size(file_name, &file_size) < 0)
	{
		pr_err("get size of %s failed\n", file_name);
		return NULL;
	}
	buf = (char *)malloc(file_size);
	if(buf == NULL)
	{
		pr_err("malloc %lld bytes for file failed\n", file_size);
		return NULL;
	}

	if(fs_set_blk_dev(ifname, dev_part, fstype))
		return NULL;
	if(((fs_read(file_name, (unsigned long)buf, 0, 0, &len)) < 0) || (len != file_size))
	{
		pr_err("load %s from USB failed\n", file_name);
		goto error;
	}

	image = malloc(sizeof(*image));
	if (image == NULL)
	{
		pr_err("malloc %d bytes for image structure failed\n", sizeof(*image));
		goto error;
	}
	memcpy(&image->uuid, &toc_entry->uuid, sizeof(struct uuid));

	image->buffer = (char *)buf;
	image->size = len;
	image->toc_entry = toc_entry;

	return image;

error:
	free(buf);
	return NULL;
}

static fip_image_t *read_image_from_ddr(toc_entry_t *toc_entry)
{
	fip_image_t *image;

	image = malloc(sizeof(*image));
	if (image == NULL)
	{
		pr_err("malloc %d bytes for image structure failed\n", sizeof(*image));
		return NULL;
	}
	memcpy(&image->uuid, &toc_entry->uuid, sizeof(struct uuid));

	image->buffer = (char *)image_addr;
	image->size = image_len;
	image->toc_entry = toc_entry;

	return image;
}

static int pack_images(void)
{
	fip_image_t *image;
	fip_toc_header_t *toc_header;
	fip_toc_entry_t *toc_entry;
	char *buf, *fip, *p;
	uint64_t entry_offset, buf_size, payload_size;
	int i;

	/* Calculate total payload size and allocate scratch buffer. */
	payload_size = 0;
	for (i = 0; i < nr_images; i++)
		payload_size += fip_images[i]->size;

	buf_size = sizeof(fip_toc_header_t) +
	    sizeof(fip_toc_entry_t) * (nr_images + 1);
	buf = (char *)malloc(buf_size);
	if (buf == NULL)
	{
		pr_err("malloc %lld bytes for toc header failed\n", buf_size);
		return -1;
	}

	/* Build up header and ToC entries from the image table. */
	toc_header = (fip_toc_header_t *)buf;
	toc_header->name = TOC_HEADER_NAME;
	toc_header->serial_number = TOC_HEADER_SERIAL_NUMBER;

	toc_entry = (fip_toc_entry_t *)(toc_header + 1);

	entry_offset = buf_size;
	for (i = 0; i < nr_images; i++) {
		image = fip_images[i];
		memcpy(&toc_entry->uuid, &image->uuid, sizeof(struct uuid));
		toc_entry->offset_address = entry_offset;
		toc_entry->size = image->size;
		toc_entry->flags = 0;
		entry_offset += toc_entry->size;
		toc_entry++;
	}

	/* Append a null uuid entry to mark the end of ToC entries. */
	memcpy(&toc_entry->uuid, &uuid_null, sizeof(struct uuid));
	toc_entry->offset_address = entry_offset;
	toc_entry->size = 0;
	toc_entry->flags = 0;

	/* Generate the FIP file. */
	fip = (char *)malloc(buf_size + payload_size);
	if(fip == NULL)
	{
		pr_err("malloc %lld bytes for fip failed\n", buf_size + payload_size);
		goto error;
	}

	p = fip;
	memcpy(p, buf, buf_size);
	p += buf_size;

	for (i = 0; i < nr_images; i++) {
		image = fip_images[i];
		memcpy(p, image->buffer, image->size);
		p += image->size;
	}

	f_ops.write_fip(fip, fip_start[fip_id], BLK_SIZE_ALIGN(buf_size + payload_size));
	free(buf);
	free(fip);
	return 0;
error:
	free(buf);
	return -1;
}

/*
 * This function is shared between the create and update subcommands.
 * The difference between the two subcommands is that when the FIP file
 * is created, the parsing of an existing FIP is skipped.  This results
 * in update_fip() creating the new FIP file from scratch because the
 * internal image table is not populated.
 */
static int update_fip(void)
{
	toc_entry_t *toc_entry;
	fip_image_t *image;

	/* Add or replace images in the FIP file. */
	for (toc_entry = toc_entries;
	     toc_entry->cmdline_name != NULL;
	     toc_entry++) {
		if (toc_entry->action != DO_PACK)
			continue;
		if(toc_entry->action_arg != NULL)
			image = read_image_from_file(toc_entry, toc_entry->action_arg);
		else
			image = read_image_from_ddr(toc_entry);
		if(image == NULL)
		{
			pr_err("read image from file failed\n");
			return -1;
		}
		if (toc_entry->image != NULL) {
			if(toc_entry->action_arg != NULL)
				pr_info("Replacing image %s.bin with %s\n", toc_entry->cmdline_name, toc_entry->action_arg);
			else
				pr_info("Replacing image %s.bin with %lld bytes data from %p\n", toc_entry->cmdline_name, image_len, image_addr);
			replace_image(toc_entry->image, image);
		} else {
			pr_info("Adding image %s\n", toc_entry->action_arg);
			add_image(image);
		}
		/* Link backpointer from lookup entry. */
		toc_entry->image = image;
	}

	return 0;
}

static int do_fipupd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	fip_toc_header_t toc_header = {0};
	toc_entry_t *toc_entry;
	char *img_name = NULL;
	char *file_name = NULL;

	if((argc != 4) && (argc != 5))
		return CMD_RET_USAGE;

	fip_id = (int)simple_strtoul(argv[1], NULL, 10);
	if((fip_id != 0) && (fip_id != 1))
		return CMD_RET_USAGE;
	img_name = argv[2];

	if(argc == 4)
		file_name = argv[3];
	else
	{
		image_addr = (void *)simple_strtoul(argv[3], NULL, 16);
		image_len = simple_strtoul(argv[4], NULL, 10);
	}

	for (toc_entry = toc_entries;
	     toc_entry->cmdline_name != NULL;
	     toc_entry++) {
		if(!strncmp(toc_entry->cmdline_name, img_name, strlen(toc_entry->cmdline_name)))
		{
			toc_entry->action = DO_PACK;
			toc_entry->action_arg = file_name;
			break;
		}
	}
	if(toc_entry->cmdline_name == NULL)
	{
		pr_err("invalid image name\n");
		return CMD_RET_USAGE;
	}

	if(parse_fip(fip_start[fip_id], &toc_header) != 0)
		goto error;
	if(update_fip() != 0)
		goto error;
	if(pack_images() != 0)
		goto error;

	free_images();
	return CMD_RET_SUCCESS;
error:
	free_images();
	return CMD_RET_FAILURE;
}

static int do_fipinfo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	fip_image_t *image;
	uint64_t image_offset;
	uint64_t image_size = 0;
	fip_toc_header_t toc_header;
	int i;

	if (argc != 2)
		return CMD_RET_USAGE;

	fip_id = (int)simple_strtoul(argv[1], NULL, 10);
	if((fip_id != 0) && (fip_id != 1))
		return CMD_RET_USAGE;

	if(parse_fip(fip_start[fip_id], &toc_header) != 0)
		return CMD_RET_FAILURE;

	image_offset = sizeof(fip_toc_header_t) +
	    (sizeof(fip_toc_entry_t) * (nr_images + 1));

	for (i = 0; i < nr_images; i++) {
		image = fip_images[i];
		if (image->toc_entry != NULL)
			printf("%s: ", image->toc_entry->name);
		else
			printf("Unknown entry: ");
		image_size = image->size;
		printf("offset=0x%llX, size=0x%llX",
		    (unsigned long long)image_offset,
		    (unsigned long long)image_size);
		if (image->toc_entry != NULL)
			printf(", cmdline=\"--%s\"",
			    image->toc_entry->cmdline_name);
		printf("\n");
		image_offset += image_size;
	}

	free_images();
	return CMD_RET_SUCCESS;
}

static cmd_tbl_t cmd_fip[] = {
	U_BOOT_CMD_MKENT(info, 2, 0, do_fipinfo, "", ""),
	U_BOOT_CMD_MKENT(update, 5, 0, do_fipupd, "", ""),
};

static int do_fipops(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *cp;

	cp = find_cmd_tbl(argv[1], cmd_fip, ARRAY_SIZE(cmd_fip));

	/* Drop the fip command */
	argc--;
	argv++;

	if (cp == NULL || argc > cp->maxargs)
		return CMD_RET_USAGE;

	return cp->cmd(cmdtp, flag, argc, argv);
}

U_BOOT_CMD(
  fip,  29, 1,  do_fipops,
	"FIP sub system",
	"info <id> - List images contained in FIPn\n"
	"fip update <id> <image> <[file]|[addr] [len]> - update the 'image' in FIPn with the 'file' in USB stick or with the 'len' bytes data from DDR 'addr'\n"
	"    tb-fw  (BL2)\n"
	"    soc-fw (BL31)\n"
	"    scp-fw (SCP_BL2, mcu)\n"
	"    tos-fw (BL32, trusted OS)\n"
	"    nt-fw  (BL33)\n"
	"    Note: FIP[0] range from 2M-4M, FIP[1] range from 4M-6M\n"
);

