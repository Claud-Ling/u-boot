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
#define CRC32(ptr, sz) crc32(0, (const unsigned char*)(ptr), sz)

#define read_uint32(pa) read_reg(2, pa, 0)
#define write_uint32(pa, v) write_reg(2, pa, 0, v, 0) /*for some reason, write_reg uses mask in opposite way*/

# define PMAN_SEC0_base 0x15005000
#if CONFIG_SIGMA_NR_UMACS > 1
# define PMAN_SEC1_base 0x15008000
#endif
#if CONFIG_SIGMA_NR_UMACS > 2
# define PMAN_SEC2_base 0x15036000
#endif
#if CONFIG_SIGMA_NR_UMACS > 3
# error "not support UMACS >= 3 yet!"
#endif

#define PMAN_SEC_GROUP_MAX	CONFIG_SIGMA_NR_UMACS
#define PMAN_REGION_MAX		32

#define PTAB_RGN_MAX    (PMAN_SEC_GROUP_MAX * PMAN_REGION_MAX)
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

struct ptbl_rgn_body {
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

#ifdef CONFIG_SIGMA_PST_VER_1_X

/*
 * PMAN TABLE V1.X DEFINITION
 *
--------+-----------------------+
        |  3  |  2  |  1  |  0  |
--------+-----+-----------------+------------
        |     |      magic      |
        | ver +-----+-----+-----+03~00H
   H    |     | 'T' | 'S' | 'P' |
   E    +-----+-----+-----------+------------
   A    |    dlen   |   tlen    |07~04H
   D    +-----------+-----------+------------
   E    |          dcrc         |0B~08H
   R    +-----------------------+------------
        |          hcrc         |0F~0CH
----+---+-----------------------+------------
    | r |          lsb          |13~10H
 R  | g |          size         |17~14H
    | n |          sec          |1B~18H
 E  | 0 |          attr         |1F~1CH
    +---+-----------------------+------------
 G  | r |          lsb          |23~20H
    | g |          size         |27~24H
 I  | n |          sec          |2B~28H
    | 1 |          attr         |2F~2CH
 O  +---+-----------------------+------------
    | . |          ...          |...
 N  +---+-----------------------+------------
    | r |          lsb          |10H*N+13~10H
 S  | g |          size         |10H*N+17~14H
    | n |          sec          |10H*N+1B~18H
    | N |          attr         |10H*N+1F~1CH
----+---+-----------------------+------------
 *
 *
 */
#define PTAB_MAGIC		"PST"

#define VERSION_MAJOR		1
#define VERSION_MINOR		0
#define PST_VERSION(a,b)	((((a)&0xf)<<4) | ((b)&0xf))
#define PST_VERSION_CODE	PST_VERSION(VERSION_MAJOR, VERSION_MINOR)

struct ptbl_hdr{
	char magic[3];		/*"PST"*/
	char ver;		/*version code*/
	uint16_t tlen;		/*table length, in bytes*/
	uint16_t dlen;		/*data length (region area), in bytes*/
	uint32_t dcrc;		/*checksum of regions*/
	uint32_t hcrc;		/*checksum of header*/
} __attribute__((packed));

#define PTAB_HDR_TRACE(h) do{			\
	trace_info("PMAN Secure Table (v1.x):\n"\
		"magic: %02x %02x %02x\n"	\
		"ver:   0x%02x\n"		\
		"tlen:  0x%04x\n"		\
		"dlen:  0x%04x\n"		\
		"dcrc:  0x%08x\n"		\
		"hcrc:  0x%08x\n",		\
		(h)->magic[0], (h)->magic[1],	\
		(h)->magic[2], (h)->ver,	\
		(h)->tlen, (h)->dlen,		\
		(h)->dcrc, (h)->hcrc);		\
}while(0)

/*
 * total of regions described in the table
 */
#define PTAB_TOTAL_REGIONS(t) ((t)->dlen / sizeof(struct ptbl_rgn_body))

/*
 * PTAB length
 */
#define PTAB_LENGTH(tab) ((tab)->dlen + sizeof(struct ptbl_hdr))

