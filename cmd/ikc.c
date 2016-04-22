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

//#define CMD_IKC_DEBUG

#include <common.h>
#include <command.h>
#include <watchdog.h>

#if defined (CONFIG_TANGO3) || defined (CONFIG_TANGO4)
#include <asm/arch/setup.h>
#endif

#include "../ixkc/channel.h"
#include "../ixkc/ios_client.h"

#ifdef	CMD_IKC_DEBUG
#define	DEBUG(fmt,args...)	printf (fmt ,##args)
#else
#define DEBUG(fmt,args...)
#endif

extern RMuint32 sigmaremap(RMuint32);
extern unsigned long tangox_dma_address(unsigned long);


#define pgbus           ((struct gbus *)0)

struct channel_control *cctrl_cs_i=NULL;
struct channel_control *cctrl_sc_i=NULL;
struct itask_context {
    unsigned int ra;
    unsigned int fp;
    unsigned int gp;
    unsigned int t9;
    unsigned int t8;
    unsigned int s7;
    unsigned int s6;
    unsigned int s5;
    unsigned int s4;
    unsigned int s3;
    unsigned int s2;
    unsigned int s1;
    unsigned int s0;
    unsigned int t7;
    unsigned int t6;
    unsigned int t5;
    unsigned int t4;
    unsigned int t3;
    unsigned int t2;
    unsigned int t1;
    unsigned int t0;
    unsigned int a3;
    unsigned int a2;
    unsigned int a1;
    unsigned int a0;
    unsigned int v1;
    unsigned int v0;
    unsigned int at;
    unsigned int sr;
    unsigned int epc;
    unsigned int sp;
    unsigned int entryhi;
};

int do_ikc ( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    unsigned int rc = 0;
#if defined (CONFIG_MIPS)
    unsigned int address31_26, address25_0;
    unsigned int saveaddr = 0, address; // saved_base_c = 0, saved_base_s = 0;
#endif    
    unsigned int *channel_index;

    if ( argc < 2 )
        return cmd_usage(cmdtp);

    channel_index = (unsigned int*)gbus_read_uint32(pgbus, REG_BASE_cpu_block+LR_CHANNEL_INDEX);
    DEBUG("channel_index = 0x%08x\n", (unsigned int)channel_index);

#if defined (CONFIG_MIPS)
    cctrl_cs_i = (struct channel_control*)gbus_read_uint32(pgbus, 
                    sigmaremap((unsigned int)&channel_index[CHANNEL_IOS_API_C2I]));

    cctrl_sc_i = (struct channel_control*)gbus_read_uint32(pgbus, 
                    sigmaremap((unsigned int)&channel_index[CHANNEL_IOS_API_I2C]));
#else
    cctrl_cs_i = (struct channel_control*)gbus_read_uint32(pgbus, 
                    (unsigned int)&channel_index[CHANNEL_IOS_API_C2I]);

    cctrl_sc_i = (struct channel_control*)gbus_read_uint32(pgbus, 
                    (unsigned int)&channel_index[CHANNEL_IOS_API_I2C]);
#endif                    

    DEBUG("(GA) control_cs_i = 0x%08x\n", (unsigned int)cctrl_cs_i);
    DEBUG("(GA) control_sc_i = 0x%08x\n", (unsigned int)cctrl_sc_i);

    if (cctrl_cs_i == 0 || cctrl_sc_i == 0) {
        printf("Error: IKC communication channel not established yet!\n");
        goto runit;
    }
    else {
#if defined (CONFIG_MIPS)        
        if ((address = sigmaremap((unsigned int)cctrl_cs_i)) == -1 ) {
            address31_26 = (unsigned int)cctrl_cs_i & (~((1<<26)-1));
            address25_0  = (unsigned int)cctrl_cs_i & ((1<<26)-1);
            saveaddr     = gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP);
            gbus_write_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP, address31_26);
            while (gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP) != (address31_26 & 0xfc000000))
            ;
            
            address = CPU_TMP_REMAP_address | address25_0;
        }
        cctrl_cs_i = (struct channel_control *)address;
#endif            
        
        DEBUG("(VA) cctrl_cs_i = 0x%08x\n", (unsigned int)cctrl_cs_i);

