#include <common.h>
#include <asm/arch/setup.h>

struct channel_control;
extern RMstatus cs_xos2k_xload(struct gbus*,
    struct channel_control*, RMuint32, RMuint32, RMuint32, RMuint32);
extern RMstatus sc_xos2k_xload_u(struct gbus*, 
    struct channel_control*, RMstatus*);

int decrypt_xload( unsigned int load_addr, unsigned int count )
{
    
    extern struct channel_control *cctrl_cs;
    extern struct channel_control *cctrl_sc;
    unsigned long cookie=1;
    RMstatus rc  = RM_ERROR;
    RMstatus xrc = RM_ERROR;
    
    unsigned int g_base_addr = load_addr;
    unsigned int g_load_addr = load_addr;
    

    if (cctrl_cs == NULL)
        cctrl_cs = (void*)(REG_BASE_cpu_block+LR_XOS2K_C2X);
        
    if (cctrl_sc == NULL)
        cctrl_sc = (void*)(REG_BASE_cpu_block+LR_XOS2K_X2C);

    
    //printf("cctrl_cs=0x%08x,cctrl_sc=0x%08x\n",(UINT32)cctrl_cs,(UINT32)cctrl_sc);
    printf("g_load_addr=0x%08x,g_base_addr=0x%08x,count=0x%08x\n",g_load_addr, g_base_addr, count);

    //if ((zhdr->attributes & ZBOOT_ATTR_GZIP))
    //    g_load_addr = 0; // XKC server won't do the copy to g_load_addr after decrypting the xload data in-place

    if ( cs_xos2k_xload(0, cctrl_cs, cookie, g_load_addr, g_base_addr, count) != RM_OK) {
        printf("cs_xos2k_xload failed!!\n");
        return -1;
    }

    printf("Waiting for XLOAD completion.\n");

    while ((rc=sc_xos2k_xload_u(0, cctrl_sc, &xrc))==RM_PENDING);
        
    if (rc!=RM_OK) {
        
        printf("sc_xos2k_xload_u failed (rc:%d)!!\n", rc);
        return -1;
    }
    
    if (xrc == 0x6) {
        printf("OK.\n");
    }
    else {
        printf("XLOAD done. xrc = %d .. ", xrc);
        printf("NOT OK.\n");
        return -1;
    }

    return 0;
}
