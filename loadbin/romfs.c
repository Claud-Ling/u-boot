#include <common.h>
#include <bootimg.h>

#include <asm/arch/setup.h>

//#include "../ixkc/xos2k_client.h"

#define ROMFS_MAGIC_1 ('-' + ('r'<<8) + ('o'<<16) + ('m'<<24))
#define ROMFS_MAGIC_2 ('1' + ('f'<<8) + ('s'<<16) + ('-'<<24))

#define MAX_ROMFS_SIZE  0x04000000  /* maximum size of any given ROMFS container */
#define MAX_FNAME_SIZE  256	    /* maximum length of file name */

typedef struct {
    unsigned int next;
    unsigned int info;
    unsigned int size;
    unsigned int crc;
} file_hdr;

//static char *romfs_found_msg = "ROMFS found at 0x%p, Volume name = %s\n";
//static char *romfs_none_msg = "ROMFS not found from 0x%x to 0x%x\n";
//static char *romfs_chk_msg = "ROMFS Check failed\n";


static inline unsigned int swapl(unsigned int x)
{
    return ((unsigned long int)((((unsigned long int)(x) & 0x000000ffU) << 24) |
        (((unsigned long int)(x) & 0x0000ff00U) <<  8) |
        (((unsigned long int)(x) & 0x00ff0000U) >>  8) |
        (((unsigned long int)(x) & 0xff000000U) >> 24)));
}


/* Check the romfs starting at 'addr' (with -romfs- marker) */
unsigned int romfs_check(unsigned int *addr)
{
    return (((*addr) == ROMFS_MAGIC_1) && ((*(addr + 1)) == ROMFS_MAGIC_2)) ? 1 : 0;
}

/* Lookup file in romfs and return file address (or 0 if no such file) */
static file_hdr *romfs_lookup(unsigned int *romfs, char *name, file_hdr *hdrprev)
{
    char *filename;
    char *volume;
    file_hdr *hdr;
    unsigned int next, max_romfs_size;

    max_romfs_size = MAX_ROMFS_SIZE;

    if (hdrprev == NULL) {
        volume = (char *)(romfs + 4); /* +4 32bits words */

        while (*volume)
            volume++;
	
        volume += 1; /* must account for the terminating null character */
        
        hdr = (file_hdr *)(((unsigned int) volume + 15) & ~0xF);
        
    } 
    else {
        
        next = (swapl(hdrprev->next) & ~0xF);
        
        if ((next == 0) || (next >= max_romfs_size))
            return NULL;
        
        hdr = (file_hdr *) (romfs+(next/sizeof(unsigned int)));
    }

    while (1) {
        
        filename = ((char *) hdr) + 16;
        next = (swapl(hdr->next) & ~0xF);

        if (name == NULL) 
            return hdr;
            
        else if ((strnlen(filename, MAX_FNAME_SIZE) <= MAX_FNAME_SIZE) && (strcmp(name, filename) == 0)) {
	    
            printf("File %s found\n", name);
            return hdr;
        } 
        else if ((next == 0) || (next >= max_romfs_size)) {
            printf("File %s not found\n", name);
            return NULL;
        }

        hdr = (file_hdr *) (romfs+(next/sizeof(unsigned int)));
    }
    
    return NULL;
}

unsigned int load_romfs( unsigned int romfsaddr, unsigned int *img_addr )
{
	unsigned int romfs_addr;
    file_hdr *hdr;
    
    char *fname;
    unsigned int next_hdr_offset;

    boot_img_hdr *android_hdr;
	    

    romfs_addr = romfsaddr;

    if ((hdr = romfs_lookup((unsigned int *)romfs_addr, NULL, NULL)) == NULL) 
		return -1;
		
    /* we just got the pointer of the first file header */
    while(1) {

        next_hdr_offset =  swapl(hdr->next) & ~0xf;
        
        if ( next_hdr_offset == 0 )
            break;
        
        /* set next header address */
        hdr = (file_hdr*)(romfsaddr + next_hdr_offset);
    }
    
    /* skip file header, spec.info, size, checksum */
    fname = ((char*)hdr)+16;
    
    /* skip file name */
    android_hdr = (boot_img_hdr *)(fname + (((strlen(fname)+15)/16)*16) );
    
    /* check to see if the image is android boot image */
    if ( strncmp( (char*)(android_hdr->magic), BOOT_MAGIC, BOOT_MAGIC_SIZE ) == 0 ) {
        
        /* return the address of the boot.img address */
        *img_addr = (unsigned int)android_hdr;
    }
    else {
        /* image is not an android boot.img */
        return -1;
    }

    return 0;
}

unsigned int load_kernel_romfs( unsigned int romfsaddr, unsigned int *img_addr )
{
	unsigned int romfs_addr;
    file_hdr *hdr;
    
    char *fname;
    unsigned int next_hdr_offset;

    romfs_addr = romfsaddr;

    if ((hdr = romfs_lookup((unsigned int *)romfs_addr, NULL, NULL)) == NULL) 
        return -1;
		
    /* we just got the pointer of the first file header */
    while(1) {

        next_hdr_offset =  swapl(hdr->next) & ~0xf;
        
        if ( next_hdr_offset == 0 )
            break;
        
        /* set next header address */
        hdr = (file_hdr*)(romfsaddr + next_hdr_offset);
    }
    
    /* skip file header, spec.info, size, checksum */
    fname = ((char*)hdr)+16;
    
    *img_addr = (unsigned int)(fname + (((strlen(fname)+15)/16)*16) );    

    return swapl(hdr->size);
}
