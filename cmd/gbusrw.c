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

//#define CMD_GBUSRW_DEBUG

#include <common.h>
#include <command.h>
#include <watchdog.h>

#if defined (CONFIG_TANGO3) || defined (CONFIG_TANGO4)
#include <asm/arch/setup.h>
#endif

#ifdef	CMD_GBUSRW_DEBUG
#define	PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define PRINTF(fmt,args...)
#endif

#if defined (CONFIG_MIPS)
extern RMuint32 sigmaremap(RMuint32 address);
#endif

int do_gbusread32 ( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    unsigned int gbus_addr;

#if defined (CONFIG_MIPS)
    unsigned int address31_26;
    unsigned int address25_0;
    unsigned int saveaddr = 0;
    unsigned int address = 0;
#endif    
    if (argc < 2)
        return cmd_usage(cmdtp);

    // convert the input address to number
    gbus_addr = simple_strtoul( argv[1], NULL, 16 );

    PRINTF("gbus_addr: 0x%lx\n", gbus_addr );

#if defined (CONFIG_MIPS)
    if( gbus_addr >= 0xe0000000) {
        printf("Please enter a physical address\n");
        return 0;
    }

    if( gbus_addr >= CPU_remap2_address && gbus_addr < 0xe0000000) {
        
        if((address = sigmaremap(gbus_addr)) == -1 ) {
        
            address31_26 = gbus_addr & (~((1<<26)-1));
			address25_0  = gbus_addr & ((1<<26)-1);

			saveaddr = gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP);
			gbus_write_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP, address31_26);

			while (gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP) != (address31_26 & 0xfc000000))
				;

			address = CPU_TMP_REMAP_address | address25_0;
		} else 
			printf("address 0x%x has been remap to 0x%x\n",gbus_addr, KSEG1ADDR(address));
	} else
		address = gbus_addr;

	printf("read: 0x%08x = 0x%08x\n",gbus_addr, (unsigned int) gbus_read_uint32(pgbus, address));

	if(saveaddr) {
		gbus_write_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP, saveaddr);
		while (gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP) != (saveaddr & 0xfc000000))
			;
	}
#else
    //printf("read: 0x%08x = 0x%08x\n",gbus_addr, (unsigned int)__raw_readl(gbus_addr));
    printf("read: 0x%08x = 0x%08x\n",gbus_addr, (unsigned int)gbus_read_uint32(pgbus, gbus_addr));
#endif    

    return 0;
}

int do_gbusread16 ( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    unsigned int gbus_addr;

#if defined (CONFIG_MIPS)
    unsigned int address31_26;
    unsigned int address25_0;
    unsigned int saveaddr = 0;
    unsigned int address = 0;
#endif
    
    if (argc < 2)
        return cmd_usage(cmdtp);

    // convert the input address to number
    gbus_addr = simple_strtoul( argv[1], NULL, 16 );

    PRINTF("gbus_addr: 0x%lx\n", gbus_addr );

#if defined (CONFIG_MIPS)
    if( gbus_addr >= 0xe0000000) {
        printf("Please enter a physical address\n");
        return 0;
    }

    if( gbus_addr >= CPU_remap2_address && gbus_addr < 0xe0000000) {
        
        if((address = sigmaremap(gbus_addr)) == -1 ) {
        
            address31_26 = gbus_addr & (~((1<<26)-1));
			address25_0  = gbus_addr & ((1<<26)-1);

			saveaddr = gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP);
			gbus_write_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP, address31_26);

			while (gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP) != (address31_26 & 0xfc000000))
				;

			address = CPU_TMP_REMAP_address | address25_0;
		} else 
			printf("address 0x%x has been remap to 0x%x\n",gbus_addr, KSEG1ADDR(address));
	} else
		address = gbus_addr;

	printf("read: 0x%08x = 0x%08x\n",gbus_addr, gbus_read_uint16(pgbus, address));

	if(saveaddr) {
		gbus_write_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP, saveaddr);
		while (gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP) != (saveaddr & 0xfc000000))
			;
	}
#else
    printf("read: 0x%08x = 0x%08x\n",gbus_addr, (unsigned int) gbus_read_uint16(pgbus, gbus_addr));
#endif    
    
    return 0;
}

