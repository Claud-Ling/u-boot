/*
 * (C) Copyright 2000-2003
 build for boot kernel 
 */

/*
 * Misc boot support
 */
#include <common.h>
#include <command.h>
#include <net.h>
#include <u-boot/crc.h>
#if defined(CONFIG_CMD_USB)
#include <usb.h>
#endif


DECLARE_GLOBAL_DATA_PTR;      //declear gb.

#ifdef CONFIG_CMD_KERNEL

//we want support boot up safe-kernel, let it be 16M 
#define KERNEL_MAX_SIZE  		0x1000000
#define CMDLINE_MAX_SIZE		1024
//#define KERNEL_LOAD_BUF			0x80100000
#define KERNEL_LOAD_BUF			0x8000

typedef void (*kernel_entry_t)(int zero, int arch, uint params);

typedef struct tagNewHEADER
{
    unsigned long  version;
    unsigned long  model;
    unsigned long crc;
    unsigned long length;
    char          vendor[16];
    char          kernel_parameter[512];
}NewHeader;

#if 0
/*Obsolete legacy structs,not used*/
typedef void (*kernel_entry) (int argc, char **argv, char **envp,int *prom_vec) ;
struct bootParams
{
	unsigned char *initRDBegin;
   	unsigned char *initRDEnd;
   	unsigned int cmd_line_size;
	char cmd_line[CMDLINE_MAX_SIZE];
}cmdlineparam;
#endif

static struct tag *params;
static void setup_start_tag (bd_t *bd)
{
        params = (struct tag *)bd->bi_boot_params;

        params->hdr.tag = ATAG_CORE;
        params->hdr.size = tag_size (tag_core);

        params->u.core.flags = 0;
        params->u.core.pagesize = 0;
        params->u.core.rootdev = 0;

        params = tag_next (params);
}

static void setup_commandline_tag(bd_t *bd, char *commandline)
{
        char *p;

        if (!commandline)
                return;

        /* eat leading white space */
        for (p = commandline; *p == ' '; p++);

        /* skip non-existent command lines so the kernel will still
 *          * use its default command line.
 *                   */
        if (*p == '\0')
                return;

        params->hdr.tag = ATAG_CMDLINE;
        params->hdr.size =
                (sizeof (struct tag_header) + strlen (p) + 1 + 4) >> 2;

        strcpy (params->u.cmdline.cmdline, p);

        params = tag_next (params);
}

static void setup_end_tag(bd_t *bd)
{
        params->hdr.tag = ATAG_NONE;
        params->hdr.size = 0;
}
/*
 * r1 = machine nr, r2 = atags or dtb pointer.
 */
static void setup_boot_prep_linux(void)
{

	gd->bd->bi_boot_params = 0x100;
	gd->bd->bi_arch_number = MACH_TYPE_SIGMA_SX6;
}

#define KERNEL_CMD_LINE_DEFAULT		"mem=14M console=ttyS0,115200n8 "

/* Allow ports to override the default behavior */
__attribute__((weak))
unsigned long do_go_exec (ulong (*entry)(int, char * const []), int argc, char * const argv[])
{
	return entry (argc, argv);
}

int do_crc_start_kernel (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
		ulong	addr, rc;
		int 	rcode = 0;
		kernel_entry_t	 p = NULL;
		char *linux_cmdline = NULL;
		//char  fs_cmdline[512];
		char new_cmdline[CMDLINE_MAX_SIZE];
		int header_size;
		unsigned long raw_crc32;
		unsigned long crc;
		NewHeader *header;
		uchar *dest;
		
		unsigned long machid;
		
		setup_boot_prep_linux();
			
		if (argc != 3) 
		{
			printf ("Usage:\n%s\n", cmdtp->usage);
			return 1;
		}
#ifdef CONFIG_CMD_WATCHDOG
		run_command("watchdog disable", 0);
#endif
#if defined(CONFIG_CMD_USB)
		usb_stop();
#endif
		dest = (uchar*)simple_strtoul(argv[1], NULL, 16);
		header = (NewHeader *)dest;
		header_size = sizeof(NewHeader);
		raw_crc32 = header->crc ;
		printf("raw_crc32 is %x length %ld header size is %d\n",  (unsigned int)raw_crc32, header->length, header_size);

		crc = crc32(0, (dest+header_size), header->length-sizeof(NewHeader));

		if(crc != raw_crc32)
		{
			printf("CRC ERROR .... File size is %ld cal crc 32 is %x,  %x\n", header->length, (unsigned int)raw_crc32, (unsigned int)crc);
		}
		memcpy((void *) KERNEL_LOAD_BUF,
                            (void *) (dest + sizeof (NewHeader)),
                            header->length - sizeof (NewHeader));
		addr = CONFIG_SYS_START_KERNEL_ADDR;	
		
						
		linux_cmdline = (char *)getenv(argv[2]);
		printf("kernel cmd is %s=%s\n", argv[2], linux_cmdline );
			       	
		printf ("## Starting Kernel at 0x%08lX ...\n", addr);
		
		if(linux_cmdline == NULL)
		{
			printf("no found cmdline,use default!\n");
			linux_cmdline = KERNEL_CMD_LINE_DEFAULT;
		}else{
			printf("get cmdline 0x%x\n",(unsigned int)linux_cmdline);
		}

		p = (kernel_entry_t)(addr); 
	
		printf("flush cache %s %d \n",__func__,__LINE__);

		flush_cache((ulong)addr, KERNEL_MAX_SIZE);
	
		sprintf( new_cmdline,  "%s", linux_cmdline );
#ifdef CONFIG_DTV_SMC
		if (getenv("armor_config"))
		{
			const char *tmp = getenv("armor_config");
			if ((sizeof(new_cmdline) - strlen(new_cmdline)) > (strlen(tmp) + 2))
			{
				strcat((char*)new_cmdline, " ");
				strcat((char*)new_cmdline, tmp);
			}
			else
			{
				printf("error: small buffer!!\n");
			}
		}
#endif

		printf("cmdline passed to kernel = %s\n",new_cmdline);
	
		//flush_dcache_range((ulong)cmdlineparam.cmd_line, (ulong)cmdlineparam.cmd_line + sizeof(new_cmdline));
		if (new_cmdline == NULL) 
			printf(".....No cmdline is passed\n");
		else {
			printf(" address of taglist = %x\n",(unsigned int)gd->bd->bi_boot_params);
			printf(" Start Kernel......\n");

#ifndef CONFIG_ARCH_SIGMA_TRIX
#ifndef CONFIG_BYPASS_L2		
			l2cache_init();
#endif
#endif
			/*
 			* build the taglist for kernel...
 			*/ 
			setup_start_tag(gd->bd);
			setup_commandline_tag(gd->bd,new_cmdline);
			setup_end_tag(gd->bd);
			/*pass machine id to kernel*/
			machid = gd->bd->bi_arch_number;
		
			p(0, machid, gd->bd->bi_boot_params);
		  }

		/*
		 * pass address parameter as argv[0] (aka command name),
		 * and all remaining args
		 */
		rc = do_go_exec ((void *)addr, argc - 1, argv + 1);
		if (rc != 0) rcode = 1;
	
		printf ("## Start Kernel terminated, rc = 0x%lX\n", rc);
		return rcode;

}

