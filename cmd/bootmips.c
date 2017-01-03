#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/arch/reg_io.h>
#include <malloc.h>
#include <mipsimg.h>
#ifdef CONFIG_CMD_EXT4
#include <fs.h>
#include <image.h>
#include <u-boot/md5.h>
#endif

//RGU for sx6/sx7/..., refer to RGU SPG
#define TRIX_RGU_CTRLREG		0x1500ee80
#define TRIX_RGU_AV_RESET_MASK		(1 << 5)
#define TRIX_RGU_DISP_RESET_MASK	(1 << 6)

//Vector register for av/disp, refer to SPG
#define AV_MIPS_START_ADDR_REG          0x100140dc
#define DISP_MIPS_START_ADDR_REG        0x194c00d8

//memfile
#define MAX_MEM_FILE_SIZE	0x2000	//maximum 8k
#define LENGHTH_SIZE		4
#define MEM_FILE_OFFSET		0x1000

/* detect host ending: 0-LE, 1-BE */
#define GET_ENDING() ({                 \
		union{int i;char c;} _t;\
		_t.i = 0x12345678;      \
		(_t.c == 0x12) ? 1 : 0; \
		})

/* swap int ending */
#define SWAP_32(v) ((((v)&0xFF)<<24)    \
		| ((((v)>>8)&0xFF)<<16) \
		| ((((v)>>16)&0xFF)<<8) \
		| (((v)>>24)&0xFF))
/* swap short ending */
#define SWAP_16(v) ((((v)&0xFF)<<8)     \
		| (((v)>>8)&0xFF))

#define getenv_yesno(v) ({char* _s=getenv(v);(_s && (*_s == 'n')) ? 0 : 1;})	/* default true */
#define getenv_yes(v) ({char* _s=getenv(v);(_s && (*_s == 'y')) ? 1 : 0;})	/* default false */

#define pr_err(fmt...)  printf("error: "fmt)
#define pr_warn(fmt...) printf("warn:  "fmt)
#define pr_info(fmt...) printf("info:  "fmt)

typedef enum _tagSdBootDevice{
	BOOTDEV_DDR=0,	/* dram memory */
	BOOTDEV_MMC,	/* mmc flash */
	BOOTDEV_NAND,	/* nand flash */

	BOOTDEV_INVAL = -1,
}SdBootDev_t;

typedef enum _tagSdImgPayload{
	IMG_AVMIPS = 0,	/* av-mips image */
	IMG_DISPMIPS,	/* disp-mips image */

	IMG_DB = 100,	/* database image */
	IMG_SOUND,	/* sound image */
	IMG_DATA,	/* generic data image */
	IMG_CFG,	/* setting_cfg img */
	IMG_ST,		/* initsetting img */
	IMG_LOGO,	/* logo img */
	IMG_AUDIO, /* audio img */

	IMG_INVAL = -1,
}SdImgPayload_t;

typedef struct _tagSdImgDesc{
	SdImgPayload_t id;
	const char* name;
}SdImgDesc_t;

typedef struct _tagSdMipsBootDev{
	SdBootDev_t id;
	int (*get_header)(void *load_addr, int load_size, mips_img_hdr* hdr);
	u32 (*get_loadaddr)(void *img, u32 len, u32 addr);
	int (*load_image)(void* img, u32 len, void* dest);
	void* (*load_memfile)(void *img, u32 len, void *buff, u32 blen);
	void* (*load_initsettings)(void *img, u32 len);
	void (*withdraw)(void);
}SdMipsBootDev_t;

typedef int (*loadfunc_t)(void* img, u32 len, void* dest);

#define BOOT_DEVICE(nm) bootdev_##nm
#define DECLARE_BOOT_DEVICE(nm, id, get_hdr, get_la, 	\
			ld_img, ld_mf, ld_st, wd)	\
	static SdMipsBootDev_t BOOT_DEVICE(nm) = {(id),	\
		(get_hdr),(get_la), (ld_img), (ld_mf), 	\
		(ld_st), (wd)}

static SdImgDesc_t imgdescs[] =
{
	{IMG_AVMIPS,	"avdecoder.bin"},
	{IMG_DISPMIPS,	"display.bin"},
	{IMG_DB,	"database.TSE"},
	{IMG_SOUND,	"sound_preset.bin"},
	{IMG_DATA,	"data"},
	{IMG_CFG,	"setting_cfg.xml"},
	{IMG_ST,	"initsetting.file"},
	{IMG_LOGO,	"logo.bin"},
	{IMG_AUDIO,  "audio_boot.bin"},
	{IMG_INVAL,	NULL},
};


#define GetImgName(_id) ({				\
	int _i;	const char*_ret="";			\
	for (_i=0;imgdescs[_i].id!=IMG_INVAL;_i++)	\
		if (imgdescs[_i].id == (_id)){		\
			_ret = imgdescs[_i].name;	\
			break;				\
		}					\
	_ret;						\
})

#ifdef CONFIG_CMD_EXT4
#define MD5SUM_LEN 32
typedef struct _tagMetaData{
	char version[16];
	char md5sum[MD5SUM_LEN];
}MetaData_t;
#endif

static unsigned int GetLoadAddr(const void* buf, const int len)
{
        const char *Tag = NULL;
        unsigned int LoadAddr = 0;

	if( !buf || len < 12)
		return 0;

	Tag = (const char*)buf;
	while ((Tag - (const char*)buf) < len - 11) {
		if (!memcmp(&Tag[0], "Load", 4) && !memcmp(&Tag[8], "Addr", 4)) {   
			LoadAddr += Tag[4];
			LoadAddr <<= 8;
			LoadAddr += Tag[5];
			LoadAddr <<= 8;
			LoadAddr += Tag[6];
			LoadAddr <<= 8;
			LoadAddr += Tag[7];
			break;
		}
		Tag++;
	}

	return LoadAddr;
}

static int stop_mips(SdImgPayload_t id)
{
	u32 ctrl_reg = TRIX_RGU_CTRLREG;
	u32 val, reset_mask = 0;
	if (IMG_AVMIPS == id){
		reset_mask = TRIX_RGU_AV_RESET_MASK;
		val = ReadRegWord(ctrl_reg);
		val &= ~reset_mask;
		WriteRegWord(ctrl_reg, val);
		debug("stop av-mips, %#x -> %#x\n", val, ctrl_reg);
	}else if (IMG_DISPMIPS == id){
		reset_mask = TRIX_RGU_DISP_RESET_MASK;
		val = ReadRegWord(ctrl_reg);
		val &= ~reset_mask;
		WriteRegWord(ctrl_reg, val);
		debug("stop disp-mips, %#x -> %#x\n", val, ctrl_reg);
	}
	return 0;
}