#else /* ifdef CONFIG_SIGMA_PST_VER_1_X */

/*
 * PMAN TABLE V0.X DEFINITION
 *
--------+-----------------------+
        |  3  |  2  |  1  |  0  |
--------+-----------------------+------------
        |        magic          |
   H    +-----+-----+-----+-----+
   E    | 'B' | 'A' | 'T' | 'P' |03~00H
   A    +-----+-----+-----+-----+------------
   D    | '!' | 'N' | 'G' | 'R' |07~04H
   E    +-----------------------+------------
   R    |       length          |0B~08H
        +-----------------------+------------
        |       crc32           |0F~0CH
----+---+-----------------------+------------
    | r |       lsb             |13~10H
 R  | g |       size            |17~14H
    | n |       sec             |1B~18H
 E  | 0 |       attr            |1F~1CH
    +---+-----------------------+------------
 G  | r |       lsb             |23~20H
    | g |       size            |27~24H
 I  | n |       sec             |2B~28H
    | 1 |       attr            |2F~2CH
 O  +---+-----------------------+------------
    | . |       ...             |...
 N  +---+-----------------------+------------
    | r |       lsb             |10H*N+13~10H
 S  | g |       size            |10H*N+17~14H
    | n |       sec             |10H*N+1B~18H
    | N |       attr            |10H*N+1F~1CH
----+---+-----------------------+------------
 *
 *
 */

#define PTAB_MAGIC		"PTABRGN!"

struct ptbl_hdr{
	char magic[8];		/*"PTABRGN!"*/
	int32_t length;		/*total length of region area, in bytes*/
	uint32_t crc;		/*checksum of region payload*/
} __attribute__((packed));

#define PTAB_HDR_TRACE(h) do{			\
	trace_info("PMAN Secure Table (v0.x):\n"\
		"magic: %02x %02x %02x %02x "	\
		"%02x %02x %02x %02x\n"		\
		"len:  0x%08x\n"		\
		"crc:  0x%08x\n",		\
		(h)->magic[0], (h)->magic[1],	\
		(h)->magic[2], (h)->magic[3],	\
		(h)->magic[4], (h)->magic[5],	\
		(h)->magic[6], (h)->magic[7],	\
		(h)->length, (h)->crc);		\
}while(0)

/*
 * total of regions described in the table
 */
#define PTAB_TOTAL_REGIONS(t) ((t)->length / sizeof(struct ptbl_rgn_body))

/*
 * PTAB length
 */
#define PTAB_LENGTH(tab) ((tab)->length + sizeof(struct ptbl_hdr))

#endif /* ifdef CONFIG_SIGMA_PST_VER_1_X */

/*
 * PTAB max length
 */
#define PTAB_MAX_LENGTH	(sizeof(struct ptbl_hdr) + sizeof(struct ptbl_rgn_body) * PTAB_RGN_MAX)

/*
 * pman table header length
 */
#define PTAB_HDR_LENGTH sizeof(struct ptbl_hdr)

/*
 * i-th region body in specified pman table
 */
#define PTAB_REGION(t, i) ((struct ptbl_rgn_body*)((t) + 1) + (i))

static struct ptbl_hdr * pman_tbl = NULL;

/*
 * PMAN secure table payload length
 */
static uint32_t pst_dlen = 0;

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
	struct ptbl_rgn_body *rgnA = (struct ptbl_rgn_body*)A;
	struct ptbl_rgn_body *rgnB = (struct ptbl_rgn_body*)B;
	if (rgnA->lsb.bits.id != rgnB->lsb.bits.id)
		return ((int)rgnA->lsb.bits.id - (int)rgnB->lsb.bits.id);
	else if (rgnA->lsb.bits.start != rgnB->lsb.bits.start)
		return ((int)rgnA->lsb.bits.start - (int)rgnB->lsb.bits.start);
	else
		return ((int)rgnA->size - (int)rgnB->size);
}

