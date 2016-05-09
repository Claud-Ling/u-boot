/***************************************************************************
*
*           Copyright (c) 2015 Sigma Designs, Inc.
*                       All rights reserved
*
* The content of this file or document is CONFIDENTIAL and PROPRIETARY
* to Sigma Designs, Inc.  It is subject to the terms of a
* License Agreement between Licensee and Sigma Designs, Inc.
* restricting among other things, the use, reproduction, distribution
* and transfer.  Each of the embodiments, including this information and
* any derivative work shall retain this copyright notice
*
****************************************************************************
*
* File:		pman_protection.c
*
* Purpose:	deploy pman protection settings from initsetting.file, support
* 		both secure and none-secure executing environment.
* 		pman setting entry shall be given in format:
* 		p <id> <addr> <size> <sec> <attr>
*
* Author:	Tony He (tony_he@sigmadesigns.com)
* Date:		2015-02-08
****************************************************************************/

#include <asm/arch/setup.h>

#define MALLOC(n) malloc(n)
#define FREE(p) free(p)
#define CRC32(seed, ptr, sz) crc32(seed, ptr, sz)

#define read_uint32(pa) read_reg(2, pa, 0)
#define write_uint32(pa, v) write_reg(2, pa, 0, v, 0) /*for some reason, write_reg uses mask in opposite way*/

#if defined (CONFIG_MACH_SIGMA_SX6) /*SX6*/
# define PMAN_SEC0_base 0x15005000
# define PMAN_SEC1_base 0x15008000
#else /*SX7...*/
# define PMAN_SEC0_base 0x15005000
# define PMAN_SEC1_base 0x15008000
# define PMAN_SEC2_base 0x15036000
#endif

#define CFG_MAX_PMAN_REGIONS    (3 * 32)
#define PMAN2PHYADDR(a)	((a) << 12)
#define PHY2PMANADDR(a)	(((a) >> 12) & ((1<<20) - 1))	/*in 4kB*/

#define LOG2_PMAN_REGION_GRANULARITY	12
#define PMAN_REGION_GRANULARITY		(1 << LOG2_PMAN_REGION_GRANULARITY)

#define ALIGNTO(v, nbits) (((v) >> (nbits)) << (nbits))
#define ALIGNTONEXT(v, nbits) ALIGNTO((v) + ((1 << (nbits)) - 1), nbits)

#ifdef DEBUG
#define trace_info(fmt...) do{	\
	printf(fmt);		\
}while(0)
#else
#define trace_info(fmt...) do{}while(0)
#endif
#define trace_error(fmt...) do{	\
	printf(fmt);		\
}while(0)

#define PTAB_REGION_TRACE(e) do{							\
	trace_info("PMAN region: pman%d 0x%08x 0x%08x 0x%08x 0x%08x\n", 		\
	(e)->lsb.bits.id, (e)->lsb.bits.start, (e)->size, (e)->sec, (e)->attr);		\
}while(0)

/*
 * return 0 on valid, non-zero otherwise
 */
#define SANITY_CHECK_REGION(e) ({			\
	int _ret = 0;					\
	_ret |= !pman_sec_base((e)->lsb.bits.id);/*id*/	\
	_ret |= !((e)->size > 0); 	/*size*/	\
	_ret |= !((e)->attr & 0x1); 	/*valid*/	\
	_ret;						\
})

#define PTAB_REGION_SET(i, e) do{							\
	uint32_t _start = PMAN2PHYADDR((e)->lsb.bits.start);				\
	uint32_t _end = ALIGNTO((_start+(e)->size-1), LOG2_PMAN_REGION_GRANULARITY);	\
	uint32_t _base = pman_sec_base((e)->lsb.bits.id);				\
	trace_info("==>pman%d[%02d]: 0x%08x 0x%08x 0x%08x 0x%08x\n", 			\
	(e)->lsb.bits.id, i, _start, _end, (e)->sec, (e)->attr);			\
	write_uint32(_base + 0x100 + 0x20 * i, _start); 	/*low*/			\
	write_uint32(_base + 0x104 + 0x20 * i, _end); 		/*high*/		\
	write_uint32(_base + 0x108 + 0x20 * i, (e)->sec); 	/*sec_access*/		\
	write_uint32(_base + 0x10c + 0x20 * i, (e)->attr); 	/*attr*/		\
}while(0)