/* -------------------------------------------------------------------- */
int do_start_kernel (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
		ulong	addr, rc;
		int 	rcode = 0;
		kernel_entry_t	 p = NULL;
		char *linux_cmdline = NULL;
		char new_cmdline[CMDLINE_MAX_SIZE];
		unsigned long machid;
		
		setup_boot_prep_linux();
				
		if (argc > 2) 
		{
			printf ("Usage:\n%s\n", cmdtp->usage);
			return 1;
		}
		else if(argc == 1)
		{
			addr = CONFIG_SYS_START_KERNEL_ADDR;	
		}
		else
		{
			addr = simple_strtoul(argv[1], NULL, 16);
		}

#if defined(CONFIG_CMD_USB)
		usb_stop();
#endif
		printf ("## Starting Kernel at 0x%08lX ...\n", addr);

		linux_cmdline = (char *)getenv("cmdline");
		
		if(linux_cmdline == NULL)
		{
			printf("no found cmdline,use default!\n");
			linux_cmdline = KERNEL_CMD_LINE_DEFAULT;
		}else{
			printf("get cmdline 0x%x\n",(unsigned int)linux_cmdline);
		}
		
		p = (kernel_entry_t)(addr); 
	
		printf("flush cache %s %d \n",__func__,__LINE__);

		flush_cache((ulong)addr, KERNEL_MAX_SIZE);
	
		sprintf( new_cmdline,  "%s", linux_cmdline );
#ifdef CONFIG_DTV_SMC
		if (getenv("armor_config"))
		{
			const char* tmp = getenv("armor_config");
			if ((sizeof(new_cmdline) - strlen(new_cmdline)) > (strlen(tmp) + 2))
			{
				strcat(new_cmdline, " ");
				strcat(new_cmdline, tmp);
			}
			else
			{
				printf("error: small buffer!!\n");
			}
		}
#endif
		printf("cmdline passed to kernel = %s\n",new_cmdline);
	
		
		//flush_dcache_range((ulong)cmdlineparam.cmd_line, (ulong)cmdlineparam.cmd_line + sizeof(new_cmdline));
		if (new_cmdline == NULL) 
			printf(".....No cmdline is passed\n");
		else {
			printf(" address of param taglist = %x\n",(unsigned int)gd->bd->bi_boot_params);
			printf(" Start Kernel......\n");

#ifndef CONFIG_ARCH_SIGMA_TRIX
#ifndef CONFIG_BYPASS_L2		
			l2cache_init();
#endif
#endif
			/*
 			* build the taglist for kernel...
 			*/ 
			setup_start_tag(gd->bd);
			setup_commandline_tag(gd->bd,new_cmdline);
			setup_end_tag(gd->bd);
			/*pass machine id to kernel*/
			machid = gd->bd->bi_arch_number;
		
			p(0, machid, gd->bd->bi_boot_params);
		  }

		/*
		 * pass address parameter as argv[0] (aka command name),
		 * and all remaining args
		 */
		rc = do_go_exec ((void *)addr, argc - 1, argv + 1);
		if (rc != 0) rcode = 1;
	
		printf ("## Start Kernel terminated, rc = 0x%lX\n", rc);
		return rcode;

}

/* -------------------------------------------------------------------- */

U_BOOT_CMD(
         start_kernel, CONFIG_SYS_MAXARGS, 1, do_start_kernel,
	"start_kernel   - Boot up Kernel \n",
	"start_kernel $start_addr $cmdline"
);

U_BOOT_CMD(
         crc_start_kernel, CONFIG_SYS_MAXARGS, 1, do_crc_start_kernel,
	"crc_start_kernel   - Crc buf and Boot up Kernel \n",
	"crc_start_kernel $crc buf $cmdline"
);
#endif