int do_gbusread8 ( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    unsigned int gbus_addr;

#if defined (CONFIG_MIPS)
    unsigned int address31_26;
    unsigned int address25_0;
    unsigned int saveaddr = 0;
    unsigned int address = 0;
#endif
    
    if (argc < 2)
        return cmd_usage(cmdtp);

    // convert the input address to number
    gbus_addr = simple_strtoul( argv[1], NULL, 16 );

    PRINTF("gbus_addr: 0x%lx\n", gbus_addr );

#if defined (CONFIG_MIPS)
    if( gbus_addr >= 0xe0000000) {
        printf("Please enter a physical address\n");
        return 0;
    }

    if( gbus_addr >= CPU_remap2_address && gbus_addr < 0xe0000000) {
        
        if((address = sigmaremap(gbus_addr)) == -1 ) {
        
            address31_26 = gbus_addr & (~((1<<26)-1));
			address25_0  = gbus_addr & ((1<<26)-1);

			saveaddr = gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP);
			gbus_write_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP, address31_26);

			while (gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP) != (address31_26 & 0xfc000000))
				;

			address = CPU_TMP_REMAP_address | address25_0;
		} else 
			printf("address 0x%x has been remap to 0x%x\n",gbus_addr, KSEG1ADDR(address));
	} else
		address = gbus_addr;

	printf("read: 0x%08x = 0x%08x\n",gbus_addr, gbus_read_uint8(pgbus, address));

	if(saveaddr) {
		gbus_write_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP, saveaddr);
		while (gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP) != (saveaddr & 0xfc000000))
			;
	}
#else
    printf("read: 0x%08x = 0x%08x\n",gbus_addr, (unsigned int) gbus_read_uint8(pgbus, gbus_addr));
#endif    
    
    return 0;
}

int do_gbuswrite32 ( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    unsigned int gbus_addr;
    unsigned int data;

#if defined (CONFIG_MIPS)
    unsigned int address31_26;
    unsigned int address25_0;
    unsigned int saveaddr = 0;
    unsigned int address = 0;
#endif
    
    if (argc < 3)
        return cmd_usage(cmdtp);

    // convert the input address to number
    gbus_addr = simple_strtoul( argv[1], NULL, 16 );
    data      = simple_strtoul( argv[2], NULL, 16 );

    PRINTF("gbus_addr: 0x%lx\n", gbus_addr );

#if defined (CONFIG_MIPS)
    if( gbus_addr >= 0xe0000000) {
        printf("Please enter a physical address\n");
        return 0;
    }

    if( gbus_addr >= CPU_remap2_address && gbus_addr < 0xe0000000) {
        
        if((address = sigmaremap(gbus_addr)) == -1 ) {
        
            address31_26 = gbus_addr & (~((1<<26)-1));
			address25_0  = gbus_addr & ((1<<26)-1);

			saveaddr = gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP);
			gbus_write_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP, address31_26);

			while (gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP) != (address31_26 & 0xfc000000))
				;

			address = CPU_TMP_REMAP_address | address25_0;
		} else 
			printf("address 0x%x has been remap to 0x%x\n",gbus_addr, KSEG1ADDR(address));
	} else
		address = gbus_addr;

    printf("write: 0x%08x = 0x%08x\n",gbus_addr, data);
	gbus_write_uint32(pgbus, address, data);

	if(saveaddr) {
		gbus_write_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP, saveaddr);
		while (gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP) != (saveaddr & 0xfc000000))
			;
	}
#else
    printf("write: 0x%08x = 0x%08x\n",gbus_addr, data);
    gbus_write_uint32(pgbus, gbus_addr, data);
#endif    
    return 0;
}

