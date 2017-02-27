/***************************************************************************
*                                                                  		
*           Copyright (c) 2003 Trident Microsystems, Inc.              	
*                       All rights reserved                          		
*                                                                          
* The content of this file or document is CONFIDENTIAL and PROPRIETARY     
* to Trident Microsystems, Inc.  It is subject to the terms of a           
* License Agreement between Licensee and Trident Microsystems, Inc.        
* restricting among other things, the use, reproduction, distribution      
* and transfer.  Each of the embodiments, including this information and   
* any derivative work shall retain this copyright notice                   
*                                                                          
****************************************************************************
*
* File:            test.c                                            	             
*                                                                            
* Purpose:         This is a sample to use HiDTV registers in user space
*                                                                          
* modification history                                                     
* --------------------												    
* 07-29-2003 Mia initial version            					            
****************************************************************************/
#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/arch/reg_io.h>
#include <malloc.h>

#ifdef CONFIG_LOADSETTINGS_TRACE
#  define TraceWriteReg(t, a, v) printf("w %s 0x%08x = 0x%x\n", t, a, v)
#  define TraceReadReg(t, a) printf("r %s 0x%08x\n", t, a)
#  define WriteRegByte(a, v) do{TraceWriteReg("b ", a, v);WriteRegByte(a,v);}while(0)
#  define WriteRegHWord(a, v) do{TraceWriteReg("hw", a, v);WriteRegHWord(a, v);}while(0)
#  define WriteRegWord(a, v) do{TraceWriteReg("w ", a, v);WriteRegWord(a,v);}while(0)
#  define ReadRegByte(a) ({TraceReadReg("b ", a);ReadRegByte(a);})
#  define ReadRegHWord(a) ({TraceReadReg("hw", a);ReadRegHWord(a);})
#  define ReadRegWord(a) ({TraceReadReg("w ", a);ReadRegWord(a);})
#endif

#define MAX_LINE_LENGTH 1024

static char *strtrim(char *str)
{
	char *p = str;
	char *q;

	if ((q = strstr(str, "//")))
		*q = '\0';
	
	while  ((*p == ' ') || (*p == '\n') ||(*p == '\r') || (*p =='\t'))
		p++;
	
	q = p + strlen(p) - 1;
	while (q > p)
	{
		if  ((*q == ' ') || (*q == '\n') ||(*q == '\r') || ( *q == '\t'))
			q--;
		else
			break;
	}
	*(q + 1) ='\0';
	return p;			
}

static void inline delay_cycle(int delay)
{
	int i;
	for (i = 0 ; i < delay; i++)
	{
  	  __asm__ __volatile__( 
		"nop\n"
	    	);
	}
}


static void read_reg(int mode, u32 addr, int delay)
{
	u32 val = 0;

	switch(mode)
	{
		case 0:
			val = ReadRegByte(addr);
			delay_cycle(delay);
			break;
		case 1:
			val = ReadRegHWord(addr);
			delay_cycle(delay);
			break;
		case 2:
			val = ReadRegWord(addr);
			delay_cycle(delay);
			break;
	}
	val = val;	/*cheat compiler*/
}

#define MASKREGVAL(RegVal, Mask, Val) do{		\
			(RegVal) &= (Mask);		\
			(RegVal) |= (Val) & (~Mask);	\
		}while(0)

static void write_reg(int mode, u32 addr,  u32 mask, u32 val, int delay)
{
	u32 RegValue = val;

	switch(mode)
	{
		case 0:
			if(mask!= 0)
			{
				RegValue = ReadRegByte(addr);
				MASKREGVAL(RegValue, mask, val);
			}
			WriteRegByte(addr, RegValue);
			delay_cycle(delay);
			break;
		case 1:
			if(mask!= 0)
			{
				RegValue = ReadRegHWord(addr);
				MASKREGVAL(RegValue, mask, val);
			}
			WriteRegHWord(addr, RegValue);
			delay_cycle(delay);
			break;
		case 2:
			if(mask!= 0)
			{
				RegValue = ReadRegWord(addr);
				MASKREGVAL(RegValue, mask, val);
			}	
			WriteRegWord(addr, RegValue);
			delay_cycle(delay);
			break;
	}
	return;
}

#ifdef CONFIG_PMAN_ENTRY_SUPPORT
#include "pman_protection.c"
#endif

