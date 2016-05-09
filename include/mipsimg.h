/*
**   @file       tools/mkmipsimg/mipsimg.h
**   @author     tony.he
**   @version
**   @date    Jan 2nd 2014
**
**   @if copyright_display
**               Copyright (c) 2014 SigmaDesigns, Inc.
**               All rights reserved
**
**               The content of this file or document is CONFIDENTIAL and PROPRIETARY
**               to SigmaDesigns, Inc.  It is subject to the terms of a
**               License Agreement between Licensee and SigmaDesigns, Inc.
**               restricting among other things, the use, reproduction, distribution
**               and transfer.  Each of the embodiments, including this information and
**               any derivative work shall retain this copyright notice.
**   @endif
**
**       More description ...
**
**   @if modification_History
**
**       <b>Modification History:\n</b>
**       Date                            Name                    Comment \n\n
**       Aug 17th 2015                   tony.he                 support multiple setting_cfg.xml
**
**
**   @endif
*/

#ifndef _MIPS_IMAGE_H_
#define _MIPS_IMAGE_H_

typedef struct mips_img_hdr mips_img_hdr;

#define MIPS_MAGIC "MIPSIMG!"
#define MIPS_MAGIC_SIZE 8
#define MIPS_NAME_SIZE 16

#define MIPS_PAYLOAD_MAX 12
#define PAYLOAD_NAME_LEN 32
#define MAX_NR_UMACS	3
struct mips_img_hdr
{
    unsigned char magic[MIPS_MAGIC_SIZE];
    struct payload{
        unsigned size;  /* payload size in bytes */
        unsigned addr;  /* payload physical load addr */
        unsigned type;  /* payload type */
#       define PT_DATA       1       //bit[0]
#       define PT_EXTRA_MASK 0x1E    //bit[4..1]
#       define PT_TYPE_MASK  (PT_DATA | PT_EXTRA_MASK)
#       define PT_EXTRA_AV   (1<<1)
#       define PT_EXTRA_DISP (2<<1)
#       define PT_EXTRA_DB   (3<<1)
#       define PT_EXTRA_SP   (4<<1)
#       define PT_EXTRA_CFG  (5<<1)
#       define PT_EXTRA_ST   (6<<1)
#       define PT_EXTRA_LOGO (7<<1)
#       define PT_AV    (PT_DATA | PT_EXTRA_AV)
#       define PT_DISP  (PT_DATA | PT_EXTRA_DISP)
#       define PT_DB    (PT_DATA | PT_EXTRA_DB)
#       define PT_SP    (PT_DATA | PT_EXTRA_SP)
#       define PT_CFG   (PT_DATA | PT_EXTRA_CFG)
#       define PT_ST    (PT_DATA | PT_EXTRA_ST)
#       define PT_LOGO  (PT_DATA | PT_EXTRA_LOGO)
#       define PT_MSZ_SHIFT 8       //bit[19..8]
#       define PT_MSZ_MASK  (((1 << (MAX_NR_UMACS * 4)) - 1) << PT_MSZ_SHIFT)
#       define PT_GET_TYPE(t)  ((t) & PT_TYPE_MASK)
#       define PT_GET_MSIZE(t) (((t) & PT_MSZ_MASK) >> PT_MSZ_SHIFT)

        char name[PAYLOAD_NAME_LEN];      /* payload name */
    }data[MIPS_PAYLOAD_MAX];

    unsigned page_size;    /* flash page size we assume */
    unsigned mcfg_ofs;     /* setting.xml offset (to host image start addr) */
    unsigned panelnode_ofs;/* 'panel' node offset in setting.xml */
    unsigned unused[3];    /* future expansion: should be 0 */

    unsigned char target_ending;     /* target endingness: 0-LE, 1-BE */
    unsigned char panel_type;        /* panel type, give 255 to get val from 'paneltype' */
    unsigned char mcfglen_size;      /* sizeof setting.xml length */
    unsigned char name[MIPS_NAME_SIZE]; /* asciiz product name */

    unsigned id[8]; /* timestamp / checksum / sha1 / etc */
};

/*
** +------------------+ 
** | mipsimg header   | 1 page
** +------------------+
** | payload#0        | n pages  
** +------------------+
** | payload#1        | m pages  
** +------------------+ 
** | ......           | ......  
** +------------------+ 
** | payload#n        | s pages
** +------------------+ 
**
** n = (data[0].size + page_size - 1) / page_size
** m = (data[1].size + page_size - 1) / page_size
** s = (data[n].size + page_size - 1) / page_size
**
** 0. page_size could be either 2048 or 4096, default 2048
** 1. all entities are page_size aligned in flash
** 2. av, dispmips, database.tse, sound_preset and 
**    initsetting.file are required (size != 0)
** 3. setting_cfg.xml will be filled in av and disp image at
**    offset=mcfg_ofs, in layout "file_len | file"
** 4. db_addr and sp_addr are required
** 5. av_addr and disp_addr are optional, as they can derived 
**    from bin files
** 6. load each element (av, disp, database.tse, sound_preset)
**    to the specified physical address (av_addr, etc)
** 7. target_ending specify mips endingness
**    give 0 for little ending or 1 for big ending, and others are undefined
** 8. panel_type specifies current panel definition in setting.xml
**    give 255 to force uboot get panel definition from uboot env
**    'paneltype' or board config data
** 
*/

#endif