int do_gbuswrite16 ( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    unsigned int gbus_addr;
    unsigned int data;

#if defined (CONFIG_MIPS)
    unsigned int address31_26;
    unsigned int address25_0;
    unsigned int saveaddr = 0;
    unsigned int address = 0;
#endif
    
    if (argc < 3)
        return cmd_usage(cmdtp);

    // convert the input address to number
    gbus_addr = simple_strtoul( argv[1], NULL, 16 );
    data      = simple_strtoul( argv[2], NULL, 16 );

    PRINTF("gbus_addr: 0x%lx\n", gbus_addr );

#if defined (CONFIG_MIPS)
    if( gbus_addr >= 0xe0000000) {
        printf("Please enter a physical address\n");
        return 0;
    }

    if( gbus_addr >= CPU_remap2_address && gbus_addr < 0xe0000000) {
        
        if((address = sigmaremap(gbus_addr)) == -1 ) {
        
            address31_26 = gbus_addr & (~((1<<26)-1));
			address25_0  = gbus_addr & ((1<<26)-1);

			saveaddr = gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP);
			gbus_write_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP, address31_26);

			while (gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP) != (address31_26 & 0xfc000000))
				;

			address = CPU_TMP_REMAP_address | address25_0;
		} else 
			printf("address 0x%x has been remap to 0x%x\n",gbus_addr, KSEG1ADDR(address));
	} else
		address = gbus_addr;

    printf("write: 0x%08x = 0x%08x\n",gbus_addr, data);
	gbus_write_uint16(pgbus, address, data);

	if(saveaddr) {
		gbus_write_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP, saveaddr);
		while (gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP) != (saveaddr & 0xfc000000))
			;
	}
#else
    printf("write: 0x%08x = 0x%08x\n",gbus_addr, data);
    gbus_write_uint16(pgbus, gbus_addr, data);
#endif    
    return 0;
}

int do_gbuswrite8 ( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    unsigned int gbus_addr;
    unsigned int data;

#if defined (CONFIG_MIPS)
    unsigned int address31_26;
    unsigned int address25_0;
    unsigned int saveaddr = 0;
    unsigned int address = 0;
#endif
    
    if (argc < 3)
        return cmd_usage(cmdtp);

    // convert the input address to number
    gbus_addr = simple_strtoul( argv[1], NULL, 16 );
    data      = simple_strtoul( argv[2], NULL, 16 );

    PRINTF("gbus_addr: 0x%lx\n", gbus_addr );

#if defined (CONFIG_MIPS)
    if( gbus_addr >= 0xe0000000) {
        printf("Please enter a physical address\n");
        return 0;
    }

    if( gbus_addr >= CPU_remap2_address && gbus_addr < 0xe0000000) {
        
        if((address = sigmaremap(gbus_addr)) == -1 ) {
        
            address31_26 = gbus_addr & (~((1<<26)-1));
			address25_0  = gbus_addr & ((1<<26)-1);

			saveaddr = gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP);
			gbus_write_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP, address31_26);

			while (gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP) != (address31_26 & 0xfc000000))
				;

			address = CPU_TMP_REMAP_address | address25_0;
		} else 
			printf("address 0x%x has been remap to 0x%x\n",gbus_addr, KSEG1ADDR(address));
	} else
		address = gbus_addr;

    printf("write: 0x%08x = 0x%08x\n",gbus_addr, data);
	gbus_write_uint8(pgbus, address, data);

	if(saveaddr) {
		gbus_write_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP, saveaddr);
		while (gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_TMP_REMAP) != (saveaddr & 0xfc000000))
			;
	}
#else
    printf("write: 0x%08x = 0x%08x\n",gbus_addr, data);
	gbus_write_uint8(pgbus, gbus_addr, data);
#endif    
    return 0;
}

/* ====================================================================== */
U_BOOT_CMD(
	gr32,	2,	1,	do_gbusread32,
	"Read memory contents starting at gbus-address.",
	" <gbus-address>"
);

U_BOOT_CMD(
	gr16,	2,	1,	do_gbusread16,
	"Read memory contents starting at gbus-address.",
	" <gbus-address>."
);

U_BOOT_CMD(
	gr8,	2,	1,	do_gbusread8,
	"Read memory contents starting at gbus-address",
	" <gbus-address>"
);

U_BOOT_CMD(
	gw32,	3,	1,	do_gbuswrite32,
	"Edit memory contents starting at gbus-address.",
	" <gbus-address> <data>"
);

U_BOOT_CMD(
	gw16,	3,	1,	do_gbuswrite16,
	"Edit memory contents starting at gbus-address.",
	" <gbus-address> <data>"
);

U_BOOT_CMD(
	gw8,	3,	1,	do_gbuswrite8,
	"Edit memory contents starting at gbus-address.",
	" <gbus-address> <data>"
);