static int load_settings(const void *simg, u32 len)
{	
	unsigned long addr, val, mask;
	int mode;
	int action;
	char buf[MAX_LINE_LENGTH];
	char *p, *q;
	int linecnt = 0;
	int delay = 0;
	u32 pos = 0;
	
	if(!simg || len==0)
	{
		printf("invalid settings!\n");
		return 1;
	}

	while (pos < len)
	{
		#define GETS(b,sz,strm) ({		\
			const char* _ret=NULL;		\
			const char* _p,*_q,*_t;		\
			_p = _q = simg + pos;		\
			while (pos < len){		\
				_t = _q; _q++; pos++;	\
				if (*_t == '\n') break;	\
			}				\
			if (_q > _p &&			\
			 (_q-_p) <= MAX_LINE_LENGTH){	\
				memcpy((b), _p, _q-_p);	\
				(b)[_q-_p] = '\0';	\
				_ret = (b);		\
			}				\
			_ret;				\
		})

		if (!GETS(buf, sizeof(buf), simg))
		{
			break;
		}
		linecnt++;
		p = strtrim(buf);
		if (strlen(p) == 0)
			continue;
		if (p[0] == '#')
			continue;

		if ((p[0] == 'r') || (p[0] =='R'))
			action = 0;	
		else if ((p[0] == 'w') || (p[0] =='W'))
			action = 1;
		else if ((p[0] == 'm') || (p[0] =='M'))
			action = 2; //Mask write
		else if ((p[0] == 'd') || (p[0] =='D'))
		{
			p++;
			p = strtrim(p);
			//sscanf(p, "%x", &delay);
			delay = simple_strtoul(p, 0, 16);
			printf("delay set to 0x%x\n", delay);
			continue;
		}
#ifdef CONFIG_PMAN_ENTRY_SUPPORT
		else if ((p[0] == 'p') || (p[0] =='P'))
		{
			p++;
			p = strtrim(p);
			pman_tbl_new_entry(p);
			continue;
		}
#endif
		else
		{
			printf("Line %d unknown operation: %s\n", linecnt, buf);
			continue;
		}	

#ifdef CONFIG_PMAN_ENTRY_SUPPORT
		pman_tbl_finish();
#endif

		p++;
		p = strtrim(p);

		if ((p[0] == 'b') || (p[0] =='B'))
		{
			mode = 0;	
			p++;
		}
		else if ((p[0] == 'w') || (p[0] =='W'))
		{
			mode = 2;
			p++;
		}
		//else if (strncasecmp(p, "hw", 2) == 0)
		else if (strncmp(p, "hw", 2) == 0 || strncmp(p, "HW", 2) == 0)
		{
			mode = 1;
			p+=2;
		}
		else
		{
			printf("Line %d unknown mode: %s\n", linecnt, buf);
			continue;
		}	
		p = strtrim(p);
		
		if (action == 0)
		{
			// read
			//sscanf(p, "%lx", &addr);
			addr = simple_strtoul(p, 0, 16);
			read_reg(mode, addr, delay);
		}	
		else if(action == 1)
		{
			q = strchr(p, ' ');
			if (!q)
				q = strchr(p, '\t');
			if (!q)
			{
				printf("W Line %d error: %s\n", linecnt, buf);
				continue;
			}
			*q = '\0';
			q++;
			//sscanf(p, "%lx", &addr);
			//sscanf(q, "%lx", &val);
			addr = simple_strtoul(p, 0, 16);
			val = simple_strtoul(q, 0, 16);
			write_reg(mode, addr, 0, val, delay);
		}
		else if(action == 2)
		{
			char* pCur;
			pCur = strtok(p, " \r\t\n");

			if(pCur == NULL)
			{
				printf("MW Line %d error: %s\n", linecnt, buf);
				continue;
			}
			//sscanf(pCur, "%lx", &addr);
			addr = simple_strtoul(pCur, 0, 16);

			pCur = strtok(NULL, " \r\t\n");
			if(pCur == NULL)
			{
				printf("MW Line %d error: %s\n", linecnt, buf);
				continue;
			}
			//sscanf(pCur, "%lx", &mask);
			mask = simple_strtoul(pCur, 0, 16);

			pCur = strtok(NULL, " \r\t\n");
			if(pCur == NULL)
			{
				printf("MW Line %d error: %s\n", linecnt, buf);
				continue;
			}
			//sscanf(pCur, "%lx", &val);
			val = simple_strtoul(pCur, 0, 16);
			write_reg(mode, addr, mask, val, delay);
		}
	}

#ifdef CONFIG_PMAN_ENTRY_SUPPORT
	pman_tbl_finish();	/*just in case pman settings sit in the last part*/
#endif

	return 0;
}


static int do_loadsettings(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;
	void * load_addr = NULL;
	u32 len = 0;

	if (argc < 3)
		return CMD_RET_USAGE;

	load_addr = (void*)simple_strtoul(argv[1], NULL, 16);
	len = (u32)simple_strtoul(argv[2], NULL, 16);
	printf("load initsettings.file at %p(0x%08x) ...\n", load_addr, len);

	ret = load_settings(load_addr, len);
	return ret;
}

U_BOOT_CMD(
	loadsettings, 3, 1, do_loadsettings,
	"load initsettings.file",
	"<addr> <len>\n"
);