static int pman_add_one_region(void* body, size_t ofs, struct ptbl_rgn_body* prgn)
{
	int i, n;
	struct ptbl_rgn_body *rgns = NULL;

	assert(body != NULL && !(ofs % sizeof(struct ptbl_rgn_body)));
	if (SANITY_CHECK_REGION(prgn)) {
		printf("error: invalid region!\n");
		PTAB_REGION_TRACE(prgn);
		return -1;
	}

	rgns = (struct ptbl_rgn_body*)body;
	n = ofs / sizeof(struct ptbl_rgn_body);
	if (n > PTAB_RGN_MAX - 1) {
		printf("error: small pman table!!\n");
		return -2;
	}

	/* in ascending order */
	for (i = 0; i < n; i++) {
		if (pman_compr_regions(prgn, rgns + i) < 0)
			break;
	}

	if (n > i)
		memmove(rgns + i + 1, rgns + i, (n - i) * sizeof(struct ptbl_rgn_body));

	memcpy(rgns + i, prgn, sizeof(struct ptbl_rgn_body));
	return sizeof(struct ptbl_rgn_body);
}

static int setup_pman_in_tee(struct ptbl_hdr* tbl)
{
#define TO_PHY_ADDR(v) ((uint32_t)(uintptr_t)(v)) /*use 32-bit for phyaddr*/
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
	struct ptbl_rgn_body rgn;
	memset(&rgn, 0, sizeof(struct ptbl_rgn_body));
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
		pman_tbl = (struct ptbl_hdr*)MALLOC(PTAB_MAX_LENGTH);
		if (pman_tbl == NULL) {
			printf("error: failed to create pman tbl!! can't parse '%s'\n", str);
			return 3;
		}

		pst_dlen = 0; /* reset */
		memset(pman_tbl, 0, sizeof(struct ptbl_hdr));
	}

	if ((n = pman_add_one_region(pman_tbl + 1, pst_dlen, &rgn)) < 0) {
		printf("error: failed to add region '%s'\n", str);
		return 4;
	}

	pst_dlen += n;
	return 0;
}

/*
 * setup pman secure table header
 */
static void pman_tbl_set_hdr(void)
{
	if (pman_tbl != NULL) {
		memcpy(pman_tbl->magic, PTAB_MAGIC, sizeof(pman_tbl->magic));

#ifdef CONFIG_SIGMA_PST_VER_1_X
		pman_tbl->ver  = PST_VERSION_CODE;
		pman_tbl->tlen = PTAB_MAX_LENGTH;
		pman_tbl->dlen = pst_dlen;
		pman_tbl->dcrc = CRC32(pman_tbl + 1, pst_dlen);
		pman_tbl->hcrc = CRC32(pman_tbl, sizeof(struct ptbl_hdr));
#else
		pman_tbl->length = pst_dlen;
		pman_tbl->crc = CRC32(pman_tbl + 1, pst_dlen);
#endif
		PTAB_HDR_TRACE(pman_tbl);
	}
}

/*
 * deploy all PMAN settings in the table
 * and destroy the local table in the end
 */
static int pman_tbl_finish(void)
{
	if (pman_tbl != NULL) {
		if (pst_dlen > 0) {
			if (!secure_get_security_state()) {
				/*warn: program executes in NS world with bus protection on, has to use secure monitor*/
				pman_tbl_set_hdr();	/*setup header*/
				printf("setup pman tbl in tee\n");
				/*set up pman settings in another world*/
				setup_pman_in_tee(pman_tbl);
			} else {
				struct ptbl_rgn_body* rgn = NULL;
				int i, j, id = -1, n = pst_dlen / sizeof(struct ptbl_rgn_body);
				printf("setup pman tbl in local\n");
				for (i = 0, j = 0; i < n; i++, j++) {
					rgn = PTAB_REGION(pman_tbl, i);
					if (rgn->lsb.bits.id != id) {
						id = rgn->lsb.bits.id;
						j = 0;
					}
					PTAB_REGION_SET(j, rgn);
				}
			}
		}

		FREE(pman_tbl);
		pman_tbl = NULL;
	}
	return 0;
}

