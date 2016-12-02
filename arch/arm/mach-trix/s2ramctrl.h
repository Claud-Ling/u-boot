/*
 * s2ramctrl.h
 * This file defines struct s2ram_resume_frame
 */

#ifndef __S2RAMCTRL_H__
#define __S2RAMCTRL_H__

#ifndef __ASSEMBLY__

/*
 * s2ram resume frame structure.
 * This structure defines the way the resume entry
 * and other stuffs are stored during an s2ram operation
 */
struct s2ram_resume_frame{
	long data[8];
};

#define S2RAM_CRC	data[7]	/* checksum for s2ram resume frame struct */
#define S2RAM_CRC1	data[6]	/* checksum for memory1 */
#define S2RAM_LEN1	data[5]
#define S2RAM_START1	data[4]
#define S2RAM_CRC0	data[3]	/* checksum for memory0 */
#define S2RAM_LEN0	data[2]
#define S2RAM_START0	data[1]
#define S2RAM_ENTRY	data[0]	/* resume entry, keep at the first!! */

#ifdef CONFIG_S2RAM_CHECKSUM
extern int s2ram_set_frame(void *entry, void *mem0, unsigned len0, void *mem1, unsigned len1, struct s2ram_resume_frame *frame);
extern int s2ram_check_crc(struct s2ram_resume_frame* frame);
#endif

#endif //__ASSEMBLY__

#endif