#define PTAB_LENGTH(tab) ((tab)->length + sizeof(struct ptbl_hdr))

struct ptab_rgn_body {
	union {
		struct { __extension__ uint32_t
			id: 3		/*pman id*/,
			hole0: 9	/*reserved*/,
			start: 20	/*start addr, in 4k granularity*/;
		} bits;
		uint32_t val;
	} lsb;
	uint32_t size;	/*size in bytes, must be multiple of 4k*/
	uint32_t sec;	/*security access*/
	uint32_t attr;	/*attributes*/
};

struct ptbl_hdr{
	char magic[8];		/*"PTABRGN!"*/
	int32_t length;		/*total length of region area, in bytes*/
	uint32_t crc;		/*checksum of region payload*/
	struct ptab_rgn_body rgns[0];
};

static struct ptbl_hdr * pman_tbl = NULL;

/*
 * pman security base address
 * return base address on success. Otherwise 0
 */
static uint32_t pman_sec_base(int id)
{
	int base = 0;
	switch(id) {
#ifdef PMAN_SEC0_base
	case 0: base = PMAN_SEC0_base; break;
#endif
#ifdef PMAN_SEC1_base
	case 1: base = PMAN_SEC1_base; break;
#endif
#ifdef PMAN_SEC2_base
	case 2: base = PMAN_SEC2_base; break;
#endif
	default:
		break;
	}
	return base;
}

static int pman_compr_regions(const void* A, const void* B)
{
	struct ptab_rgn_body *rgnA = (struct ptab_rgn_body*)A;
	struct ptab_rgn_body *rgnB = (struct ptab_rgn_body*)B;
	if (rgnA->lsb.bits.id != rgnB->lsb.bits.id)
		return ((int)rgnA->lsb.bits.id - (int)rgnB->lsb.bits.id);
	else
		return ((int)rgnA->lsb.bits.start - (int)rgnB->lsb.bits.start);
}

static int pman_add_one_region(struct ptbl_hdr* tbl, struct ptab_rgn_body* rgn)
{
	int i, n = tbl->length / sizeof(struct ptab_rgn_body);

	if (SANITY_CHECK_REGION(rgn)) {
		printf("error: invalid region!\n");
		PTAB_REGION_TRACE(rgn);
		return -1;
	}

	if (n > CFG_MAX_PMAN_REGIONS - 1) {
		printf("error: small pman table!!\n");
		return -2;
	}

	for (i=n; i>0; i--) {
		if (pman_compr_regions(rgn, &tbl->rgns[i-1]) >= 0)
			break;
	}

	if (n - i > 0)
		memmove(&tbl->rgns[i+1], &tbl->rgns[i], (n - i) * sizeof(struct ptab_rgn_body));

	if (i < 0)
		i = 0;

	memcpy(&tbl->rgns[i], rgn, sizeof(struct ptab_rgn_body));
	tbl->length += sizeof(struct ptab_rgn_body);
	return i;
}

static int setup_pman_by_armor(struct ptbl_hdr* tbl)
{
#define TO_PHY_ADDR(v) ((uint32_t)(v))
	flush_cache((ulong)tbl, PTAB_LENGTH(tbl));
	return secure_set_mem_protection(TO_PHY_ADDR(tbl), PTAB_LENGTH(tbl));
#undef TO_PHY_ADDR
}

