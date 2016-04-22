//#define DEBUG

#include <common.h>
#include <linux/compiler.h>
#include <stdbool.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/arch/addrspace.h>
#include <asm/arch/setup.h>
#include <asm/arch/xenvkeys.h>
#include <asm/arch/tango4.h>

DECLARE_GLOBAL_DATA_PTR;

unsigned long xenv_addr;
unsigned long *xenv_mem = NULL;

// prototypes
extern int xenv_isvalid(u32 *base, u32 maxsize);

int tangoxinit( void )
{
    /* read zxenv location */
    xenv_addr = gbus_read_uint32(pgbus, REG_BASE_cpu_block + LR_ZBOOTXENV_LOCATION);
    debug( "xenv_addr: 0x%x\n", (unsigned int)xenv_addr );
    
    if ( xenv_addr != 0 ) {
        if ( !(xenv_isvalid((void *)xenv_addr, MAX_XENV_SIZE) < 0) ) {
            /* currently setxenv and sata driver use xenv_mem global variable */
            xenv_mem = (unsigned long*)xenv_addr;
        }
    }
    return 0;
}


