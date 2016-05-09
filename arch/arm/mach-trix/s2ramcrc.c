#ifndef __UBOOT__
#define __UBOOT__
#endif
#include "s2ramctrl.h"

#ifdef __UBOOT__
# include <common.h>
# include "crc32.h"
# define PDEBUG(fmt...) printf(fmt)
# define FLUSH_CACHE(s, l) flush_cache((unsigned long)(s),l)
# define MMAP(p, l) (p)
# define MUNMAP(v) do{(v) = NULL;}while(0)
# define GET_CRC32(s,l) crc32_lite (S2RAM_CRC32_SEED, (const void*)(s), l)
#elif defined __KERNEL__
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/crc32.h>
#include <asm/mach/map.h>
#include <asm/cacheflush.h>

# define PDEBUG(fmt...) printk(fmt)
# define FLUSH_CACHE(s, l) do{				\
		flush_cache_all();			\
		outer_clean_range(virt_to_phys(s),	\
			virt_to_phys(s) + (l));		\
	}while(0)
# define MMAP(p, l)	(void __iomem*)__arm_ioremap((unsigned long)(p), l, MT_MEMORY)
# define MUNMAP(v)	do{				\
		__arm_iounmap((void __iomem*)(v));	\
		(v) = NULL;				\
	}while(0)

# define GET_CRC32(s,l) crc32(S2RAM_CRC32_SEED, (const void*)(s), l)
#else
# include "crc32.h"
# define PDEBUG(fmt...)
# define FLUSH_CACHE(s, l)
# define MMAP(p, l) (p)
# define MUNMAP(v) do{(v) = NULL;}while(0)
# define GET_CRC32(s,l) crc32_lite (S2RAM_CRC32_SEED, (const void*)(s), l)
#endif

#define S2RAM_CRC32_SEED	0x12345678

static void print_memory(const void *mem, unsigned len)
{
#define MAX_PRINT_MEM_SIZE 256
	int i, total;
	int *start = (int*)((int)mem & (~3));
	total = (((MAX_PRINT_MEM_SIZE > len) ? len : MAX_PRINT_MEM_SIZE) + 3 ) >> 2;
	for (i = 0; i < total; i ++)
	{
		if (!(i & 0x3))
		{
			if (i != 0)
				PDEBUG("\n");
			PDEBUG("%08x: ", (int)(start + i));
		}
		PDEBUG("%08x ", *(start+i));
	}
	PDEBUG("\n");
}

int s2ram_set_frame(void *entry, void *mem0, unsigned len0, void *mem1, unsigned len1, struct s2ram_resume_frame *frame)
{
	void *vmem = NULL;
	frame->S2RAM_ENTRY = (long)entry;
	frame->S2RAM_START0 = (long)mem0;
	frame->S2RAM_LEN0 = (long)len0;
	if (mem0 && len0 > 0)
	{
		vmem = MMAP(mem0, len0);
		frame->S2RAM_CRC0 = GET_CRC32(vmem, len0);
		PDEBUG("------mem#0 [0x%08x],CRC=0x%08x------\n", (int)mem0, (int)frame->S2RAM_CRC0);
		print_memory(vmem, len0);
		MUNMAP(vmem);
	}
	frame->S2RAM_START1 = (long)mem1;
	frame->S2RAM_LEN1 = (long)len1;
	if (mem1 && len1 > 0)
	{
		vmem = MMAP(mem1, len1);
		frame->S2RAM_CRC1 = GET_CRC32(vmem, len1);
		PDEBUG("------mem#1 [0x%08x],CRC=0x%08x------\n", (int)mem1, (int)frame->S2RAM_CRC1);
		print_memory(vmem, len1);
		MUNMAP(vmem);
	}
	frame->S2RAM_CRC = GET_CRC32(frame, S2RAM_FRAME_SIZE - 4);
	PDEBUG("------frame [0x%08x],CRC=0x%08x------\n", (int)frame, (int)frame->S2RAM_CRC);
	print_memory(frame, S2RAM_FRAME_SIZE);
	PDEBUG("---------------------------------------------\n");
	FLUSH_CACHE(frame, S2RAM_FRAME_SIZE);
	return 0;
}

int s2ram_check_crc(struct s2ram_resume_frame* frame)
{
	unsigned long crc32 = -1l;
	crc32 = GET_CRC32(frame, S2RAM_FRAME_SIZE - 4);
	PDEBUG("------frame [0x%08x],CRC=0x%08x------\n", (int)frame, (int)crc32);
	print_memory(frame, S2RAM_FRAME_SIZE);
	if (crc32 != frame->S2RAM_CRC)
	{
		PDEBUG("resume frame: CRC error!\n");
		goto FAIL;
	}
	if (frame->S2RAM_START0 && frame->S2RAM_LEN0 > 0)
	{
		crc32 = GET_CRC32(frame->S2RAM_START0, frame->S2RAM_LEN0);
		PDEBUG("------mem#0 [0x%08x],CRC=0x%08x------\n", (int)frame->S2RAM_START0, (int)crc32);
		print_memory((void*)frame->S2RAM_START0, frame->S2RAM_LEN0);
		if (crc32 != frame->S2RAM_CRC0)
		{
			PDEBUG("mem[%08x - %08x]: CRC error!\n", (int)frame->S2RAM_START0,
				(int)(frame->S2RAM_START0 + frame->S2RAM_LEN0));
			goto FAIL;
		}
	}
	if (frame->S2RAM_START1 && frame->S2RAM_LEN1 > 0)
	{
		crc32 = GET_CRC32(frame->S2RAM_START1, frame->S2RAM_LEN1);
		PDEBUG("------mem#1 [0x%08x],CRC=0x%08x------\n", (int)frame->S2RAM_START1, (int)crc32);
		print_memory((void*)frame->S2RAM_START1, frame->S2RAM_LEN1);
		if (crc32 != frame->S2RAM_CRC1)
		{
			PDEBUG("mem[%08x - %08x]: CRC error!\n", (int)frame->S2RAM_START1,
				(int)(frame->S2RAM_START1 + frame->S2RAM_LEN1));
			goto FAIL;
		}
	}
	PDEBUG("---------------------------------------------\n");

	return 0;
FAIL:
	PDEBUG("Resume failed, enter infinite loop\n");
	while(1);
	return 1;
}