static int start_mips(SdImgPayload_t id, u32 addr)
{
	u32 ctrl_reg = TRIX_RGU_CTRLREG;
	u32 val, reset_mask = 0;
	if (IMG_AVMIPS == id){
		reset_mask = TRIX_RGU_AV_RESET_MASK;
		val = addr & 0x1fffffff;
		val >>= 12;
		val |= 0x40020000;
		WriteRegWord(AV_MIPS_START_ADDR_REG, val);
		debug("%#x -> %#x\n", val, AV_MIPS_START_ADDR_REG);
		val = ReadRegWord(ctrl_reg);
		val |= reset_mask;
		WriteRegWord(ctrl_reg, val);
		debug("start av-mips at 0x%08x, %#x -> %#x\n", addr, val, ctrl_reg);
	}else if (IMG_DISPMIPS == id){
		reset_mask = TRIX_RGU_DISP_RESET_MASK;
		val = addr | 0xa0000000;
		WriteRegWord(DISP_MIPS_START_ADDR_REG, val);
		debug("%#x -> %#x\n", val, DISP_MIPS_START_ADDR_REG);
		val = ReadRegWord(ctrl_reg);
		val |= reset_mask;
		WriteRegWord(ctrl_reg, val);
		debug("start disp-mips at 0x%08x, %#x -> %#x\n", addr, val, ctrl_reg);
	}
	return 0;
}

static int refine_header(mips_img_hdr* hdr)
{
	int i = 0;
	debug("refine byte order for header...\n");
        for (i=0; i<MIPS_PAYLOAD_MAX; i++) {
		hdr->data[i].size = ntohl(hdr->data[i].size);
		hdr->data[i].addr = ntohl(hdr->data[i].addr);
		hdr->data[i].type = ntohl(hdr->data[i].type);
	}
	hdr->page_size = ntohl(hdr->page_size);
	hdr->mcfg_ofs = ntohl(hdr->mcfg_ofs);
	hdr->panelnode_ofs = ntohl(hdr->panelnode_ofs);
	hdr->unused[0] = ntohl(hdr->unused[0]);
	return 0;
}

static u32 ddr_get_loadaddr(void* img, u32 len, u32 addr)
{
	u32 dest = GetLoadAddr(img + len - 12, 12);
	if (0 == dest)
		dest = addr;
	return dest;
}

static int ddr_load_image(void* img, u32 len, void* dest)
{
	memcpy(dest, img, len);
	flush_cache((ulong)dest, len);

	return len;
}

static int ddr_get_header(void *load_addr, int load_size, mips_img_hdr* hdr)
{
	memcpy(hdr, load_addr, sizeof(mips_img_hdr));
	refine_header(hdr);
	return 0;
}

static void* ddr_load_memfile(void *img, u32 len, void *buff, u32 blen)
{
	return img;
}

static void* ddr_load_initsettings(void *img, u32 len)
{
	return img;
}

static void ddr_withdraw(void)
{
	return;
}

DECLARE_BOOT_DEVICE(ddr,
		BOOTDEV_DDR,
		ddr_get_header,
		ddr_get_loadaddr,
		ddr_load_image,
		ddr_load_memfile,
		ddr_load_initsettings,
		ddr_withdraw);


#define ROUND512(l) ROUND(l, 512)
#define ROUND4K(l) ROUND(l, 4096)
static void* dummy_buffer_addr = NULL;
static int dummy_buffer_len  = 0;
static void* get_buffer(int len, int round)
{
	if (dummy_buffer_len < len)
	{
		dummy_buffer_len = 0;
		if (dummy_buffer_addr)
			free(dummy_buffer_addr);
		if ((dummy_buffer_addr = malloc(ROUND(len, round))))
			dummy_buffer_len = ROUND(len, round);
	}
	return dummy_buffer_addr;
}

static void free_buffer(void)
{
	if (dummy_buffer_addr)
		free(dummy_buffer_addr);
	dummy_buffer_addr = NULL;
	dummy_buffer_len = 0;
}

#ifdef CONFIG_CMD_MMC
#define MMC_ISBLKALIGN(a) (!(((int)a) & 0x1FF))
#define MMC_ADDR2BLK(a)	((u32)(((int)a) >> 9))
#define MMC_SIZE2CNT(sz) (((sz) + 511) >> 9)
#define PAGESIZE 512

static int mmc_read_data(void *addr, u32 ofs, u32 len)
{
	int ret = 0;
	int flag = 0;
	char cmd_buffer[64];

	sprintf(cmd_buffer, "mmc read %lx %x %x", (uintptr_t)addr, MMC_ADDR2BLK(ofs), MMC_SIZE2CNT(len));
	ret = run_command(cmd_buffer, flag);
	return (ret == 0) ? len : 0;
}

static u32 mmc_get_loadaddr(void* img, u32 len, u32 addr)
{
	#define GETLOADADDR(loc,len) ({			\
		void* buff = get_buffer(len, PAGESIZE);	\
		mmc_read_data(buff, (u32)loc, len);	\
		GetLoadAddr(buff, len);			\
	})
	#define PAGEADDR(a) ((u32)(a) & (~(PAGESIZE - 1)))
	u32 dest = addr;
	u32 slen = ((u32)img + len) & (PAGESIZE - 1);
	slen += ((slen == 0) || (PAGEADDR(img + len) != PAGEADDR(img + len - 12))) ? PAGESIZE : 0;

	if (0 == (dest = GETLOADADDR((img + len - 12), slen)))
		dest = addr;
	return dest;
	#undef GETLOADADDR
	#undef PAGEADDR
}

static int mmc_load_image(void* img, u32 len, void* dest)
{
	int ret = mmc_read_data(dest, (u32)img, len);
	if (ret > 0)
		flush_cache((ulong)dest, len);
	return ret;
}

static int mmc_get_header(void *load_addr, int load_size, mips_img_hdr* hdr)
{
	void *buffer = NULL;
	u32 rdlen = sizeof(mips_img_hdr);

	/* check load_addr alignment */
	if (!MMC_ISBLKALIGN(load_addr))
	{
		pr_err("Not a mmc block aligned addr:%p!\n", load_addr);
		return 1;
	}

	buffer = get_buffer(rdlen, PAGESIZE);
	mmc_read_data(buffer, (u32)load_addr, rdlen);
	memcpy(hdr, buffer, sizeof(mips_img_hdr));
	refine_header(hdr);
	return 0;
}

static void* mmc_load_memfile(void *img, u32 len, void *buff, u32 blen)
{
	void *ret = NULL;
	if (blen >= ROUND512(len))
	{
		mmc_read_data(buff, (u32)img, len);
		ret = buff;
	}
	return ret;
}

static void* mmc_load_initsettings(void *img, u32 len)
{
	void* buffer = get_buffer(len, PAGESIZE);
	if (buffer)
		mmc_read_data(buffer, (u32)img, len);
	return buffer;
}

static void mmc_withdraw(void)
{
	free_buffer();
}