#if defined (CONFIG_MIPS)        
        if ((address = sigmaremap((unsigned int)cctrl_sc_i)) == -1 ) {
            address31_26 = (unsigned int)cctrl_sc_i & (~((1<<26)-1));
            address25_0  = (unsigned int)cctrl_sc_i & ((1<<26)-1);
            saveaddr     = gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP);
            gbus_write_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP, address31_26);
             while (gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP) != (address31_26 & 0xfc000000))
            ;
            address = CPU_TMP_REMAP_address | address25_0;
        }
        cctrl_sc_i = (struct channel_control *)address;
#endif        
        DEBUG("(VA) cctrl_sc_i = 0x%08x\n", (unsigned int)cctrl_sc_i);
    }

    if (strcmp(argv[1],"meminfo")==0) {
        int usage, log2size;
        if (argc==2) {
            if (cs_ios_meminfo(pgbus, cctrl_cs_i)!=RM_OK) {
                printf("cs_ios_meminfo failed!!\n");
                goto wayout;
            }
             
            while ((rc=sc_ios_meminfo_u(pgbus, cctrl_sc_i, &usage, &log2size)) == RM_PENDING);
             
            if (rc!=RM_OK) {
                printf("sc_ios_meminfo_u failed!!\n");
                goto wayout;
            }
            printf("Memory usage: %d/%d bytes (%d%%)\n", usage, 1<<log2size, (100*usage)>>log2size);
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }
#define em86xx_msleep(d)    mdelay(d)
    if (strcmp(argv[1],"monitor")==0) {
        unsigned long i;
        struct irq_monitoring_sample sample;
        if (argc==2) {
            if (cs_ios_monitoring_start(pgbus,cctrl_cs_i)!=RM_OK) {
                printf("cs_ios_monitoring_start failed!!\n");
                goto wayout;
            }
            for (i=0; i<20; i++) {
                em86xx_msleep(4000);

                if (cs_ios_monitoring_sample(pgbus,cctrl_cs_i) != RM_OK) {
                    printf("cs_ios_monitoring_sample failed!!\n");
                    goto wayout;
                }
                
                while ((rc=sc_ios_monitoring_sample_u(pgbus,cctrl_sc_i,&sample))==RM_PENDING);
                
                if (rc!=RM_OK) {
                    printf("sc_ios_monitoring_sample_u failed!!\n");
                    goto wayout;
                }

#define MEAN(time,count) ((count)?((RMuint32)(time)/(RMuint32)(count)):0)
#define IPUUSAGE(time) (((time)*100)/(sample.measure_duration))

                printf("iiq count:%05ld mean_time:%05ld max_time:%05ld ipu_usage:%02ld%%\n", 
                    sample.irq_count[3], MEAN(sample.irq_time[3], sample.irq_count[3]), 
                    sample.irq_max_time[3], IPUUSAGE(sample.irq_time[3]));
                printf("iiq count:%05ld mean_time:%05ld max_time:%05ld ipu_usage:%02ld%%\n", 
                    sample.irq_count[2], MEAN(sample.irq_time[2], sample.irq_count[2]), 
                    sample.irq_max_time[2], IPUUSAGE(sample.irq_time[2]));
                printf("iiq count:%05ld mean_time:%05ld max_time:%05ld ipu_usage:%02ld%%\n", 
                    sample.irq_count[1], MEAN(sample.irq_time[1], sample.irq_count[1]), 
                    sample.irq_max_time[1], IPUUSAGE(sample.irq_time[1]));
                printf("iiq count:%05ld mean_time:%05ld max_time:%05ld ipu_usage:%02ld%%\n", 
                    sample.irq_count[0], MEAN(sample.irq_time[0], sample.irq_count[0]), 
                    sample.irq_max_time[0], IPUUSAGE(sample.irq_time[0]));
            }
            
            if (cs_ios_monitoring_stop(pgbus,cctrl_cs_i)!=RM_OK) {
                printf("cs_ios_monitoring_stop failed!!\n");
                goto wayout;
            }
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"logfifo")==0) {

        unsigned long log_ga;
        RMstatus irc;  

        if (argc==2) {
            if (cs_ios_getlogfifo(pgbus,cctrl_cs_i)!=RM_OK) {
                printf("cs_ios_getlogfifo failed!!\n");
                goto wayout;
            }
             
            while ((rc=sc_ios_getlogfifo_u(pgbus,cctrl_sc_i,&irc,&log_ga))==RM_PENDING);
            
            if (rc!=RM_OK) {
                printf("sc_ios_getlogfifo_u failed!!\n");
                goto wayout;
            }
            
            if (irc != 6)
                printf("logfifo failed. status = 0x%x\n", irc);
            else
                printf("log fifo ready at gbus address: 0x%08lx\n", log_ga);
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"cache")==0) {
         unsigned long dump_ga;

        if (argc==3) {
            if ( (strict_strtoul(argv[2], 16, &dump_ga)) < 0 && 
                 (strict_strtoul(argv[2], 10, &dump_ga)) < 0  ) 
                cmd_usage(cmdtp);
                goto wayout;

            if (cs_ios_cachedump(pgbus,cctrl_cs_i,dump_ga)!=RM_OK) {
                printf("cs_ios_cachedump failed!!\n");
                goto wayout;
            }
            
            while ((rc=sc_ios_cachedump_u(pgbus,cctrl_sc_i))==RM_PENDING);
            
            if (rc!=RM_OK) {
                printf("sc_ios_cachedump_u failed!!\n");
                goto wayout;
            }
            
            printf("cache dump ready at gbus address: 0x%08lx\n", dump_ga);
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }
    
    if (strcmp(argv[1],"pause")==0) {
        if (argc==2) {
            if (cs_ios_pause(pgbus,cctrl_cs_i)!=RM_OK) {
                printf("cs_ios_pause failed!!\n");
                goto wayout;
            }
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"ub")==0) {
        if (argc==2) {
            if (cs_ios_ub(pgbus,cctrl_cs_i)!=RM_OK) {
                printf("cs_ios_ub failed!!\n");
                goto wayout;
            }
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }
    
    if (strcmp(argv[1],"version")==0) {
         unsigned long v,ev;
    
        if (argc==2) {
            if (cs_ios_version(pgbus,cctrl_cs_i)!=RM_OK) {
                printf("cs_ios_version failed!!\n");
                goto wayout;
            }
            
            while ((rc=sc_ios_version_u(pgbus,cctrl_sc_i,&v,&ev))==RM_PENDING);
        
            if (rc!=RM_OK) {
                printf("sc_ios_version_u failed!!, rc= %d\n", rc);
                goto wayout;
            }
        
            printf("ios_version=%ld.%02ld\n",
                RMunshiftBits(v,8,8),
                RMunshiftBits(v,8,0));
                printf("ios_flags=");
            if (v>>16) {
                if (v & IOS_FDEBUG)
                    printf("_DEBUG ");
                if (v & IOS_FDEBUGGER)
                    printf("IOS_DEBUGGER ");
                if (v & IOS_FKERNELONLY)
                    printf("IOS_KERNELONLY ");
            }
            else {
                printf("(none)");
            }
            
            printf("\nemhwlib_version=%ld.%ld.%ld.%ld\n",
                 RMunshiftBits(ev,8,24),
                 RMunshiftBits(ev,8,16),
                 RMunshiftBits(ev,8,8),
                 RMunshiftBits(ev,8,0));
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }   
    }

    if ( strcmp(argv[1],"ps")==0 ) {
        struct ps_entry ps;
        unsigned long count=0;
       
        if (argc==2) {
            printf(" #   tid vmid key        image_key  pc         ro     rw     stack  priority state\n");
            
            do {
                if (cs_ios_ps(pgbus,cctrl_cs_i) != RM_OK) {
                    printf("cs_ios_ps failed!!\n");
                    goto wayout;
                }
            
                while ((rc=sc_ios_ps_u(pgbus,cctrl_sc_i,&ps))==RM_PENDING);
            
                if (rc!=RM_OK) {
                    printf("sc_ios_ps_u failed!!\n");
                    goto wayout;
                }
            
                if (ps.last)
                    break;
                count++;
                printf("%03ld  %03d %03d  0x%08lx 0x%08lx 0x%08lx %06d %06d %06d %02d       ",
                    count, ps.info.tid, ps.info.vmid, ps.key, ps.info.image_key, ps.info.pc, ps.info.ro_size,
                    ps.info.rw_size, ps.info.stack_size, ps.info.priority);
                switch(ps.info.state) {
                    case ITASK_INFO_STATE_TERMINATED: printf("Terminated\n"); break;
                    case ITASK_INFO_STATE_READY: printf("Ready\n"); break;
                    case ITASK_INFO_STATE_RUNNING: printf("Running\n"); break;
                    case ITASK_INFO_STATE_SLEEPING: printf("Sleeping\n"); break;
                    case ITASK_INFO_STATE_ZOMBIE: printf("Zombie\n"); break;
                    default: printf("Unknown state %d\n", (int)ps.info.state); 
                }
            } while (1);
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"regdump")==0) {
        unsigned long t_cookie, i;
        struct ios_itask_info info;
        struct itask_context context, *pC=&context;
        unsigned int *pcontext = (unsigned int*)&context;
        RMstatus irc;  

        if (argc==3) {
            if ( (strict_strtoul(argv[2], 16, &t_cookie)) < 0 &&
                 (strict_strtoul(argv[2], 10, &t_cookie)) < 0 ) {
                cmd_usage(cmdtp);
                goto wayout;
            }

            if (cs_ios_getinfo(pgbus,cctrl_cs_i,t_cookie)!=RM_OK) {
                printf("regdump: cs_ios_getinfo failed!!\n");
                goto wayout;
            }
        
            while ((rc=sc_ios_getinfo_u(pgbus,cctrl_sc_i,&info))==RM_PENDING);
        
            if (rc!=RM_OK) {
                printf("regdump: sc_ios_getinfo_u failed!!\n");
                goto wayout;
            }
            if (info.state == ITASK_INFO_STATE_TERMINATED) {
                printf("itask is Terminated");
                goto runit;
            }
            if ((info.state != ITASK_INFO_STATE_ZOMBIE)&&(info.state != ITASK_INFO_STATE_SLEEPING)) {
                if (cs_ios_suspend(pgbus,cctrl_cs_i,t_cookie) != RM_OK) {
                    printf("regdump: cs_ios_suspend failed!!\n");
                    goto wayout;
                }
             
               while ((rc=sc_ios_suspend_u(pgbus,cctrl_sc_i,&irc))==RM_PENDING);
                
                if (rc!=RM_OK) {
                    printf("regdump: sc_ios_suspend_u failed!!\n");
                    goto wayout;
                }
             
                if (irc != 6) {
                    printf("regdump: suspend failed. status = 0x%x\n", irc);
                    goto runit;
                }
            }
            
            for (i=0; i<(sizeof(struct itask_context)/4); i++) {
                unsigned long value;

                em86xx_msleep(1); // delay required to avoid the RM_PENDING below
                if (cs_ios_getregister(pgbus,cctrl_cs_i,t_cookie,i) != RM_OK) {
                    printf("cs_ios_getregister failed!!\n");
                    goto wayout;
                }
                
                while ((rc=sc_ios_getregister_u(pgbus,cctrl_sc_i,&irc,&value))==RM_PENDING);
                
                if (rc!=RM_OK) {
                    printf("sc_ios_getregister_u failed!!\n");
                    goto wayout;
                }
             
                if (irc != 6) {
                    printf("regdump failed. i = %ld, status = 0x%x\n", i, irc);
                    goto runit;
                }

                pcontext[i] = value;
            }
            
            if ((info.state != ITASK_INFO_STATE_ZOMBIE)&&(info.state != ITASK_INFO_STATE_SLEEPING)) {
                if (cs_ios_resume(pgbus,cctrl_cs_i,t_cookie) != RM_OK) {
                    printf("regdump: cs_ios_resume failed!!\n");
                    goto wayout;
                }
            
                while ((rc=sc_ios_resume_u(pgbus,cctrl_sc_i,&irc))==RM_PENDING);
                
                if (rc!=RM_OK) {
                    printf("regdump: sc_ios_resume_u failed!!\n");
                    goto wayout;
                }
            
                if (irc != 6) {
                    printf("regdump: resume failed. status = 0x%x\n", irc);
                    goto runit;
               }
           }
           printf("hi=0x%08x at=0x%08x v0=0x%08x v1=0x%08x a0=0x%08x a1=0x%08x a2=0x%08x a3=0x%08x\n",
                 pC->entryhi, pC->at, pC->v0, pC->v1, pC->a0, pC->a1, pC->a2, pC->a3);
           printf("t0=0x%08x t1=0x%08x t2=0x%08x t3=0x%08x t4=0x%08x t5=0x%08x t6=0x%08x t7=0x%08x\n",
                 pC->t0, pC->t1, pC->t2, pC->t3, pC->t4, pC->t5, pC->t6, pC->t7);
           printf("s0=0x%08x s1=0x%08x s2=0x%08x s3=0x%08x s4=0x%08x s5=0x%08x s6=0x%08x s7=0x%08x\n",
                 pC->s0, pC->s1, pC->s2, pC->s3, pC->s4, pC->s5, pC->s6, pC->s7);
           printf("t8=0x%08x t9=0x%08x sr=0x%08x pc=0x%08x gp=0x%08x sp=0x%08x fp=0x%08x ra=0x%08x\n",
                 pC->t8, pC->t9, pC->sr, pC->epc,pC->gp, pC->sp, pC->fp, pC->ra);
             goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }
    
    if (strcmp(argv[1],"info")==0) {
        unsigned long t_cookie;
        struct ios_itask_info info;
        if (argc==3) {
            if ( (strict_strtoul(argv[2], 16, &t_cookie)) < 0 && 
                 (strict_strtoul(argv[2], 10, &t_cookie)) < 0 ) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if (cs_ios_getinfo(pgbus,cctrl_cs_i,t_cookie)!=RM_OK) {
                printf("cs_ios_getinfo failed!!\n");
                goto wayout;
            }
            
            while ((rc=sc_ios_getinfo_u(pgbus,cctrl_sc_i,&info))==RM_PENDING);
            
            if (rc!=RM_OK) {
                printf("sc_ios_getinfo_u failed!!\n");
                goto wayout;
            }
           
            switch(info.state) {
                case ITASK_INFO_STATE_TERMINATED: printf("Terminated\n"); break;
                case ITASK_INFO_STATE_READY: printf("Ready\n"); break;
                case ITASK_INFO_STATE_RUNNING: printf("Running\n"); break;
                case ITASK_INFO_STATE_SLEEPING: printf("Sleeping\n"); break;
                case ITASK_INFO_STATE_ZOMBIE: printf("Zombie\n"); break;
                default: printf("Unknown state %d\n", (int)info.state);
            }
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"setpriority")==0) {
         unsigned long t_cookie, priority;
         RMstatus irc;  
         
        if (argc==4) {
            
            if ( (strict_strtoul(argv[2], 16, &t_cookie)) < 0 && 
                 (strict_strtoul(argv[2], 10, &t_cookie)) < 0 ) {
                cmd_usage(cmdtp);
                goto wayout;
            }

            if ( (strict_strtoul(argv[3], 16, &priority)) < 0 && 
                 (strict_strtoul(argv[3], 10, &priority)) < 0 ) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if (cs_ios_setpriority(pgbus,cctrl_cs_i,t_cookie,priority)!=RM_OK) {
               printf("cs_ios_setpriority failed!!\n");
               goto wayout;
            }
            
            while ((rc=sc_ios_setpriority_u(pgbus,cctrl_sc_i,&irc))==RM_PENDING);
            
            if (rc!=RM_OK) {
               printf("sc_ios_setpriority_u failed!!\n");
               goto wayout;
            }
            
            if (irc != 6)
               printf("setpriority failed. status = 0x%x\n", irc);
             
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if ( strcmp(argv[1], "suspend") == 0) {
        unsigned long t_cookie;
        RMstatus irc;  
        
        if (argc==3) {
            if ( (strict_strtoul(argv[2], 16, &t_cookie)) < 0 && 
                 (strict_strtoul(argv[2], 10, &t_cookie)) < 0 ) {
                cmd_usage(cmdtp);
                goto wayout;
            }

            if (cs_ios_suspend(pgbus,cctrl_cs_i,t_cookie)!=RM_OK) {
            
                printf("cs_ios_suspend failed!!\n");
                goto wayout;
            }
            
            while ((rc=sc_ios_suspend_u(pgbus,cctrl_sc_i,&irc))==RM_PENDING);
            
            if (rc!=RM_OK) {
                printf("sc_ios_suspend_u failed!!\n");
                goto wayout;
            }

            if (irc != 6)
                printf("suspend failed. status = 0x%x\n", irc);
            
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"resume")==0) {
         unsigned long t_cookie;
         RMstatus irc;  

        if (argc==3) {

            if ( (strict_strtoul(argv[2], 16, &t_cookie)) < 0 && 
                 (strict_strtoul(argv[2], 10, &t_cookie)) < 0 ) {
                cmd_usage(cmdtp);
                goto wayout;
            }

            if (cs_ios_resume(pgbus,cctrl_cs_i,t_cookie)!=RM_OK) {
                printf("cs_ios_resume failed!!\n");
                goto wayout;
            }
            
            while ((rc=sc_ios_resume_u(pgbus,cctrl_sc_i,&irc))==RM_PENDING);
            
            if (rc!=RM_OK) {
                printf("sc_ios_resume_u failed!!\n");
                goto wayout;
            }
            
            if (irc != 6)
                printf("resume failed. status = 0x%x\n", irc);
            
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"start")==0) {
        unsigned long t_cookie, l_cookie, priority=8, flag=0;
        struct itask_start_param p;
        RMstatus irc;
        
        if (argc>=8 && argc<=10) {

            if ( (strict_strtoul(argv[2], 16, &t_cookie)) < 0 && 
                 (strict_strtoul(argv[2], 10, &t_cookie)) < 0 ) {
                cmd_usage(cmdtp);
                goto wayout;
            }

            if ( (strict_strtoul(argv[3], 16, &l_cookie)) < 0 && 
                 (strict_strtoul(argv[3], 10, &l_cookie)) < 0 ) {
                cmd_usage(cmdtp);
                goto wayout;
            }

            if ( (strict_strtoul(argv[4], 16, &p.a0)) < 0 && 
                 (strict_strtoul(argv[4], 10, &p.a0)) < 0 ) {
                cmd_usage(cmdtp);
                goto wayout;
            }


            if ( (strict_strtoul(argv[5], 16, &p.a1)) < 0  && 
                 (strict_strtoul(argv[5], 10, &p.a1)) < 0 ) {
                cmd_usage(cmdtp);
                goto wayout;
            }

            if ( (strict_strtoul(argv[6], 16, &p.a2)) < 0 && 
                 (strict_strtoul(argv[6], 10, &p.a2)) < 0 ) {
                cmd_usage(cmdtp);
                goto wayout;
            }
           
 
            if ( (strict_strtoul(argv[7], 16, &p.a3)) < 0  && 
                 (strict_strtoul(argv[7], 10, &p.a3)) < 0 ) {
                cmd_usage(cmdtp);
                goto wayout;
            }
            
            if (argc >= 9) {
               if ( (strict_strtoul(argv[6], 16, &priority)) < 0 && 
                    (strict_strtoul(argv[6], 10, &priority)) < 0 ) {
                    cmd_usage(cmdtp);
                    goto wayout;
                }
 
            }
            
            if (argc == 10) {
                if ( (strict_strtoul(argv[6], 16, &flag)) < 0 && 
                     (strict_strtoul(argv[6], 10, &flag)) < 0 ) {
                    cmd_usage(cmdtp);
                    goto wayout;
                }
              
            }
        
            if (cs_ios_start(pgbus, cctrl_cs_i, t_cookie, l_cookie, &p, priority, flag) != RM_OK) {
                printf("cs_ios_start failed!!\n");
                goto wayout;
            }
           
            while ((rc=sc_ios_start_u(pgbus,cctrl_sc_i,&irc))==RM_PENDING);
            
            if (rc!=RM_OK) {
                printf("sc_ios_start_u failed!!\n");
                goto wayout;
            }
           
            if (irc != 6)
                printf("start failed. status = 0x%x\n", irc);
            
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }
    
    if (strcmp(argv[1],"kill")==0) {
        unsigned long t_cookie;
        RMstatus irc;  
        
        if (argc==3) {
            if ( (strict_strtoul(argv[2], 16, &t_cookie)) < 0 && 
                 (strict_strtoul(argv[2], 10, &t_cookie)) < 0 ) {
                cmd_usage(cmdtp);
                goto wayout;
            }

            if (cs_ios_kill(pgbus,cctrl_cs_i,t_cookie)!=RM_OK) {
                printf("cs_ios_kill failed!!\n");
                goto wayout;
            }
    
            while ((rc=sc_ios_kill_u(pgbus,cctrl_sc_i,&irc))==RM_PENDING);
    
            if (rc!=RM_OK) {
                printf("sc_ios_kill_u failed!!\n");
                goto wayout;
            }
            
            if (irc != 6)
                printf("kill failed. status = 0x%x\n", irc);

            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"unload")==0) {
        unsigned long l_cookie;
        RMstatus irc;  
        
        if (argc==3) {

            if ( (strict_strtoul(argv[2], 16, &l_cookie)) < 0 && 
                 (strict_strtoul(argv[2], 10, &l_cookie)) < 0 ) {
                cmd_usage(cmdtp);
                goto wayout;
            }

            if (cs_ios_unload(pgbus,cctrl_cs_i,l_cookie)!=RM_OK) {
                printf("cs_ios_unload failed!!\n");
                goto wayout;
            }
        
            while ((rc=sc_ios_unload_u(pgbus,cctrl_sc_i,&irc))==RM_PENDING);
        
            if (rc!=RM_OK) {
                printf("sc_ios_unload_u failed!!\n");
                goto wayout;
            }
            
            if (irc != 6)
                printf("unload failed. status = 0x%x\n", irc);
            
            goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }

    if (strcmp(argv[1],"load")==0) {
        unsigned long size, l_cookie=0, g_base_addr, kva_base_addr;
        RMstatus irc;
            if (argc==5) {
 
                if ( (strict_strtoul(argv[2], 16, &l_cookie)) < 0 && 
                     (strict_strtoul(argv[2], 10, &l_cookie)) < 0 ) {
                    cmd_usage(cmdtp);
                    goto wayout;
                }
             
 
                if ( (strict_strtoul(argv[2], 16, &kva_base_addr)) < 0 && 
                     (strict_strtoul(argv[2], 10, &kva_base_addr) < 0 )) {
                    cmd_usage(cmdtp);
                    goto wayout;
                }

           
                if ( (strict_strtoul(argv[2], 16, &size)) < 0 && 
                     (strict_strtoul(argv[2], 10, &size)) < 0 ) {
                    cmd_usage(cmdtp);
                    goto wayout;
                }

                //g_base_addr = tangox_dma_address(PHYS(kva_base_addr));
                g_base_addr = tangox_dma_address(kva_base_addr);
                printf("LOADING src=0x%08lx, size=0x%08lx\n", g_base_addr, size);

                if (cs_ios_load(pgbus, cctrl_cs_i, l_cookie, g_base_addr, size) != RM_OK) {
                    printf("cs_ios_load failed!!\n");
                    goto wayout;
                }
        
                printf("Waiting for LOAD completion...\n");
             
                while ((rc=sc_ios_load_u(pgbus, cctrl_sc_i, &irc))==RM_PENDING);
             
                if (rc!=RM_OK) {
                    printf("sc_ios_load_u failed!!\n");
                    goto wayout;
                }
            
                 printf("LOAD done. status = 0x%x .. ", irc);   

                if (irc == 6)
                    printf("OK.\n");
                else
                    printf("NOT OK.\n");

                goto runit;
        }
        else {
            printf("Syntax error!\n");
            cmd_usage(cmdtp);
        }
    }
    
    cmd_usage(cmdtp);
    goto wayout;
    

runit:
    rc = 0;

wayout:

#if defined (CONFIG_MIPS)
    /* cleanup code goes here */
    if (saveaddr) {
        gbus_write_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP, saveaddr);
        while (gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP) != (saveaddr & 0xfc000000));
    }
#endif    

    return rc;
}

/* ====================================================================== */
U_BOOT_CMD(
	ikc,	2,	1,	do_ikc,
	"Process ikc request",
	"\t(load <image_key> <source KV addr.> <load_size>) |\n"
    "\t(unload <image_key>) |\n"
    "\t(start <itask_key> <image_key> <a0> <a1> <a2> <a3>\n"
    "\t [<priority>] [<flag>] |\n"
    "\t(kill <itask_key> |\n"
    "\tsuspend <itask_key> |\n"
    "\tresume <itask_key> |\n"
    "\tsetpriority <itask_key> <priority> |\n"
    "\tinfo <itask_key> |\n"
    "\tregdump <itask_key> |\n"
    "\tcache <dump gbus addr.> |\n"
    "\tps | version"
);


