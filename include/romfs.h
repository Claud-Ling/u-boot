#ifndef __ROMFS_H__
#define __ROMFS_H__

unsigned int load_romfs( unsigned int romfsaddr, unsigned int *img_addr );
unsigned int romfs_check(unsigned int *addr);

#endif