DECLARE_BOOT_DEVICE(mmc,
		BOOTDEV_MMC,
		mmc_get_header,
		mmc_get_loadaddr,
		mmc_load_image,
		mmc_load_memfile,
		mmc_load_initsettings,
		mmc_withdraw);

#undef MMC_ISBLKALIGN
#undef MMC_ADDR2BLK
#undef MMC_SIZE2CNT
#undef PAGESIZE
#endif /* CONFIG_CMD_MMC */

#ifdef CONFIG_CMD_NAND
#include <nand.h>
#define PAGESIZE nand_info[nand_curr_device].writesize

#ifdef DEBUG
# define CHK_ALIGN(a) do {							\
		if ((u32)(a) & (PAGESIZE - 1))					\
			pr_err("addr(0x%x) is NOT page(0x%x) aligned!",		\
			(u32)(a), PAGESIZE);					\
	}while(0)
#else
# define CHK_ALIGN(a) do{}while(0)
#endif

#define TONANDADDR(a) ({				\
		CHK_ALIGN(a);				\
		(u32)((u32)(a) & ~(PAGESIZE - 1));	\
	})
#define NAND_ISALIGN(a) (!((u32)(a) & (PAGESIZE - 1)))

static int nand_read_data(void *addr, u32 ofs, u32 cnt)
{
	int ret = 0;
	int flag = 0;
	char cmd_buffer[64];

	sprintf(cmd_buffer, "nand read %x %x %x", (unsigned int)addr, TONANDADDR(ofs), cnt);
	ret = run_command(cmd_buffer, flag);
	return (ret==0) ? cnt : 0;
}

static u32 nand_get_loadaddr(void* img, u32 len, u32 addr)
{
	#define GETLOADADDR(loc,len) ({			\
		void* buff = get_buffer(len, PAGESIZE);	\
		nand_read_data(buff, (u32)loc, len);	\
		GetLoadAddr(buff, len);			\
	})
	#define PAGEADDR(a) ((u32)(a) & (~(PAGESIZE - 1)))

	u32 dest = addr;
	u32 slen = ((u32)img + len) & (PAGESIZE - 1);
	slen += (slen == 0) || (PAGEADDR(img + len) != PAGEADDR(img + len - 12)) ? PAGESIZE : 0;

	if (0 == (dest = GETLOADADDR((img + len - 12), slen)))
		dest = addr;
	return dest;
	#undef GETLOADADDR
	#undef PAGEADDR
}

static int nand_load_image(void* img, u32 len, void* dest)
{
	int ret = nand_read_data(dest, (u32)img, len);
	if (ret > 0)
		flush_cache((ulong)dest, len);
	return ret;
}

static int nand_get_header(void *load_addr, int load_size, mips_img_hdr* hdr)
{
	void *buffer = NULL;
	u32 rdlen = sizeof(mips_img_hdr);

	if (!NAND_ISALIGN(load_addr)) {
		pr_err("Not a nand page (0x%x) aligned address (0x%x)\n", PAGESIZE, (u32)load_addr);
		return 1;
	}

	buffer = get_buffer(rdlen, PAGESIZE);
	nand_read_data(buffer, (u32)load_addr, rdlen);
	memcpy(hdr, buffer, sizeof(mips_img_hdr));
	refine_header(hdr);
	return 0;
}

static void* nand_load_memfile(void *img, u32 len, void *buff, u32 blen)
{
	void *ret = NULL;
	if (blen >= len)
	{
		nand_read_data(buff, (u32)img, len);
		ret = buff;
	}
	return ret;
}

static void* nand_load_initsettings(void *img, u32 len)
{
	void* buffer = get_buffer(len, PAGESIZE);
	if (buffer)
		nand_read_data(buffer, (u32)img, len);
	return buffer;
}

static void nand_withdraw(void)
{
	free_buffer();
}

DECLARE_BOOT_DEVICE(nand,
		BOOTDEV_NAND,
		nand_get_header,
		nand_get_loadaddr,
		nand_load_image,
		nand_load_memfile,
		nand_load_initsettings,
		nand_withdraw);

#undef PAGESIZE
#undef CHK_PAGESIZE
#undef CHK_ALIGN
#undef TONANDADDR
#undef NAND_ISALIGN
#endif /* CONFIG_CMD_NAND */

#ifdef CONFIG_DEBUG_MIPS
/*
 * Debug boot MIPS
 *
 * This code provides a method to easily debug mips at uboot basis, which
 * is very helpful when bootmips is on.
 *
 * The main idea is to offer a generic method for users to replace certain
 * mips related images through usb devices at run time. The usb should be
 * formatted in FAT32 or other compatile mode per design. A debug directory
 * is defined in usb device, that is "debug/mipsimg". People shall copy
 * images they intend to debug to a usb stick under mentioned debug directory,
 * while each image shall be set with a specified name which will be further
 * talked below, connect that usb stick to target board at proper connector,
 * and then reboot set. while uboot will search mips images from usb device
 * under this debug directory before boot mips, by naming. All images in usb
 * stick take precedence to the ones stored in flash memory when boot mips.
 *
 * This function is locked by an uboot env variable 'debugmips' and it's
 * OFF by default in respect to security and quick boot aspects. While
 * people is allowed to turn on (off) it whenever debugging is (not) expected
 * , if only a prompts is granted for either uboot or kernel.
 *
 *     @@ turn on mips debug in uboot prompts
 *     SX6-EVB# set debugmips y;save
 *
 *     @@ turn off mips debug in uboot prompts
 *     SX6-EVB# set debugmips;save
 *
 *     @@ turn on mips debug in kernel prompts
 *     # uenv set debugmips y
 *
 *     @@ turn off mips debug in kernel prompts
 *     # uenv set debugmips n
 *
 * The supporting images and relative naming (case sensitive) are listed below:
 * 	avdecoder.bin
 * 	display.bin
 * 	database.TSE
 * 	sound_preset.bin
 * 	setting_cfg.xml
 * 	initsettings.file
 * 	logo.bin
 *
 * Quick start example
 * 1. press any key to enter uboot prompts and type command
 * 	SX6-EVB# set debugmips y;save
 * 2. copy images to debug/mipsimg of one usb stick
 * 3. connect that usb stick to target
 * 4. give a reset
 *
 */

typedef struct _tagSdMipsDbg{
	int active;
#define MDBG_DOMAIN_NONE	0
#define MDBG_DOMAIN_USB		1
	int domain;
	int (*init)(void);			/* init, mandatory */
	int (*prepare)(void);
	int (*cleanup)(void);
	int (*scan)(int arg1, int arg2);	/* scan for debug images, return 0 on success or -1 otherwise */
	int (*load)(int id, void* buf,int len);	/* load len bytes of specified debug image to buffer,
						 * giving len=0 for all.
						 * return number of copied bytes on success, or -1 on error
						 */
	struct mdbg_payload{
		SdImgPayload_t id;
	}data[MIPS_PAYLOAD_MAX];
}SdMipsDbg_t;