static int pman_get_params(char* str, int *id, int *addr, uint32_t *sz, uint32_t *sec, uint32_t *attr)
{
	int ret = 0;
	const char* delim = " \r\t\n";
	char *p, *t = strdup(str);

	if ((p = strtok(t, delim)) == NULL)
		goto ERROR;
	*id = (int)simple_strtoul(p, 0, 16);
	ret++;

	if ((p = strtok(NULL, delim)) == NULL)
		goto ERROR;
	*addr = (int)simple_strtoul(p, 0, 16);
	ret++;
	
	if ((p = strtok(NULL, delim)) == NULL)
		goto ERROR;
	*sz = simple_strtoul(p, 0, 16);
	ret++;

	if ((p = strtok(NULL, delim)) == NULL)
		goto ERROR;
	*sec = simple_strtoul(p, 0, 16);
	ret++;

	if ((p = strtok(NULL, delim)) == NULL)
		goto ERROR;
	*attr = simple_strtoul(p, 0, 16);
	ret++;

	free(t);
	t = NULL;
	return ret;

ERROR:
	free(t);
	t = NULL;
	printf("input error (%d)!! it should be <id> <addr> <sz> <sec> <attr>\n", ret);
	return -1;
}

/*
 * read in one pman entry
 * input str should be given in format: <id> <addr> <sz> <sec> <attr>
 */
static int pman_tbl_new_entry(char* str)
{
	int n, tid, taddr;
	char *p = str; /*<id> <addr> <sz> <sec> <attr>*/
	struct ptab_rgn_body rgn;
	memset(&rgn, 0, sizeof(struct ptab_rgn_body));
	n = pman_get_params(p, &tid, &taddr, &rgn.size, &rgn.sec, &rgn.attr);
	if (n != 5) {
		printf("incomplete entry: '%s'\n", str);
		return 1;
	}
	rgn.lsb.bits.id = tid;
	rgn.lsb.bits.start = PHY2PMANADDR(taddr);

	if (SANITY_CHECK_REGION(&rgn)) {
		printf("ignore invalid region: 'p %s'\n", str);
		return 2;
	}

	PTAB_REGION_TRACE(&rgn);

	if (!pman_tbl) {
		pman_tbl = (struct ptbl_hdr*)MALLOC(sizeof(struct ptbl_hdr) + CFG_MAX_PMAN_REGIONS * sizeof(struct ptab_rgn_body));
		if (pman_tbl == NULL) {
			printf("error: failed to create pman tbl!! can't parse '%s'\n", str);
			return 3;
		}

		memset(pman_tbl, 0, sizeof(struct ptbl_hdr));
		memcpy(pman_tbl->magic, "PTABRGN!", 8);
	}

	if (pman_add_one_region(pman_tbl, &rgn) < 0) {
		printf("error: failed to add region '%s'\n", str);
		return 4;
	}

	return 0;
}

/*
 * deploy all PMAN settings in the table
 * and destroy the local table in the end
 */
static int pman_tbl_finish(void)
{
	if (pman_tbl != NULL) {
		printf("setup pman tbl then...\n");
		if (pman_tbl->length > 0) {
			int i, j, id = -1, n = pman_tbl->length / sizeof(struct ptab_rgn_body);
			if (!secure_get_security_state()) {
				/*warn: program executes in NS world with bus protection on, has to use secure monitor*/
				pman_tbl->crc = CRC32(-1, (void*)pman_tbl->rgns, pman_tbl->length);
				printf("let armor set pman tbl: len=%#x, crc=%08x\n", pman_tbl->length, pman_tbl->crc);
				/*set up pman settings in another world*/
				setup_pman_by_armor(pman_tbl);
			} else {
				id = -1;
				for (i=0,j=0; i<n; i++,j++) {
					if (pman_tbl->rgns[i].lsb.bits.id != id) {
						id = pman_tbl->rgns[i].lsb.bits.id;
						j = 0;
					}
					PTAB_REGION_SET(j, &pman_tbl->rgns[i]);
				}
			}
		}

		FREE(pman_tbl);
		pman_tbl = NULL;
	}
	return 0;
}

