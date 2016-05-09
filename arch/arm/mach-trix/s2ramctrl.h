/*
 * s2ramctrl.h
 * This file defines struct s2ram_resume_frame
 */

#ifndef __S2RAMCTRL_H__
#define __S2RAMCTRL_H__

#define S2RAM_FRAME_SIZE 	32	/* sizeof struct s2ram_resume_frame */

#define	S2RAM_CRC_OFS		28	/* checksum for s2ram resume frame struct */
#define S2RAM_CRC1_OFS		24	/* checksum for memory1 */
#define S2RAM_LEN1_OFS		20
#define	S2RAM_START1_OFS	16
#define S2RAM_CRC0_OFS		12	/* checksum for memory0 */
#define S2RAM_LEN0_OFS		8
#define S2RAM_START0_OFS	4
#define S2RAM_ENTRY_OFS		0	/* resume entry, keep at the first!! */

#ifndef __ASSEMBLY__

/*
 * s2ram resume frame structure.
 * This structure defines the way the resume entry
 * and other stuffs are stored during an s2ram operation
 */
struct s2ram_resume_frame{
	long data[(S2RAM_FRAME_SIZE) >> 2];
};

#define	S2RAM_CRC	data[S2RAM_CRC_OFS >> 2]
#define S2RAM_CRC1	data[S2RAM_CRC1_OFS >> 2]
#define S2RAM_LEN1	data[S2RAM_LEN1_OFS >> 2]
#define S2RAM_START1	data[S2RAM_START1_OFS >> 2]
#define S2RAM_CRC0	data[S2RAM_CRC0_OFS >> 2]
#define S2RAM_LEN0	data[S2RAM_LEN0_OFS >> 2]
#define S2RAM_START0	data[S2RAM_START0_OFS >> 2]
#define S2RAM_ENTRY	data[S2RAM_ENTRY_OFS >> 2]

#ifdef CONFIG_S2RAM_CHECKSUM
extern int s2ram_set_frame(void *entry, void *mem0, unsigned len0, void *mem1, unsigned len1, struct s2ram_resume_frame *frame);
extern int s2ram_check_crc(struct s2ram_resume_frame* frame);
#endif

#endif //__ASSEMBLY__

#endif