static int mdbg_domain_init(void);
static int mdbg_domain_scan(int, int);
static int mdbg_domain_load(int, void*, int);
static SdMipsDbg_t mdbg = {
	.active = 0,
	.domain = MDBG_DOMAIN_NONE,
	.init = mdbg_domain_init,
	.scan = mdbg_domain_scan,
	.load = mdbg_domain_load,
	.data[0].id = IMG_INVAL,
};

#if defined(CONFIG_CMD_USB) && defined(CONFIG_CMD_FAT)
#include <part.h>
#include <fat.h>
#define MDBG_DIR "debug/mipsimg/"

#if defined(CONFIG_SIGMA_SOC_SX6) && defined(CONFIG_USB_MONZA_EHCI) && !defined(CONFIG_SYS_DCACHE_OFF)
# define mdbg_sys_prepare()	do{			\
			debug("dcache off\n");		\
			run_command("dcache off", 0);	\
		}while(0)
# define mdbg_sys_cleanup()	do{			\
			debug("dcache on\n");		\
			run_command("dcache on", 0);	\
		}while(0)
#else
# define mdbg_sys_prepare() do{}while(0)
# define mdbg_sys_cleanup() do{}while(0)
#endif

static int mdbg_domain_prepare(void)
{
	mdbg_sys_prepare();
	return 0;
}

static int mdbg_domain_cleanup(void)
{
	mdbg_sys_cleanup();
	return 0;
}

#undef mdbg_sys_prepare
#undef mdbg_sys_cleanup

static int mdbg_domain_init(void)
{
	mdbg.prepare = mdbg_domain_prepare;
	mdbg.cleanup = mdbg_domain_cleanup;
	return 0;
}

extern long do_fat_read (const char *filename, void *buffer, unsigned long maxsize, int dols);
static int mdbg_domain_scan(int dev, int arg2)
{
#define FAT_TRY_LEN	4
#define CHECK_IMG(__f, __id) do{ 						\
		char __t[FAT_TRY_LEN];						\
		int __ret = do_fat_read(MDBG_DIR __f, __t, sizeof(__t), LS_NO);	\
		if (__ret != -1 ) {						\
			debug("mdbg: got %s\n", __f);				\
			mdbg.data[i++].id = __id;				\
		}								\
	}while(0)

	int i = 0, ret = -1;
	int part = 1;
	block_dev_desc_t *dev_desc=NULL;

	run_command("usb start", 0);

	dev_desc = get_dev("usb", dev);
	if (dev_desc == NULL) {
		puts("\n** mdbg: Invalid device **\n");
		goto out;
	}

	if (fat_register_device(dev_desc, part) != 0) {
		printf("\n** Unable to use usb %d:%d for fatload **\n", dev, part);
		goto out;
	}

	ret = 0;
	mdbg.domain = MDBG_DOMAIN_USB;

#if 0
	CHECK_IMG("avdecoder.bin",	IMG_AVMIPS);
	CHECK_IMG("display.bin",	IMG_DISPMIPS);
	CHECK_IMG("sound_preset.bin",	IMG_SOUND);
	CHECK_IMG("database.TSE",	IMG_DB);
	CHECK_IMG("setting_cfg.xml",	IMG_CFG);
	CHECK_IMG("initsetting.file",	IMG_ST);
	CHECK_IMG("logo.bin",		IMG_LOGO);
#endif
out:
	mdbg.data[i].id = IMG_INVAL;
	return ret;
#undef FAT_TRY_LEN
#undef CHECK_IMG
}

static int mdbg_domain_load(int id, void* buff, int len)
{
	int sz = 0;
	char url[64];

	if (mdbg.domain != MDBG_DOMAIN_USB) {
		return -1;
	}

	snprintf(url, 64, "%s%s", MDBG_DIR, GetImgName(id));
	sz = file_fat_read(url, (unsigned char*)buff, len);
	if (sz > 0) {
		printf("[  OK] ");
		printf("load '%s' to 0x%08x(0x%08x)\n", GetImgName(id), (int)buff, sz);
		flush_cache((ulong)buff, sz);
	} else {
		printf("[FAIL] ");
		debug("unable to read debug image '%s' from usb", GetImgName(id));
		printf("\n");
	}

	return sz;
}
#else /*CONFIG_CMD_USB && CONFIG_CMD_FAT*/
static int mdbg_domain_init(void)
{
	return 0;
}

static int mdbg_domain_scan(int arg1, int arg2)
{
	mdbg.domain = MDBG_DOMAIN_NONE;
	return 0;
}

static int mdbg_domain_load(int id, void* buf, int len)
{
	return -1;
}
#endif /*CONFIG_CMD_USB && CONFIG_CMD_FAT*/

static int mdbg_setup(void)
{
	mdbg.init();
	if (getenv_yes("debugmips")) {
		mdbg.active = 1;
		if (mdbg.prepare)
			mdbg.prepare();
		return mdbg.scan(0, 0);
	} else
		return -1;
}

static int mdbg_cleanup(void)
{
	if (mdbg.active) {
		mdbg.active = 0;
		if (mdbg.cleanup)
			mdbg.cleanup();
	}
	return 0;
}

#endif /*CONFIG_DEBUG_MIPS*/

#ifdef CONFIG_CMD_EXT4
static int load_from_ext4(SdImgPayload_t id)
{
	if(id == IMG_DB || id == IMG_LOGO || id == IMG_AUDIO)
		return 1;
	return 0;
}

static int ext4_load_file(const char* file_name, void *addr)
{
	const char *ifname = "mmc";
	const char *dev_part = "0";
	int fstype = FS_TYPE_EXT;
	loff_t len_read;

	if(!file_name || !addr)
		return -1;

	if(fs_set_blk_dev(ifname, dev_part, fstype))
		return -1;

	if(fs_read(file_name, (unsigned long)addr, 0, 0, &len_read) < 0)
		return -1;

	return len_read;
}

static int ext4_get_md5sum(const char* file_name, char* md5sum)
{
	char *addr;
	const char *md5sum_dir = ".meta/";
	char md5sum_file[64];

	if(!file_name || !md5sum)
		return -1;

	addr = (char *)malloc(128);
	if(!addr)
	{
		pr_err("malloc for md5sum file failed\n");
		return -1;
	}
	snprintf(md5sum_file, sizeof(md5sum_file), "%s%s.meta", md5sum_dir, file_name);
	if(ext4_load_file(md5sum_file, (void *)addr) < 0)
	{
		pr_warn("can't get %s\n", md5sum_file);
		free(addr);
		return -1;
	}
	memcpy(md5sum, addr, MD5SUM_LEN);

	free(addr);
	return 0;
}

