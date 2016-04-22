/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#define CMD_XKC_DEBUG

#include <common.h>
#include <command.h>
#include <watchdog.h>

#if defined (CONFIG_TANGO3) || defined (CONFIG_TANGO4)
#include <asm/arch/setup.h>
#endif

#include "../ixkc/channel.h"
#include "../ixkc/xos2k_client.h"

#ifdef	CMD_XKC_DEBUG
#define	DEBUG(fmt,args...)	printf (fmt ,##args)
#else
#define DEBUG(fmt,args...)
#endif

extern RMuint32 sigmaremap(RMuint32);
extern unsigned long tangox_dma_address(unsigned long);

#define XKC_TMP_MEM_ADDR    LOADADDR_HEAP_UNZIP
#define MAX_XKC_TMP_MEM_SIZE    0x00e00000
#define pgbus                   ((struct gbus *)0)
#define PMEM_BASE_xpu_block     0x1e0000

/*
  gbus slaves protections
  G0 xpu
  G1 dsps
  G2 cpu
  G3 host

  dma slaves protection [dma is understood as: mbus, vbus and direct load port transfers]
  D1 dsps
  D2 video out
  D3 host
*/
#define ac_G3r (1<<31)
#define ac_G3w (1<<30)
#define ac_G2r (1<<29)
#define ac_G2w (1<<28)
#define ac_G1r (1<<27)
#define ac_G1w (1<<26)
#define ac_G0r (1<<25)
#define ac_G0w (1<<24)
#define ac_D3r (1<<23)
#define ac_D3w (1<<22)
#define ac_D2r (1<<21)
#define ac_D2w (1<<20)
#define ac_D1r (1<<19)
#define ac_D1w (1<<18)
#define ac_D0r (1<<17)
#define ac_D0w (1<<16)

/* shortcuts */
#define ac_Gr   (ac_G3r|ac_G2r|ac_G1r|ac_G0r)
#define ac_Gw   (ac_G3w|ac_G2w|ac_G1w|ac_G0w)
#define ac_Grw  (ac_Gr|ac_Gw)

#define ac_Dr   (ac_D3r|ac_D2r|ac_D1r|ac_D0r)
#define ac_Dw   (ac_D3w|ac_D2w|ac_D1w|ac_D0w)
#define ac_Drw  (ac_Dr|ac_Dw)

#define ac_G01rw (ac_G1w|ac_G0w|ac_G1r|ac_G0r)



struct pregs {
    unsigned int ma_system_block[1];
    unsigned int ma_dram_controller_0[6];
    unsigned int ma_dram_controller_1[6];
    unsigned int ma_cpu_block[1];
    unsigned int ma_display_block[4];
    unsigned int ma_mpeg_engine_0[4];
    unsigned int ma_mpeg_engine_1[4];
    unsigned int ma_audio_engine_0[4];
    unsigned int ma_audio_engine_1[4];
    unsigned int ma_demux_engine_0[4];
    unsigned int ma_demux_engine_1[4];
    unsigned int ma_xpu_block[4];
    unsigned int ma_ipu_block[1];

    unsigned int ra_system_block[5];
    unsigned int ra_dram_controller_0[5];
    unsigned int ra_dram_controller_1[5];
    unsigned int ra_cpu_block[5];
    unsigned int ra_display_block[5];
    unsigned int ra_mpeg_engine_0[5];
    unsigned int ra_mpeg_engine_1[5];
    unsigned int ra_audio_engine_0[5];
    unsigned int ra_audio_engine_1[5];
    unsigned int ra_demux_engine_0[5];
    unsigned int ra_demux_engine_1[5];
    unsigned int ra_xpu_block[5];
    unsigned int ra_ipu_block[5];
};

struct channel_control *cctrl_cs=NULL;
struct channel_control *cctrl_sc=NULL;