static int ext4_calc_md5sum(void* addr, unsigned int len, char* md5sum)
{
	unsigned char output[16];
	unsigned int i;
	char *p;

	if(!addr || len == 0 || !md5sum)
		return -1;

	md5_wd((unsigned char *)addr, len, output, CHUNKSZ_MD5);

	p = md5sum;
	for (i = 0; i < 16; i++) {
		sprintf(p, "%02x", output[i]);
		p += 2;
	}

	return 0;
}

static int ext4_check_file(const char* file_name, void* addr, unsigned int len)
{
	MetaData_t meta1, meta2;

	if(!file_name || !addr || len == 0)
		return -1;
	if(ext4_calc_md5sum(addr, len, meta1.md5sum) != 0)
		return -1;
	if(ext4_get_md5sum(file_name, meta2.md5sum) != 0)
		return -1;
	if(memcmp(meta1.md5sum, meta2.md5sum, MD5SUM_LEN) != 0)
	{
		pr_warn("checksum did not match\n");
		return -1;
	}

	return 0;
}
#endif

static int do_load_memfile(void* img, u32 len, void* mimg_ddr, u32 mlen, int target_ending)
{
	int ret = 0;
	void *mdest = NULL;

	if(!img || (((int)img) & 3) || !mimg_ddr || len < mlen + MEM_FILE_OFFSET + LENGHTH_SIZE)
		return 0;

	mdest = (void*)(img + MEM_FILE_OFFSET);
	debug("load memfile to 0x%p(%x)\n", mdest, mlen);
	if (mlen <= MAX_MEM_FILE_SIZE)
	{
		if (GET_ENDING() ^ target_ending)
			*(int*)mdest = SWAP_32(mlen);
		else
			*(int*)mdest = mlen;

		if (mdest + LENGHTH_SIZE != mimg_ddr)
			memcpy(mdest + LENGHTH_SIZE, mimg_ddr, mlen);
		flush_cache((ulong)mdest, mlen + LENGHTH_SIZE);
		ret = mlen + LENGHTH_SIZE;
	}
	else
	{
		pr_err("memfile size exceeds %#x!!\n", MAX_MEM_FILE_SIZE);
		ret = 0;
	}
	return ret;
}

static int do_check_memfile(void *img, u32 len, int target_ending)
{
	u32 mlen = 0;
	void *mstart = NULL, *rootpos = NULL;
	if (!img || len < MEM_FILE_OFFSET + 4 + MAX_MEM_FILE_SIZE)
		return 1;

	if (((int)img) & 3)
	{
		pr_err("base addr(%p) doesn't aligned by DWORD!\n", img);
		return 1;
	}

	mstart = img + MEM_FILE_OFFSET;
	if (GET_ENDING() ^ target_ending)
		mlen = SWAP_32(*(int*)mstart);
	else
		mlen = *(int*)mstart;

	if (mlen > MAX_MEM_FILE_SIZE)
	{
		pr_err("memfile size %#x read @%p exceeds %#x!!\n", mlen, mstart, MAX_MEM_FILE_SIZE);
		return 1;
	}

	mstart += 4;
	if (!(rootpos = strstr(mstart, "<root>")) || (rootpos - mstart) > MAX_MEM_FILE_SIZE)
	{
		pr_err("cant find memfile @%p!!\n", mstart);
		return 1;
	}

	return 0;
}

typedef struct simple_xml_node{
	const char* start;
	const char* end;
} simple_xml_node_t;

static char * strnstr(const char * s1,const char * s2,const int n)
{
	int l1, l2;
	int m = n;

	l2 = strlen(s2);
	if (!l2)
		return (char *) s1;
	l1 = strlen(s1);
	while (l1 >= l2 && m >= l2) {
		l1--;
		m--;
		if (!memcmp(s1,s2,l2))
			return (char *) s1;
		s1++;
	}
	return NULL;
}

static int xml_get_node_attr(void *handler, const char* attrname, u32 *pVal)
{
	int ret = -10;
	u32 val = 0;
	simple_xml_node_t *node;
	char* t = NULL;

	if (NULL == handler || NULL == attrname)
		goto fail;

	node = (simple_xml_node_t*)handler;

	/*
	 * retrieve attr value
	 * <nodename attrname0= "xx",attrname1= "xx",...
	 */
	if ((t = strnstr(node->start, attrname, node->end - node->start)) == NULL) {
		pr_err("cant find attr '%s', wrong xml file!?\n", attrname);
		goto fail;
	}

	if ((t = strchr(t + strlen(attrname), '"')) == NULL || (t > node->end)) {
		pr_err("failed in searching value for '%s', wrong xml file!?\n", attrname);
		goto fail;
	}

	*pVal = val = simple_strtoul(t + 1, NULL, 0);
	ret = 0;
#ifdef DEBUG
	printf("get value:0x%x, attr: %s\n", val, attrname);
#endif
fail:
	return ret;

}

static int xml_get_node(void* handler, const char* nodename, simple_xml_node_t* node)
{
	simple_xml_node_t* root = (simple_xml_node_t*)handler;
	char *s, *h, *t;
	int nb = root->end - root->start;
	s = h = t = NULL;
	if((s = strnstr(root->start, nodename, nb)) != NULL) {
		if ( *(s + strlen(nodename)) == '>') {
			/*
			 * <name>
			 *    body
			 * </name>
			 */
			h = s + strlen(nodename) + 1; /*skip '>'*/
			nb -= h - root->start;
			if ((s = strnstr(h, nodename, nb)) != NULL) {
				t = s - 3;	/*skip "</"*/
				goto ok;
			} else {
				goto error;
			}
		} else {
			/*
			 * <name body />
			 */
			h = s + strlen(nodename);
			nb -= h - root->start;
			if ((s = strnstr(h, "/>", nb)) != NULL) {
				t = s - 1;
				goto ok;
			} else {
				goto error;
			}
		}
	} else {
		goto error;
	}

ok:
	memset(node, 0, sizeof(simple_xml_node_t));
	node->start = h;
	node->end = t;
#ifdef DEBUG
	printf("dump xml node <%s>:\n", nodename);
	s = h;
	while(s != t)
		putc(*s++);
	printf("\n============= end ============\n");
#endif
	return 0;
error:
	pr_err("fail to get node <%s>, wrong xml file?!\n", nodename);
	return -11;
}

static int xml_get_uint(void *handler, const char* nodename, const char* attrname, u32 * pVal)
{
	int ret = -12;
	u32 val = 0;
	simple_xml_node_t *root, node;

	if (NULL == handler || NULL == nodename || NULL == attrname)
		goto fail;

	root = (simple_xml_node_t*)handler;

	/*get node*/
	if (xml_get_node(root, nodename, &node) != 0)
		goto fail;

	if (xml_get_node_attr(&node, attrname, &val) != 0)
		goto fail;

	*pVal = val;
	ret = 0;
#ifdef DEBUG
	printf("get value:0x%x, node: %s\n", val, nodename);
#endif
fail:
	return ret;
}

static int do_fixup_xml_node_attr(void* base, unsigned target_ending, unsigned ofs,
				   unsigned dlsize, int val, const char* attr)
{
	int ret = 1;
	int dlen = 0;
	char *p = NULL, *q = NULL, *r = NULL;
	char str_buff[32];

	if (!base || (((int)base) & 3))
	{
		pr_err("invalid memfile addr:%p\n", base);
		return 1;
	}

	if (dlsize == 0)
		dlsize = 4;

	if (dlsize == 4 ){
		dlen = *(int*)base;
		if (GET_ENDING() ^ target_ending)
			dlen = SWAP_32(dlen);
	} else if (dlsize == 2){
		dlen = *(short*)base;
		if (GET_ENDING() ^ target_ending)
			dlen = SWAP_16(dlen);
	} else if (dlsize == 1){
		dlen = *(char*)base;
	} else {
		pr_err("invalid dlen size:%d\n", dlsize);
		return 1;
	}

	debug("dlen=%d\n", dlen);
	if (dlen > MAX_MEM_FILE_SIZE)
	{
		pr_err("invalid memfile size: %d\n", dlen);
		return 1;
	}

	sprintf(str_buff, "\"0x%x\"", val);
	/*
	 * fixup value for node in memfile, in pattern
	 * '<name Val="value" />'
	 */
	if (!strncmp(base+dlsize+ofs, attr, strlen(attr))) {
		p = base + dlsize + ofs;
	} else {
		p = strstr(base+dlsize, attr);
	}

	if (p != NULL && (p - ((char*)base + dlsize)) < MAX_MEM_FILE_SIZE)
	{
		q = strchr(p + strlen(attr), '"');
		r = strstr(p + strlen(attr), "/>");
		if (q && r && q < r)
		{
			if ((r - q) < strlen(str_buff))
			{
				int nbytes = strlen(str_buff) - (r - q);
				pr_info("memmove memfile at offset %d by %d bytes\n", ((void*)r - base - dlsize), nbytes);
				memmove(r + nbytes, r, dlen - ((void*)r - base - dlsize));
				r += nbytes;
				dlen += nbytes;
				if (dlsize == 4 ) {
					if (GET_ENDING() ^ target_ending)
						dlen = SWAP_32(dlen);
					*(int*)base = dlen;
				} else if (dlsize == 2) {
					if (GET_ENDING() ^ target_ending)
						dlen = SWAP_16(dlen);
					*(short*)base = dlen;
				} else {
					*(char*)base = dlen;
				}
			}

			pr_info("<%s Val -> '%d'\n", attr, val);
			memcpy(q, str_buff, strlen(str_buff));
			q += strlen(str_buff);
			while (q < r)
				*q++ = ' ';

			flush_cache((ulong)base, dlen + dlsize);
#ifdef DEBUG
			char log_buff[32];
			memcpy(log_buff, p, 31);
			log_buff[31] = '\0';
			printf("-----------------cut here---------------\n");
			printf("%s ...\n", log_buff);
			printf("-----------------cut here---------------\n");
#endif
			ret = 0;
		}
	}
	return ret;
}

static int do_fixup_loadaddr(SdImgPayload_t id, void* cfg, unsigned dlen, unsigned *paddr)
{
	simple_xml_node_t root, target;
	if (NULL == cfg)
		return 1;

	memset(&root, 0, sizeof(simple_xml_node_t));
	root.start = (char*)cfg;
	root.end = (char*)cfg + dlen;

	if (IMG_DB == id) {
		simple_xml_node_t dispnode;
		return (xml_get_node(&root, "DISP_MIPS", &dispnode) ||
			xml_get_node(&dispnode, "InitParam", &target) ||
			xml_get_uint(&target, "start", "Val", paddr));
	} else if (IMG_SOUND == id) {
		simple_xml_node_t avnode;
		return (xml_get_node(&root, "AV_MIPS", &avnode) ||
			xml_get_node(&avnode, "SndBuf", &target) ||
			xml_get_uint(&target, "start", "Val", paddr));
	} else if (IMG_LOGO == id) {
		/*
		 * note that 0xffffffff (-1) might be given for invalid logo addr
		 * and loading logo shall be skipped in that case
		 */
		unsigned retv, tmp = 0;
		retv = (xml_get_node(&root, "LOGO_ADDR", &target) ||
			xml_get_uint(&target, "start", "Val", &tmp));
		tmp = (0xffffffff == tmp) ? 0 : tmp;
		*paddr = tmp;
		return retv;
	}
	return 0;
}

static int do_load_image(SdImgPayload_t id, void* img, u32 slen, u32 dest, u32 blen, loadfunc_t loader)
{
	int ret = 0, len = 0;

	if (0 == dest)
	{
		pr_err("no load addr is specified for %s\n", GetImgName(id));
		return -1;
	}

#ifdef CONFIG_DEBUG_MIPS
	/* in case have debug image... */
	if((ret = mdbg.load(id, (void*)dest, blen)) != -1)
		return ret;
#endif

#ifdef CONFIG_CMD_EXT4
	if(1 == load_from_ext4(id))
	{
		if((len = ext4_load_file(GetImgName(id), (void*)dest)) != -1)
		{
			if(ext4_check_file(GetImgName(id), (void*)dest, len) == 0)
			{
				printf("\nEXT4 load: %s load: OK\n", GetImgName(id));
				return 0;
			}
		}
	}
#endif

	if (!img || slen==0)
		return -1;
	if (!loader)
		return -1;

	len = (blen == 0 || blen > slen) ? slen : blen;
	//stop mips
	stop_mips(id);
	debug("load '%s' to 0x%08x(0x%08x)\n", GetImgName(id), dest, len);

	//load images
	ret = loader(img, len, (void*)dest);

	//start mips
	//start_mips(id, (u32)dest);	// postpone start mips until load settings

	return ret;
}