static void printprot(RMuint32 v)
{
    unsigned long relevant=v&(ac_Grw|ac_Drw);

    if (relevant==0) {
    printf(" denied\n"); // no one can access even xpu. Used for hw with looping effect (end of dram)
    return;
    }
    if (relevant==(ac_Grw|ac_Drw)) {
    printf(" noprot\n"); // not protected at all
    return;
    }
    if (relevant==ac_Grw) {
    printf(" nogprot\n"); // not protected from gbus access
    return;
    }
    if (relevant==(ac_G0r|ac_G0w)) {
     printf(" xpurwonly\n");
    return;
    }
#define DO(i)               \
    do {                \
    if (v&i) {          \
        printf(" "#i);      \
    }               \
    } while (0)

    DO(ac_G3r);
    DO(ac_G3w);
    DO(ac_G2r);
    DO(ac_G2w);
    DO(ac_G1r);
    DO(ac_G1w);
    DO(ac_G0r);
    DO(ac_G0w);
    DO(ac_D3r);
    DO(ac_D3w);
    DO(ac_D2r);
    DO(ac_D2w);
    DO(ac_D1r);
    DO(ac_D1w);
    DO(ac_D0r);
    DO(ac_D0w);

    printf("\n");
}


static void dumpbusprot(struct pregs *r)
{
    unsigned long last,v;

#undef DO
#define DO(i,j)                                                         \
    do {                                \
    RMuint32 cur;                           \
                                    \
    v=r->ma_dram_controller_ ##i[j];                \
    cur=(v&(ac_D0w-1))<<14;                     \
    if (cur==0) {                           \
        printf("(%d) [0x%08lx,...       [ is",j,last);      \
        printprot(v);                       \
    }                               \
    else {                              \
        cur+=MEM_BASE_dram_controller_ ##i;             \
        printf("(%d) [0x%08lx,0x%08lx[ is",j,last,cur);     \
        printprot(v);                       \
        last=cur;                           \
    }                               \
    } while (0)

    printf("DRAM0 data\n");
    last=MEM_BASE_dram_controller_0;
    DO(0,0);
    DO(0,1);
    DO(0,2);
    DO(0,3);
    DO(0,4);
    DO(0,5);

    printf("DRAM1 data\n");
    last=MEM_BASE_dram_controller_1;
    DO(1,0);
    DO(1,1);
    DO(1,2);
    DO(1,3);
    DO(1,4);
    DO(1,5);

#undef DO
#define DO(i,j)                             \
    do {                                \
    unsigned long cur;                         \
                                    \
    v=r->ma_ ##i[j];                        \
    cur=v&(ac_D1w-1);                       \
        if (cur==0) {                       \
        printf("(%d) [0x%08lx,...       [ is",j,last);      \
        printprot(v);                       \
        }                               \
        else {                          \
        cur+=PMEM_BASE_ ##i;                    \
        printf("(%d) [0x%08lx,0x%08lx[ is",j,last,cur);     \
        printprot(v);                       \
        last=cur;                       \
        }                               \
    } while (0)

#define DO2(x)                              \
    do {                                \
    printf(#x " data\n");                       \
    last=PMEM_BASE_ ##x;                        \
    DO(x,0);                            \
    DO(x,1);                            \
    DO(x,2);                            \
    DO(x,3);                            \
    } while (0)

    DO2(mpeg_engine_0);
    DO2(mpeg_engine_1);
    DO2(audio_engine_0);
    DO2(audio_engine_1);
    DO2(demux_engine_0);
    DO2(demux_engine_1);
    DO2(display_block);
    DO2(xpu_block);

    printf("\n");

#undef DO
#define DO(i,j)                             \
    do {                                \
    unsigned long cur;                         \
                                    \
    v=r->ra_ ##i[j];                        \
    cur=v&(ac_D1w-1);                       \
    if (cur==0) {                           \
        printf("(%d) [0x%08lx,...       [ is",j,last);      \
        printprot(v);                       \
    }                               \
    else {                              \
        cur+=REG_BASE_ ##i;                     \
        printf("(%d) [0x%08lx,0x%08lx[ is",j,last,cur);     \
        printprot(v);                       \
        last=cur;                           \
    }                               \
    } while (0)

#undef DO2
#define DO2(x)                              \
    do {                                \
    printf(#x " regs\n");                       \
    last=REG_BASE_ ##x;                     \
    DO(x,0);                            \
    DO(x,1);                            \
    DO(x,2);                            \
    DO(x,3);                            \
    DO(x,4);                            \
    } while (0)

    DO2(system_block);
    /* DO2(host_interface); HAS NONE YET. */
    DO2(dram_controller_0);
    DO2(dram_controller_1);
    DO2(cpu_block);
    DO2(display_block);
    DO2(mpeg_engine_0);
    DO2(mpeg_engine_1);
    DO2(audio_engine_0);
    DO2(audio_engine_1);
    DO2(demux_engine_0);
    DO2(demux_engine_1);
    DO2(xpu_block);
    DO2(ipu_block);
}



static int simple_strtochar( const char* cp, char* val )
{
    if ( strlen(cp) > 1 )
        return -1;
    
    if ( *cp < '0' || *cp > 'z' )
        return -1;

    *val = *cp;

    return 0;
}

int do_xkc ( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    unsigned int rc = 0;

    if (cctrl_cs == NULL)
         cctrl_cs = (void*)(REG_BASE_cpu_block+LR_XOS2K_C2X);
    if (cctrl_sc == NULL)
        cctrl_sc = (void*)(REG_BASE_cpu_block+LR_XOS2K_X2C);

    if ( argc < 2 )
        return cmd_usage(cmdtp);

    if (strcmp(argv[1],"xpfree")==0) {

         unsigned long ga;

         if (argc==3) {

            if ((strict_strtoul(argv[2], 16, &ga) < 0) && 
                (strict_strtoul(argv[2], 10, &ga) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }

            if (cs_xos2k_xpfree(pgbus,cctrl_cs,ga)!=RM_OK) {
                printf("cs_xos2k_xpfree failed!!\n");
                goto wayout;
            }
            
            while ((rc=sc_xos2k_xpfree_u(pgbus,cctrl_sc))==RM_PENDING);
            
            if (rc!=RM_OK) {
                printf("sc_xos2k_xpfree_u failed!!\n");
                goto wayout;
            }
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"xpalloc")==0) {

        unsigned long size, ga, actualorder;

        if (argc==3) {
            if ((strict_strtoul(argv[2], 16, &size) < 0) && 
                (strict_strtoul(argv[2], 10, &size) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if (cs_xos2k_xpalloc(pgbus,cctrl_cs,size)!=RM_OK) {
                printf("cs_xos2k_xpalloc failed!!\n");
                goto wayout;
            }
    
            while ((rc=sc_xos2k_xpalloc_u(pgbus,cctrl_sc,&ga,&actualorder))==RM_PENDING);
    
            if (rc!=RM_OK) {
                printf("sc_xos2k_xpalloc_u failed!!\n");
                goto wayout;
            }
            printf("ga = 0x%08lx\n", ga);
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"pbusopen")==0) {
     
        RMstatus xrc;
    
        if (argc==2) {
     
            if (cs_xos2k_pbusopen(pgbus,cctrl_cs)!=RM_OK) {
                printf("cs_xos2k_pbusopen failed!!\n");
                goto wayout;
            }
     
            while ((rc=sc_xos2k_pbusopen_u(pgbus,cctrl_sc,&xrc))==RM_PENDING);
           
            if (rc!=RM_OK) {
                printf("sc_xos2k_pbusopen_u failed!!\n");
                goto wayout;
            }
            
            if (xrc != 6)
                printf("pbusopen failed. status = 0x%x\n", xrc);
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }


    if (strcmp(argv[1],"hello")==0) {

        unsigned long server_version;
        if (argc==2) {

            if (cs_xos2k_hello(pgbus,cctrl_cs,XOS2K_PROTOCOL_VERSION)!=RM_OK) {
                printf("cs_xos2k_hello failed!!\n");
                goto wayout;
            }
            
            while ((rc=sc_xos2k_hello_u(pgbus,cctrl_sc,&server_version))==RM_PENDING);
            
            if (rc!=RM_OK) {
                printf("sc_xos2k_hello_u failed!!\n");
                goto wayout;
            }

            if (server_version!=XOS2K_PROTOCOL_VERSION) {
                printf("%s: protocol version mismatch\n", argv[0]);
                goto wayout;
            }
            printf("xkc protocol version: %u\n", (unsigned int)server_version);
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"protregs")==0) {
        struct protregs r;
        struct pregs regs;
        if (argc==2) {
#define PROTREGS(x)                                     \
    do {                                        \
         if (cs_xos2k_protregs(pgbus,cctrl_cs, (REG_BASE_ ##x)>>16)!=RM_OK) {   \
             printf("cs_xos2k_protregs failed!!\n");                \
             goto wayout;                           \
         }                                  \
         while ((rc=sc_xos2k_protregs_u(pgbus,cctrl_sc,&r))==RM_PENDING);   \
         if (rc!=RM_OK) {                           \
             printf("sc_xos2k_protregs_u failed!!\n");              \
             goto wayout;                           \
         }                                  \
         memcpy((regs.ma_ ##x), r.mem_ac, sizeof(regs.ma_ ##x));        \
         memcpy((regs.ra_ ##x), r.reg_ac, sizeof(regs.ra_ ##x));        \
    } while (0)

            PROTREGS(system_block);
            /* PROTREGS(host_interface); HAS NONE YET. */
            PROTREGS(dram_controller_0);
            PROTREGS(dram_controller_1);
            PROTREGS(cpu_block);
            PROTREGS(display_block);
            PROTREGS(mpeg_engine_0);
            PROTREGS(mpeg_engine_1);
            PROTREGS(audio_engine_0);
            PROTREGS(audio_engine_1);
            PROTREGS(demux_engine_0);
            PROTREGS(demux_engine_1);
            PROTREGS(xpu_block);
            PROTREGS(ipu_block);

            dumpbusprot(&regs);
            goto runit;
        }
        else{
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"chpll")==0) {
         unsigned long pll0, mux;

         if (argc==4) {
            if ((strict_strtoul(argv[2], 16, &pll0) < 0) && 
                (strict_strtoul(argv[2], 10, &pll0) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if ((strict_strtoul(argv[3], 16, &mux) < 0) && 
                (strict_strtoul(argv[3], 10, &mux) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }

            if (cs_xos2k_chpll(pgbus,cctrl_cs,pll0,mux)!=RM_OK) {
                printf("cs_xos2k_chpll failed!!\n");
                goto wayout;
            }
            
            while ((rc=sc_xos2k_chpll_u(pgbus,cctrl_sc))==RM_PENDING);
            
            if (rc!=RM_OK) {
                printf("sc_xos2k_chpll_u failed!!\n");
                goto wayout;
            }
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"random")==0) {
        unsigned long r;

        if (argc==2) {
            if (cs_xos2k_random(pgbus,cctrl_cs)!=RM_OK) {
                printf("cs_xos2k_random failed!!\n");
                goto wayout;
            }
            
            while ((rc=sc_xos2k_random_u(pgbus,cctrl_sc,&r))==RM_PENDING);
            
            if (rc!=RM_OK) {
                printf("sc_xos2k_random_u failed!!\n");
                goto wayout;
            }
            
            printf("0x%08lx\n",r);
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"ub")==0) {
        if (argc==2) {
            if (cs_xos2k_ub(pgbus,cctrl_cs)!=RM_OK) {
                printf("cs_xos2k_ub failed!!\n");
                goto wayout;
            }
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"dst")==0) {
        if (argc==2) {
            if (cs_xos2k_dst(pgbus,cctrl_cs)!=RM_OK) {
                printf("cs_xos2k_dst failed!!\n");
                goto wayout;
            }
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"formatall")==0) {
        RMstatus xrc;
        if (argc==2) {
            if (cs_xos2k_formatall(pgbus,cctrl_cs)!=RM_OK) {
                printf("cs_xos2k_formatall failed!!\n");
                goto wayout;
            }
            
            while ((rc=sc_xos2k_formatall_u(pgbus,cctrl_sc,&xrc))==RM_PENDING);
            
            if (rc!=RM_OK) {
                printf("sc_xos2k_formatall_u failed!!\n");
                goto wayout;
            }
            
            if (xrc != 6)
                printf("formatall failed. status = 0x%x\n", xrc);
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"xtoken")==0) {
        RMuint32 size, g_base_addr, kva_base_addr, type;
        RMstatus xrc;

        if (argc==5) {
            if ((strict_strtoul(argv[2], 16, &kva_base_addr) < 0) && 
                (strict_strtoul(argv[2], 10, &kva_base_addr) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            if ((strict_strtoul(argv[3], 16, &size) < 0) && 
                (strict_strtoul(argv[3], 10, &size) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            if ((strict_strtoul(argv[4], 16, &type) < 0) && 
                (strict_strtoul(argv[4], 10, &type) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }

            //g_base_addr = tangox_dma_address(PHYS(kva_base_addr));
            g_base_addr = tangox_dma_address(kva_base_addr);

            {
                if (cs_xos2k_xtoken(pgbus, cctrl_cs, type, g_base_addr, size)!=RM_OK) {
                    printf("cs_xos2k_xtoken failed!!\n");
                    goto wayout;
                }
                while ((rc=sc_xos2k_xtoken_u(pgbus, cctrl_sc, &xrc))==RM_PENDING);
                if (rc!=RM_OK) {
                    printf("sc_xos2k_xbind_u failed!!\n");
                    goto wayout;
                }
                if (xrc!=6)
                    printf("xtoken failed. status=0x%x\n", xrc);
            }

            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
        goto wayout;
    }

    if (strcmp(argv[1],"xbind")==0) {
        unsigned long size, g_base_addr, kva_base_addr;
        RMstatus xrc;
        if (argc==4) {
            if ((strict_strtoul(argv[2], 16, &kva_base_addr) < 0) && 
                (strict_strtoul(argv[2], 10, &kva_base_addr) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if ((strict_strtoul(argv[3], 16, &size) < 0) && 
                (strict_strtoul(argv[3], 10, &size) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }

            //g_base_addr = tangox_dma_address(PHYS(kva_base_addr));
            g_base_addr = tangox_dma_address(kva_base_addr);

            if (cs_xos2k_xbind(pgbus, cctrl_cs, g_base_addr, size) != RM_OK) {
                printf("cs_xos2k_xbind failed!!\n");
                goto wayout;
            }

            while ((rc=sc_xos2k_xbind_u(pgbus,cctrl_sc,&xrc))==RM_PENDING);

            if (rc!=RM_OK) {
                printf("sc_xos2k_xbind_u failed!!\n");
                goto wayout;
            }
    
            if (xrc != 6)
                printf("xbind failed. status = 0x%x\n", xrc);
           
             goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }


    if (strcmp(argv[1],"xstart")==0) {
        
        unsigned long xt_cookie, xl_cookie;
        struct xstart_param p;
        RMstatus xrc;
        
        if (argc==8) {
            
            if ((strict_strtoul(argv[2], 16, &xt_cookie) < 0) && 
                (strict_strtoul(argv[2], 10, &xt_cookie) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if ((strict_strtoul(argv[3], 16, &xl_cookie) < 0) && 
                (strict_strtoul(argv[3], 10, &xl_cookie) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if ((strict_strtoul(argv[4], 16, &p.param[0]) < 0) && 
                (strict_strtoul(argv[4], 10, &p.param[0]) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if ((strict_strtoul(argv[5], 16, &p.param[1]) < 0) && 
                (strict_strtoul(argv[5], 10, &p.param[1]) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if ((strict_strtoul(argv[6], 16, &p.param[2]) < 0 ) && 
                (strict_strtoul(argv[6], 10, &p.param[2]) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if ((strict_strtoul(argv[7], 16, &p.param[3]) < 0) && 
                (strict_strtoul(argv[7], 10, &p.param[3]) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }

            if (cs_xos2k_xstart(pgbus, cctrl_cs, xt_cookie, xl_cookie, &p) != RM_OK) {
                printf("cs_xos2k_xstart failed!!\n");
                goto wayout;
            }
            
            while ((rc=sc_xos2k_xstart_u(pgbus,cctrl_sc,&xrc))==RM_PENDING);
            
            if (rc!=RM_OK) {
                printf("sc_xos2k_xstart_u failed!!\n");
                goto wayout;
            }
            
            if (xrc != 6)
                printf("xstart failed. status = 0x%x\n", xrc);
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"xkill")==0) {
        unsigned long xt_cookie;
        long sig=-1;
        
        if (argc==4 || argc==3) {
            if ((strict_strtoul(argv[2], 16, &xt_cookie) < 0) && 
                (strict_strtoul(argv[2], 10, &xt_cookie) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if ((argc==4) && 
                (strict_strtoul(argv[3], 16, (unsigned long*)&sig) < 0) && 
                (strict_strtoul(argv[3], 10, (unsigned long*)&sig) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if (cs_xos2k_xkill(pgbus,cctrl_cs,xt_cookie,sig)!=RM_OK) {
                printf("cs_xos2k_xkill failed!!\n");
                goto wayout;
            }
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"ustart")==0) {
        unsigned long xl_cookie;
        long dspid;
        RMstatus xrc;
        char c, *p, dsp_set_8644[]="vVaAdD", dsp_set_8654[]="vadD";

        if (argc==4) {
            if ((strict_strtoul(argv[2], 16, &xl_cookie) < 0) && 
                (strict_strtoul(argv[2], 10, &xl_cookie) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            //if (sscanf(argv[3],"%c",&c)!=1) {
            if (simple_strtochar(argv[3], &c) < 0) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if (((gbus_read_uint16(pgbus,REG_BASE_host_interface+PCI_REG0) >> 4)&0xf) == 5)
                dspid = ((int)(p=strchr(dsp_set_8654, c)) ? p-dsp_set_8654 : -1);
            else
                dspid = ((int)(p=strchr(dsp_set_8644, c)) ? p-dsp_set_8644 : -1);
            
            if (dspid == -1) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if (cs_xos2k_ustart(pgbus, cctrl_cs, xl_cookie, dspid) != RM_OK) {
                printf("cs_xos2k_ustart failed!!\n");
                goto wayout;
            }
            
            while ((rc=sc_xos2k_ustart_u(pgbus,cctrl_sc,&xrc))==RM_PENDING);
            
            if (rc!=RM_OK) {
                printf("sc_xos2k_ustart_u failed!!\n");
                goto wayout;
            }
            
            if (xrc != 6)
                printf("ustart failed. status = 0x%x\n", xrc);
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"ukill")==0) {
        long dspid;
        long sig=-1;
        char c, *p, dsp_set_8644[]="vVaAdD", dsp_set_8654[]="vadD";

        if (argc==4 || argc==3) {
            //if (sscanf(argv[2],"%c",&c)!=1) {
            if (simple_strtochar(argv[2], &c) < 0) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if (((gbus_read_uint16(pgbus,REG_BASE_host_interface+PCI_REG0) >> 4)&0xf) == 5)
                dspid = ((int)(p=strchr(dsp_set_8654, c)) ? p-dsp_set_8654 : -1);
            else
                dspid = ((int)(p=strchr(dsp_set_8644, c)) ? p-dsp_set_8644 : -1);

            if (dspid == -1) {
                cmd_usage(cmdtp);
                goto wayout;
            }

            if ((argc==4) && 
                (strict_strtoul(argv[3], 16, (unsigned long*)&sig) < 0) && 
                (strict_strtoul(argv[3], 10, (unsigned long*)&sig) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
           
             if (cs_xos2k_ukill(pgbus,cctrl_cs,dspid,sig)!=RM_OK) {
                printf("cs_xos2k_ukill failed!!\n");
                goto wayout;
            }
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"xunload")==0) {
        unsigned long xl_cookie;
        RMstatus xrc;

        if (argc==3) {
            if ((strict_strtoul(argv[2], 16, &xl_cookie) < 0) && 
                (strict_strtoul(argv[2], 10, &xl_cookie) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if (cs_xos2k_xunload(pgbus,cctrl_cs,xl_cookie)!=RM_OK) {
                printf("cs_xos2k_xunload failed!!\n");
                goto wayout;
            }
        
            while ((rc=sc_xos2k_xunload_u(pgbus,cctrl_sc,&xrc))==RM_PENDING);
        
            if (rc!=RM_OK) {
                printf("sc_xos2k_xunload_u failed!!\n");
                goto wayout;
            }
            
            if (xrc != 6)
                printf("xunload failed. status = 0x%x\n", xrc);
            
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"xload")==0) {
        unsigned long size, g_load_addr, xl_cookie=0, g_base_addr, kva_base_addr;
        RMstatus xrc;
        //static unsigned long cookie_cnt = 1;
        if (argc==6) {
            if ((strict_strtoul(argv[2], 16, &xl_cookie) < 0) && 
                (strict_strtoul(argv[2], 10, &xl_cookie) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }  
                
            if ((strict_strtoul(argv[3], 16, &kva_base_addr) < 0) && 
                (strict_strtoul(argv[3], 10, &kva_base_addr) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
    
            if ((strict_strtoul(argv[4], 16, &size) < 0) && 
                (strict_strtoul(argv[4], 10, &size) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }
             
            if ((strict_strtoul(argv[5], 16, &g_load_addr) < 0) && 
                (strict_strtoul(argv[5], 10, &g_load_addr) < 0)) {
                cmd_usage(cmdtp);
                goto wayout;
            }

            //g_base_addr = tangox_dma_address(PHYS(kva_base_addr));
            g_base_addr = tangox_dma_address(kva_base_addr);
             //printf("g_base_addr = 0x%08lx, g_load_addr = 0x%08lx, size = 0x%08lx\n", g_base_addr, g_load_addr, size);
             //kva_base_addr = KSEG1(sigmaremap(g_base_addr));
             //printf("kva_base_addr = 0x%08lx\n", kva_base_addr);
             //memcpy((void *)kva_base_addr, (void *)addr, size);
             //printf("memcpy: src = 0x%08lx, dest = 0x%08lx \n", addr, kva_base_addr);
            printf("XLOADING src=0x%08lx, dest=0x%08lx, size=0x%08lx\n", g_base_addr, g_load_addr, size);
            if (cs_xos2k_xload(pgbus, cctrl_cs, xl_cookie, g_load_addr, g_base_addr, size) != RM_OK) {
                printf("cs_xos2k_xload failed!!\n");
                goto wayout;
            }
            printf("Waiting for XLOAD completion...\n");
            while ((rc=sc_xos2k_xload_u(pgbus, cctrl_sc, &xrc))==RM_PENDING);
             
            if (rc!=RM_OK) {
                printf("sc_xos2k_xload_u failed!!\n");
                goto wayout;
            }
            printf("XLOAD done. status = 0x%x .. ", xrc);

            if (xrc == 0x6)
                printf("OK.\n");
            else
                printf("NOT OK.\n");
  
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    } // -xload
    
    cmd_usage(cmdtp);
    goto wayout;
    

runit:
    rc = 0;

wayout:

    return rc;
}

/* ====================================================================== */
U_BOOT_CMD(
	xkc,	10,	1,	do_xkc,
	"Process xkc request",
	"\t(xload <lkey> <source KV addr.> <xload_size>\n"
    "\t <dest. gbus addr.>) |\n"
    "\t(xunload <lkey>) |\n"
    "\t(xbind <source KV addr.> <xload_size>) |\n"
    "\t(xtoken <source KV addr.> <size> <type>) |\n"
    "\t(xstart <tkey> <lkey> <a0> <a1> <a2> <a3>) |\n"
    "\t(xkill <tkey> [<sig>]) |\n"
    "\t(ustart <lkey> <dspid>) |\n"
    "\t(ukill <dspid> [<sig>]) |\n"
    "\t(chpll <pll0> <mux> |\n"
    "\tprotregs | hello"
);