static int do_bootmips(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;
	mips_img_hdr header, *hdr = &header;
	unsigned int npages = 0;
	unsigned int av_addr = 0;
	unsigned int av_size = 0;
	unsigned int disp_addr = 0;
	unsigned int disp_size = 0;
	unsigned int cfg_size = 0;
	unsigned int st_size = 0;
	void* cfg_img = NULL;
	void* st_img = NULL;
	void* img = NULL;
	char cmd_buffer[64];

	void* cfg_buf = NULL;
	void* load_addr = NULL;
	int load_size = -1;
	int mem_sel = 0;
	int i = 0;
	SdMipsBootDev_t *bootdev = &BOOT_DEVICE(ddr); /* default boot from dram */
	char* p = NULL, *cfg_fn = NULL, *st_fn = NULL;

	if (argc < 2)
		return CMD_RET_USAGE;

	load_addr = (void *)simple_strtoul(argv[1], NULL, 16);
	if (argc > 2)
		load_size = (int)simple_strtoul(argv[2], NULL, 16);

#ifdef CONFIG_SIGMA_MCONFIG
	/**
	 * Fix up cfg selection
	 *
	 * 1. uboot env 'mconfig'
	 *    it's a tripple state variable that controls memory config scheme
	 *    y - auto config
	 *    m - manual config (allow user to set 'msize')
	 *    n - off (legacy mode)
	 * 2. uboot env 'msize'
	 *    it's an 32-bit integar variable in syntax
	 *    bit[3:0]   UMAC0 memory size
	 *    bit[7:4]   UMAC1 memory size
	 *    bit[11:8]  UMAC2 memory size
	 *    bit[31:12] reserved
	 *    @ UMAC memory size is illuminated in 128MB granule
	 *    @ it will be auto set by mprobe cmd in case of mconfig=y or preset by user for mconfig=m
	 **/
	p = getenv("mconfig");
	if (p && (*p == 'y' || *p == 'm')) {
		p = getenv("msize");
		if (p == NULL) {
			pr_err("can't getenv 'msize', bootmips failed!\n");
			return 2;
		}
		mem_sel = simple_strtoul(p, 0, 16);
	}
#endif

	if ((p = strchr(argv[1], ':')))
	{
		p++;
		if (strncmp(p, "ddr", 3) == 0)
			bootdev = &BOOT_DEVICE(ddr);
#ifdef CONFIG_CMD_MMC
		else if (strncmp(p, "mmc", 3) == 0)
			bootdev = &BOOT_DEVICE(mmc);
#endif
#ifdef CONFIG_CMD_NAND
		else if (strncmp(p, "nand", 4) == 0)
			bootdev = &BOOT_DEVICE(nand);
#endif
		else
			{pr_err("invalid bootdev: '%s'\n", p); return CMD_RET_USAGE;}
	}

#ifdef CONFIG_DEBUG_MIPS
	mdbg_setup();
#endif

	/* get boot.img header */
	memset(hdr, 0, sizeof(mips_img_hdr));
	bootdev->get_header(load_addr, load_size, hdr);

	/* let's check the image loading address to see if it is mips boot.img first */
	if (strncmp((char*)hdr->magic, MIPS_MAGIC, MIPS_MAGIC_SIZE) != 0)
	{
		pr_err("Not a valid MIPS boot.img!\n");
		ret = 1;
		goto OUT;
	}

	/*   
 	 * this is raw mips boot image, let's find out where the 
	 * av, disp and other images be
	 */
	printf("Boot mips from %s at %08x\n", p ? p : "ddr", (int)load_addr);

	#define LOCAL_PT_TYPE(i) PT_GET_TYPE(hdr->data[i].type)
	#define LOCAL_PT_MSIZE(i) PT_GET_MSIZE(hdr->data[i].type)
	#define for_each_image(i, d, n, h, l) for((i)=0,(d)=(l)+(h)->page_size,(n)=0;	\
		(i)<MIPS_PAYLOAD_MAX && (h)->data[i].type!=0;				\
		(n)=((h)->data[i].size + (h)->page_size - 1) / (h)->page_size,		\
		(d)=(d) + (n) * (h)->page_size, i++)
	for_each_image(i, img, npages, hdr, load_addr) {
		printf("##%-20s 0x%08x(0x%08x) -> 0x%08x\n",
			hdr->data[i].name, (int)img, hdr->data[i].size, hdr->data[i].addr);
		if (PT_AV == LOCAL_PT_TYPE(i)) {
			av_size = hdr->data[i].size;
			av_addr = hdr->data[i].addr;
		} else if (PT_DISP == LOCAL_PT_TYPE(i)) {
			disp_size = hdr->data[i].size;
			disp_addr = hdr->data[i].addr;
		} else if (PT_CFG == LOCAL_PT_TYPE(i)) {
			if (mem_sel == 0 || mem_sel == LOCAL_PT_MSIZE(i) || (LOCAL_PT_MSIZE(i) == 0 && cfg_img == NULL)) {
				cfg_img = img;
				cfg_size = hdr->data[i].size;
				cfg_fn = hdr->data[i].name;
			}
		} else if (PT_ST == LOCAL_PT_TYPE(i)) {
			if (mem_sel == 0 || mem_sel == LOCAL_PT_MSIZE(i) || (LOCAL_PT_MSIZE(i) == 0 && st_img == NULL)) {
				st_img = img;
				st_size = hdr->data[i].size;
				st_fn = hdr->data[i].name;
			}
		}
	}

	if (mem_sel != 0 && cfg_img == NULL) {
		pr_warn("failed to match config file 'setting_cfg%x.xml'!\n", mem_sel);
	} else {
		pr_info("select config file '%s'\n", cfg_fn);
	}

	if (mem_sel != 0 && st_img == NULL) {
		pr_warn("failed to match setting file 'initsetting%x.file'!\n", mem_sel);
	} else {
		pr_info("select setting file '%s'\n", st_fn);
	}

	/*
	 * preload memfile to dram (must prior to load images for possible addr fixup)
	 */
	if (cfg_img) {
		cfg_buf = get_buffer(MAX_MEM_FILE_SIZE, 1);
		if ((cfg_size=do_load_image(IMG_CFG,cfg_img,cfg_size,(u32)cfg_buf,MAX_MEM_FILE_SIZE,bootdev->load_image)) < 0)
			cfg_buf = NULL;
	}

	/*
 	 * load images
 	 */
	for_each_image(i, img, npages, hdr, load_addr) {
		#define PTYPE2ID(t) ({					\
			int _i = IMG_DATA;				\
			switch ((t) & PT_TYPE_MASK){			\
				case PT_AV: _i=IMG_AVMIPS;break;	\
				case PT_DISP: _i=IMG_DISPMIPS;break;	\
				case PT_DB: _i=IMG_DB;break;		\
				case PT_SP: _i=IMG_SOUND;break;		\
				case PT_CFG: _i=IMG_CFG;break;		\
				case PT_ST: _i=IMG_ST;break;		\
				case PT_LOGO: _i=IMG_LOGO;break;	\
				case PT_AUDIO: _i=IMG_AUDIO;break;	\
				default: _i=IMG_DATA;break;		\
			}						\
			_i;						\
		})

		int id = PTYPE2ID(hdr->data[i].type);
		/* fixup image load addr in case of invalid addr by lookuping cfg xml file */
		if (hdr->data[i].addr == 0xffffffff) {
			hdr->data[i].addr = 0; /*reset*/
			if (cfg_buf != NULL)
				do_fixup_loadaddr(id, cfg_buf, cfg_size, &hdr->data[i].addr);
			pr_info("fixup %-20s load addr to 0x%08x\n", hdr->data[i].name, hdr->data[i].addr);
		}

		/* do we really need to load image? */
		if (hdr->data[i].addr != 0) {
			ret = do_load_image(id, img, hdr->data[i].size, hdr->data[i].addr, 0, bootdev->load_image);
			if (ret < 0)
				goto OUT;
		}
	}

	/*
	 * load memfile to dram
	 */
	if (!cfg_buf ||
	    (!av_addr||!do_load_memfile((void*)av_addr, av_size, cfg_buf, cfg_size, hdr->target_ending)) ||
	    (!disp_addr||!do_load_memfile((void*)disp_addr, disp_size, cfg_buf, cfg_size, hdr->target_ending))) {
		pr_warn("load memfile failed!\n");
	}

	/*
	 * post-load images
	 */
	if (av_addr != 0)
	{
		/* verify memfile */
		if (do_check_memfile((void*)av_addr, av_size, hdr->target_ending)) {
			pr_err("invalid memfile!\n");
			goto OUT;
		}
	}

	if (disp_addr != 0)
	{
		simple_xml_node_t root, node;
		unsigned int tmp = 0;
		unsigned nodeofs = 0;
		/* verify memfile */
		if (do_check_memfile((void*)disp_addr, disp_size, hdr->target_ending)) {
			pr_err("invalid memfile!\n");
			goto OUT;
		}

		memset(&root, 0, sizeof(simple_xml_node_t));
		root.start = (char*)cfg_buf;
		root.end = (char*)(cfg_buf + cfg_size);
		if ((ret = xml_get_node(&root, "DISP_MIPS", &root)) ||
		    (ret = xml_get_node(&root, "InitParam", &root))) {
			pr_err("xml_get_node error, unknown xml file!?\n");
			goto OUT;
		}
		/*
		 * revise paneltype from env 'panel_type' in case paneltype=0xFF is given
		 */
		if ((ret = xml_get_node(&root, "panel", &node)) ||
		    (ret = xml_get_node_attr(&node, "Val", &tmp))) {
			pr_err("fail to get panel val, unknown xml file!?\n");
			goto OUT;
		}

		if (0xFF == tmp) {
			char* p = getenv("panel_type");
			if (p != NULL) {
				const char* attr = "panel";
				nodeofs = (unsigned)((void*)node.start - cfg_buf - strlen(attr));
				tmp = simple_strtoul(p, NULL, 10);
				if ((do_fixup_xml_node_attr((void*)disp_addr+hdr->mcfg_ofs,
							   hdr->target_ending, nodeofs,
							   hdr->mcfglen_size, tmp, attr))) {
					pr_err("fixup panel type failed!\n");
					goto OUT;
				}
			} else {
				pr_warn("env 'panel_type' is not set!\n");
			}
		}

		/*
		 * revise frcxpal from env 'frcx_panel' in case frcxpal=0xFF is given
		 */
		if ((ret = xml_get_node(&root, "frcxpal", &node)) ||
		    (ret = xml_get_node_attr(&node, "Val", &tmp))) {
			pr_err("fail to get frcxpal val, unknown xml file!?\n");
			goto OUT;
		}

		if (0xFF == tmp){
			char* p = getenv("frcx_panel");
			if (p != NULL) {
				const char* attr = "frcxpal";
				nodeofs = (unsigned)((void*)node.start - cfg_buf - strlen(attr));
				tmp = simple_strtoul(p, NULL, 10);
				if ((do_fixup_xml_node_attr((void*)disp_addr+hdr->mcfg_ofs,
							    hdr->target_ending, nodeofs,
							    hdr->mcfglen_size, tmp, attr))) {
					pr_err("fixup frcxpal type failed!\n");
					goto OUT;
				}
			} else {
				pr_warn("env 'frcx_panel' is not set!\n");
			}
		}

		/*
		 * revise mipsflag from env 'mipsflag' in case mipsflag=0xFFFFFFFF is given
		 */
		if ((ret = xml_get_node(&root, "mipsflag", &node)) ||
		    (ret = xml_get_node_attr(&node, "Val", &tmp))) {
			pr_err("fail to get mipsflag val, legacy xml file!?\n");
			//goto OUT;	/*no abandon for 'mipsflag' not exist in legacy prj*/
		}

		if (0xFFFFFFFF == tmp){
			char* p = getenv("mipsflag");
			if (p != NULL) {
				const char* attr = "mipsflag";
				nodeofs = (unsigned)((void*)node.start - cfg_buf - strlen(attr));
				tmp = simple_strtoul(p, NULL, 10);
				if ((do_fixup_xml_node_attr((void*)disp_addr+hdr->mcfg_ofs,
							    hdr->target_ending, nodeofs,
							    hdr->mcfglen_size, tmp, attr))) {
					pr_err("fixup mipsflag failed!\n");
					goto OUT;
				}
			} else {
				pr_warn("env 'mipsflag' is not set!\n");
			}
		}

		ret = 0;	/*reset*/
	}

#ifdef CONFIG_CMD_LOADSETTINGS
	/*
 	 * load init settings
 	 * NOTE: init settings include PMAN protection settings, which restricts accessible
 	 * memory range on host CPU as well, thus postone load settings after done all load_img
 	 * besides, settings are required before start mips
 	 */
	if (getenv_yesno("loadinitsetting"))
	{
#define MAX_SETTINGS_SIZE 0x4000	/* 16k? */
		void *st_buf = get_buffer(MAX_SETTINGS_SIZE, 1);
		st_size = do_load_image(IMG_ST, st_img, st_size, (u32)st_buf, MAX_SETTINGS_SIZE, bootdev->load_image);
		if(st_size > 0) {
			sprintf(cmd_buffer, "loadsettings %x %x", (unsigned int)st_buf, st_size);
			ret = run_command(cmd_buffer, flag);
			if (ret != 0)
				goto OUT;
		} else {
			pr_warn("load initsettings failed!\n");
			goto OUT;
		}
#undef MAX_SETTINGS_SIZE
	}
#endif /* CONFIG_CMD_LOADSETTINGS */

	/*
 	 * start mips 
 	 */
	if (av_addr) start_mips(IMG_AVMIPS, av_addr);
	if (disp_addr) start_mips(IMG_DISPMIPS, disp_addr);
	printf("Boot mips ... OK\n");

OUT:
#ifdef CONFIG_DEBUG_MIPS
	mdbg_cleanup();
#endif
	free_buffer();
	bootdev->withdraw();
	return ret;
}

U_BOOT_CMD(
	bootmips, CONFIG_SYS_MAXARGS, 1, do_bootmips,
	"boot mips from MIPS boot.img",
	("<addr[:dev]> [size]\n"
	"    boot mips from device 'dev' at hex address 'addr'\n"
	"    where 'dev' can be:\n"
	"      'ddr'  - dram (default)\n"
#ifdef CONFIG_CMD_MMC
	"      'mmc'  - mmc flash\n"
#endif
#ifdef CONFIG_CMD_NAND
	"      'nand' - nand flash\n"
#endif
	)
);
