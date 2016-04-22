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

//#define CMD_SETXENV_DEBUG

#include <common.h>
#include <command.h>
#include <watchdog.h>
#include <stdbool.h>
#include <linux/ctype.h>
#include <nand.h>
#include <mmc.h>
#include <spi_flash.h>

#if defined (CONFIG_TANGO4)
#include <asm/arch/setup.h>
#include <asm/arch/xenvkeys.h>
#endif

#ifdef	CMD_SETXENV_DEBUG
#define	PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define PRINTF(fmt,args...)
#endif

#define MAXMUM_LENGTH 4096

// 
#define CTRL_C          ('C'-0x40)
#define CR              0x0D

// temp def
#define SHELL_ERROR_SYNTAX -1
#define SHELL_ERROR_OPTION -2
#define SHELL_ERROR_FAILED -3

extern unsigned long xenv_addr; 
//extern unsigned long xenv_base;
extern unsigned long *xenv_mem;
//bool flush_xenv_block = true;
unsigned long *tmp_xenv_mem;
bool force_xenv_addr = false;
bool multi_binary    = false;
bool del_key         = false;


char *shell_error_data;
static char     *alt_xenv_mem;
static uint32_t binary_count = 0;
static char     buf[MAXMUM_LENGTH];
static unsigned char tmp_buf[MAXMUM_LENGTH];

static uint32_t *memAddr;
static uint32_t memLen;


extern int xenv_isvalid(u32 *base, u32 maxsize);
extern int xenv_get(u32 *base, u32 size, char *recordname, void *dst, u32 *datasize);
extern int xenv_set(u32 *base, u32 size, char *recordname, void *src, u8 attr, u32 datasize);


static void xenv_record_value_display(unsigned char *buf, uint32_t count)
{
    if ( count != sizeof(uint32_t) || 
         ( buf[0]=='\"' && buf[3]=='\"' && isprint(buf[1]) && isprint(buf[2])) ) {
	
        int all_print=true,j;

        for ( j=0; j < count; j++ ) {
            if ( !isprint(buf[j]) ) {
                if (buf[j] == 0xa)
                    continue;
                if (buf[j] || j != count-1) {
                    all_print=false;
                    break;
                }
            }
        }
        
        if ( !all_print ) {
            for ( j = 0; j < count; j++ ) {
                if ( (j%16) == 0 )
                    printf("\n");
                printf( "%02x ",(unsigned char)buf[j] );
            }
        }
        else {	
            buf[count] = '\0';
            printf("%s", buf);
        }
    }
    else if ( count == sizeof(uint32_t) ) {
        
        int i = 0;
        
        // check to see if all the values are printable
        for( i = 0; i < count; i++ ) {
            if( !(buf[i] >= ' ' && buf[i] <= '~') )
            break;
        }
        
        // if so, print as ascii
        if ( i == count ) {
            for( i = 0; i < count; i++ )
                printf( "%c", buf[i] );
            
            memcpy( &count, buf, sizeof(uint32_t) );
            printf(" (0x%08x)", count);
        }
        else {
            // print it as value too
            memcpy( &count, buf, sizeof(uint32_t) );
            printf("0x%08x", count);
        }
        
    }
    else {
        memcpy( &count, buf, sizeof(uint32_t) );
        printf("0x%08x", count);
    }
}


static uint32_t xenv_list(char *base, unsigned long size)
{
	int i;
	int env_size;
	unsigned long records=0;

	env_size = xenv_isvalid((uint32_t *)base,size);
	
	if ( env_size < 0 ) 
		return -1; //SHELL_ERROR_FAILED;
	
#if 1 //defined(TANGO3_CHANGE_IMPL) || defined(TANGO3_USE_ROM_IMPL)
	i=36; 			// jump over header
#else // defined(TANGO3_CHANGE_IMPL) || defined(TANGO3_USE_ROM_IMPL)
	i=24; 			// jump over header
#endif // defined(TANGO3_CHANGE_IMPL) || defined(TANGO3_USE_ROM_IMPL)
	
	while( i < env_size ) {
		char rec_attr;
		unsigned short rec_size;
		char *recordname;
		unsigned long key_len;
		unsigned char *x;
		//int j;
		//unsigned long val;
        
		rec_attr   = ( base[i]>>4 ) & 0xf;
		rec_size   = ( (base[i]&0xf) << 8 ) + ( ((unsigned short)base[i+1]) & 0xff );
		recordname = base + i + 2;
		key_len    = strlen( recordname );
		x          = (unsigned char *)( recordname + key_len + 1 );
		
		printf("(0x%02x) %4ld %s ", rec_attr, (rec_size - 2 - (key_len+1)), recordname);

		memcpy( tmp_buf, x, rec_size - 2 - (key_len + 1) );
		xenv_record_value_display( tmp_buf, rec_size - 2 - (key_len + 1) );
		printf("\n");
		
		records++;
		i+=rec_size;
	}
	
	printf("%lu records, %d bytes\n\n", records, env_size);
	return 0; //(OK);
}



static int is_valid_numeric_string(const char *str)
{
	int base = 10;

	if (str == NULL)
		return(0);
        
	else if (!isdigit(*str))
		return(0); /* Not a valid numeric ASCII string */
        else if ((*str == '0') && (toupper(*(str + 1)) == 'X')) {
		base = 16;
		str += 2;
	}

	for (; *str != '\0'; str++) {
		if (base == 10) {
			if (!isdigit(*str))
				return(0);
		} else {
			if (!isxdigit(*str))
				return(0);
		}
	}
	return(1);
}

static uint32_t string2num(const char *str)
{
    uint32_t res = 0;
    int base = 10;
    int digit;
    int i;

    if ((*str == '0') && (toupper(*(str + 1)) == 'X')) {
	 base = 16;
	 str += 2;
    }

    for (i = 0; *str != '\0'; str++, i++) {
	 if (isdigit(*str))
	     digit = (*str - '0');
	 else if (base == 16) {
		 if (isxdigit(*str))
			 digit = (toupper(*str) - 'A') + 10;
		 else
			 return(res);
	 } else 
		 return(res);
	 res *= base;
	 res += digit;
    }
    return(res);
}


static uint32_t flush_xenv( void )
{
    /* just print out message for the user */
    printf( "Please place \"setxenv commit\" command to save your modifications permanently.\n");
    
    return 0;
}

static uint32_t get_options( uint32_t argc, 
                             char     **argv, 
                             char     **name,
                             char     **value,
                             bool     *binary )
{
    bool   ok = true;
    char   *token;
    uint32_t arg;
    uint32_t error = SHELL_ERROR_SYNTAX;

    force_xenv_addr = false;
    multi_binary    = false;
    
    if (xenv_mem == NULL)
	    return SHELL_ERROR_FAILED;

    /* Defaults */
    *name  = NULL;
    
    if( value )
        *value  = NULL;
    
    if( binary )
        *binary = false;

    for( arg = 1; ok &&  (arg < argc) && (token = argv[arg]); arg++ ) {

        if( strcmp( token, "-b" ) == 0 ) {
            if( binary )
                *binary = true;
            else {
                error            = SHELL_ERROR_OPTION;
                shell_error_data = token;
                ok               = false;
            }
        }
        else if ( strcmp(token, "-mb") == 0 ) {
            multi_binary = true;
            
            if( binary )
                *binary = true;
            else {
                error		     = SHELL_ERROR_OPTION;
                shell_error_data = token;
                ok		         = false;
            }
        }
        else if ( strcmp(token, "-a") == 0 ) {
            force_xenv_addr = true;
            if (!isalpha(*argv[arg+1]) && is_valid_numeric_string(argv[arg+1])) {
                alt_xenv_mem = ((char*)((uint32_t *)string2num(argv[arg+1])));
                arg++;
            } 
            else {
                error = SHELL_ERROR_SYNTAX;
                shell_error_data = argv[arg+1];
                ok = false;
            }
        }
        else if ( *token == '-' ) {
            error	         = SHELL_ERROR_OPTION;
            shell_error_data = token;
            ok		         = false;
        }
        else if ( strcmp(token, "commit") == 0 ) {

            *name  = token;
            *value  = (char*)0xff;  // dummy value to hack the code
        }
        else if( !(*name) ) {
            
            if( isalpha( *token ) )
                *name  = token;
            else
                ok     = false;
        }
        else if( value && !(*value) ) {
            *value = token;
            
            if (multi_binary) {
                
                uint32_t i, val;

                binary_count = 0;
                
                for (i = arg; i < argc; i++) {
                
                    if (!is_valid_numeric_string(argv[i])) {
                
                        printf("Not a valid numeric string (%s)\n", argv[i]);
                        error = SHELL_ERROR_SYNTAX;
                        shell_error_data = argv[i];
                        ok = false;
                        break;
                    }
                    else {
                        val = string2num(argv[i]);
                        memcpy(buf+(binary_count*sizeof(uint32_t)), &val, 4);
                        binary_count++;
                    }
                }
                break;
            }
        }
	else
	    ok = false;
    }   

    //return ok ? OK : error;
    return ok ? 0 : error;
}



static uint32_t get_blk_options( uint32_t argc, char **argv, char **name, bool *action )
{
    bool        ok     = true;
    char        *token = NULL;
    uint32_t    arg;
    uint32_t    error  = SHELL_ERROR_SYNTAX;

    memLen = 0xFFFFFFFF;
    memAddr = NULL;

    force_xenv_addr = false;
    if (xenv_mem == NULL)
	    return SHELL_ERROR_FAILED;

    /* Defaults */
    *name  = NULL;
    
    if( action )
        *action = false; //display

    for( arg = 1; ok && (arg < argc) && (token = argv[arg]); arg++ ) {
        if( strcmp( token, "-k" ) == 0 ) {
            if (isalpha(*argv[arg+1])) {
                *name = argv[arg+1];
                arg++;
            }
            else {
                error            = SHELL_ERROR_SYNTAX;
                shell_error_data = argv[arg+1];
                ok               = false;
            }
        }
        else if ( strcmp(token, "-m") == 0 ) {
            if ( !isalpha(*argv[arg+1]) && is_valid_numeric_string(argv[arg+1]) ) {
                memAddr = (uint32_t *)string2num(argv[arg+1]);
                *action = true;
                arg++;
            }
            else {
                error            = SHELL_ERROR_SYNTAX;
                shell_error_data = argv[arg+1];
                ok               = false;
            }
        }
        else if ( strcmp(token, "-l") == 0 ) {
            if (!isalpha(*argv[arg+1]) && is_valid_numeric_string(argv[arg+1])) {
                memLen = (uint32_t)string2num(argv[arg+1]);
                arg++;
            }
            else {
                error            = SHELL_ERROR_SYNTAX;
                shell_error_data = argv[arg+1];
                ok               = false;
            }
        }
        else if( strcmp( token, "-d" ) == 0 ) {
             if (isalpha(*argv[arg+1])) {
                *name = argv[arg+1];
                arg++;
                del_key = true;
            }
            else {
                error            = SHELL_ERROR_SYNTAX;
                shell_error_data = argv[arg+1];
                ok               = false;
            }            
        }
        else if ( *token == '-' ) {
                error	         = SHELL_ERROR_OPTION;
                shell_error_data = token;
                ok		         = false;
        }
        else if( !(*name) ) {
            if( isalpha( *token ) )
                *name  = token;
            else
                error = SHELL_ERROR_SYNTAX;
            shell_error_data = token;
            ok     = false;
        }
        else
            ok = false;
    }   
	
    if ( (NULL != memAddr) &&  (0xFFFFFFFF == memLen) ) { 
        printf("The length must be specified!\n");
        ok = false;
    }
	
    if ( (0xFFFFFFFF != memLen) && (MAXMUM_LENGTH < memLen) ) {
        printf("The maximum length must be less than 4096 bytes!\n");
		ok = false;
	}

    return ok ? 0 /*OK*/ : error;
}

//This a function to display block  data in hexidecimal format
static void block_display( char *keyname, char *tmp_buf, uint32_t len )
{
    uint32_t         rlen;
    uint32_t         i;
    uint32_t         j;
	
    printf("%s in hexidecimal : \n", keyname);
	
    if ( 0xFFFFFFFF == memLen )	
        rlen=len;
    else 
        rlen=memLen;

    for ( i=0; i < rlen; i++ ) {
        j=i;
        if ( 0 != j && 0 == j%16 ) 
            printf("\n");
        printf("%02x ", 0xff & tmp_buf[j]);
    }
    printf("\n");
}


//This is function to execute memory block attachment and display the key's content
static int set_blk( int32_t argc, char **argv )
{
    /* Options */
    char	        value[MAXMUM_LENGTH];
    char            *keyname=NULL;
    char            *pvalue=NULL;
    bool            action = false;
    int32_t         rc;

    int32_t         rlen;
    int32_t         wlen;
    int32_t         i;
    //int32_t         j;
 
    if ( argc > 1 )
        rc = get_blk_options( argc, argv, &keyname, &action );
    else 
        return 0;

    if (force_xenv_addr)
        tmp_xenv_mem = (unsigned long*)alt_xenv_mem;
    else
        tmp_xenv_mem = xenv_mem;

    if( rc != 0 /*OK*/ )
        return rc;
    else {
        if( xenv_addr && tmp_xenv_mem &&  keyname ) {
            if ( !action ) {
                if ( del_key ) {
                    // delete key
                    del_key = false;
                    
                    if (xenv_get((void *)tmp_xenv_mem, MAX_XENV_SIZE, keyname, tmp_buf, (u32*)&rlen) == RM_OK) {
                        if (xenv_set((u32 *)tmp_xenv_mem, MAX_XENV_SIZE, keyname, NULL, 0, 0) != RM_OK) {
                            printf("unsetxenv (%s) -- failed\n", keyname);
                        }
                        flush_xenv();
                    }
                    else
                        printf("Unsetting non-existed key: %s.\n", keyname);
                        
                    return 0 /*OK*/;
                }
                else {

                    // display one entry only
                    rlen = MAXMUM_LENGTH;
                    if (xenv_get((void *)tmp_xenv_mem, MAX_XENV_SIZE, keyname, tmp_buf, (u32*)&rlen) == RM_OK) {
                        block_display(keyname,(char*)tmp_buf, rlen);
                    }
                    else
                        printf("Error: %s not found\n", keyname);
                        return 0 /*OK*/;
                }
            }
            else {
                // Append memory data here
                pvalue = (char *) memAddr;
                wlen   = memLen;
                
                for ( i = 0; i < wlen; i++ ) {
                    value[i] = 0xFF & *pvalue++;			      
                }
		      
                value[i] = 0x0;
                
                if (xenv_set((u32*)tmp_xenv_mem, MAX_XENV_SIZE, keyname, value, 0, wlen) != RM_OK) {
                    printf("Set XENV (%s) -- failed\n", keyname); 
                    return SHELL_ERROR_FAILED;
                } 
                else {
                    block_display(keyname,value,wlen);
                }
            }
            rc = flush_xenv();
        } 
    }
	return rc;
}

/*
 * save modificatio into the nand flash
 * 
 */
void commit_xenv( void )
{
    char   ch;
    char   *boot_dev;
    struct mmc *mmc;

    size_t blocksize, len;
    u32    offset;
    u32    good_blk_cnt = 0;
    nand_erase_options_t nand_erase_options;

    printf("Are you sure to commit the change? (N/y) ");
    
    while ( !(ch = getc()) );
    
    printf("%c\n", ch);
    
    /* when user just press enter... take it as no */
    if ( ch == 0xd )
        ch = 'n';
    
    if (ch != 'Y' && ch != 'y' ) {
        printf("Commit Aborted!\n");
        return;
    }


    /* check to see if where we boot from*/
    boot_dev = getenv("bootdev");

#if defined ( CONFIG_TANGOX_MMC )
    if ( strncmp( boot_dev, "emmc", 4 ) == 0 ) {

        int dev = CONFIG_XENV_EMMC_DEV;

        mmc = find_mmc_device(dev);
        
        if (!mmc) {
            /* it shouldn't be happen... */
			printf("no mmc device at slot %x\n", dev);
			return;
		}
        mmc_init(mmc);
    
        /* zxenv0 offset:  0x700, zxenv1 offset: 0x800 */
        // for the test writ the zxenv at 0x2800 (5MB) offset 
        {
            u32 blk, cnt, n;
            void *addr;
            
            blk   = CONFIG_XENV_EMMC_OFFSET0;
            cnt   = ((*xenv_mem) + (512-1)) / 512;
            addr  = (void*)xenv_mem;
            
            n = mmc->block_dev.block_write(&mmc->block_dev, blk,
                                  cnt, addr);
                                  
            printf("Wrote %d blocks at 0x%x: %s\n",
                    n, blk, (n == cnt) ? "OK" : "ERROR");

            blk   = CONFIG_XENV_EMMC_OFFSET1;

            n = mmc->block_dev.block_write(&mmc->block_dev, blk,
                                  cnt, addr);

            printf("Wrote %d blocks at 0x%x: %s\n",
                    n, blk, (n == cnt) ? "OK" : "ERROR");
            
        }

    }
#endif

#if defined( CONFIG_NAND_TANGOX )
    if ( strncmp( boot_dev, "nand", 4 ) == 0 ) {
    /* in case we boot from NAND */    
    
        /* minimal sanity check */
        blocksize = nand_info[0].erasesize;
        
        if (!blocksize) {
            printf("Invalid block size!!!\n");
            return;
        }
        
        
        /* 
         * find good 7th good block from block 0. search up to 0x200000.  
         * (i.e block 6 will be 7th good block, if there are no bad block between blk 0 and blk 5) 
         */
        for ( offset = 0; offset < 0x200000; offset+= blocksize ) {
            
            if ( good_blk_cnt == 6 )
                break;
                
            if (nand_block_isbad(&nand_info[0], offset))
                continue;
            else 
                good_blk_cnt++;
        }
        
        
        /*
         * erase and write zxenv0
         */
        printf("Update zxenv0 ( offset: 0x%x )\n", offset );

        /* prepare erase struct and erase the block */
        memset(&nand_erase_options, 0, sizeof(nand_erase_options));
        nand_erase_options.length = blocksize;
        nand_erase_options.offset = offset;
        
        if ( nand_erase_opts(&nand_info[0], &nand_erase_options )) {
            printf( "Fail to erase zxenv0 !!!\n");
            return;
        }

        /* update zxenv0 */
        len = blocksize;
        if ( nand_write(&nand_info[0], offset, &len, (unsigned char*)xenv_mem) ) {
            printf( "Fail to write zxenv0 !!!\n");
            return;
        }
        
        
        /*
         *  search next good block to save zxenv1
         */ 
        do {
            offset += blocksize;
            if ( !nand_block_isbad(&nand_info[0], offset) )
                break;
        }while ( offset < 0x200000 );
        
        
        /*
         * erase and write zxenv1
         */
        printf("Update zxenv1 ( offset: 0x%x )\n", offset );
        
        /* prepare erase struct and erase the block */
        memset(&nand_erase_options, 0, sizeof(nand_erase_options));
        nand_erase_options.length = blocksize;
        nand_erase_options.offset = offset;
        
        if ( nand_erase_opts(&nand_info[0], &nand_erase_options )) {
            printf( "Fail to erase zxenv1 !!!\n");
            return;
        }

        /* update zxenv1 */
        len = blocksize;
        if ( nand_write(&nand_info[0], offset, &len, (unsigned char*)xenv_mem) ) {
            printf( "Fail to write zxenv1 !!!\n");
            return;
        }
    } // end of else
#endif
    
#if defined ( TANGOX_SPI )    
    if ( strncmp( boot_dev, "spi", 3 ) == 0 ) {
        /*
         * spi flash block: 64K
         * phyblock : 128k * 2 = 256k = 64k * 4
         * thimble  : 256k * 2 = 512k = 64k * 8
         * zxenv    : 128k * 2 = 256 (starts from 13th block)
         */
        unsigned int bus    = CONFIG_TANGOX_SPI_BUS;
        unsigned int cs     = CONFIG_TANGOX_SPI_CS;
        unsigned int speed  = CONFIG_SF_DEFAULT_SPEED;
        unsigned int mode   = CONFIG_SF_DEFAULT_MODE;
        unsigned int offset;
        int ret = 0;

        struct spi_flash *spi_fl = NULL;

        /* probe spi flash first */
        spi_fl = spi_flash_probe(bus, cs, speed, mode);
        if (!spi_fl) {
            printf("Failed to initialize SPI flash at %u:%u\n", bus, cs);
            return;
        }

        /* update the first xenv */
        offset = spi_fl->erase_size * 12; // 13th block

        ret = spi_flash_erase(spi_fl, offset, spi_fl->erase_size);

        if ( ret ) {
            printf("Fail to erase...\n");
            goto spi_error;
        }

        ret = spi_flash_write(spi_fl, offset, spi_fl->erase_size, xenv_mem);

        if ( !ret ) {
            printf( "Update xenv at 0x%x\n", offset );
        }
        else {
            printf( "Faile to update xenv at 0x%x\n", offset );
            goto spi_error;
        }

        /* update the second xenv */
        offset = spi_fl->erase_size * 12; // 13th block

        ret = spi_flash_erase(spi_fl, offset, spi_fl->erase_size);

        if ( ret ) {
            printf("Fail to erase...\n");
            goto spi_error;
        }

        ret = spi_flash_write(spi_fl, offset, spi_fl->erase_size, xenv_mem);

        if ( !ret ) {
            printf( "Update xenv at 0x%x\n", offset );
        }
        else {
            printf( "Faile to update xenv at 0x%x\n", offset );
        }

spi_error:
        if ( spi_fl )
            spi_flash_free(spi_fl);
    }
#endif

    if ( strncmp( boot_dev, "sata", 4 ) == 0 ) {
        printf("WARNING: Cannot update the xenv in SATA!\n");
        printf("Change xenv device with the following command and try it again.\n");
        printf("    NAND: setenv bootdev nand\n");
        printf("    SPI : setenv bootdev spi\n");
        printf("    eMMC: setenv bootdev emmc\n");
    }
}

int do_setxenv ( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    char            *name  = NULL;
    char	        *value = NULL;
    bool            binary = false;
    uint32_t        rc;
    uint32_t        nval;
 
    rc = get_options( argc, (char**)argv, &name, &value, &binary );
    
    if (force_xenv_addr)
        tmp_xenv_mem = (unsigned long*)alt_xenv_mem;
    else
        tmp_xenv_mem = xenv_mem;    

    PRINTF("tmp_xenv_mem: 0x%p\n", (void*)tmp_xenv_mem );
    PRINTF("rc: %d\n", rc );    
   
    if( rc != 0 /*OK*/ ) {
        return set_blk( argc, (char**)argv);
    }
    else {
        if( xenv_addr && tmp_xenv_mem ) {
            if( !name ){   
                return(xenv_list((char *)tmp_xenv_mem, MAX_XENV_SIZE));
            }
            else {

                int32_t is_numeric = 0;

                if ( !value ) {
                    // display one entry only
                    nval = MAXMUM_LENGTH;
                        
                    if ( xenv_get((void *)tmp_xenv_mem, MAX_XENV_SIZE, name, tmp_buf, &nval) == RM_OK ) {
                        printf("%s = ", name);
                        xenv_record_value_display(tmp_buf, nval);
                        printf("\n");
                    }
                    else
                        printf("Error: %s not found\n", name);

                    return 0 /*OK*/;
                }
                
                /* commit xenv changes to nand */
                if (strcmp(name, "commit") == 0) {
                    
                    commit_xenv();
                    
                    return 0;
                }
                else if (strcmp(name, XENV_KEY_UBOOT_IPADDR) == 0) {
                    debug( "new ip => %s\n", value );
                    setenv( "ipaddr", value );
                }
                else if (strcmp(name, XENV_KEY_UBOOT_SUBNET) == 0) {
                    debug( "new subnet => %s\n", value );
                    setenv( "netmask", value );
                }
                else if (strcmp(name, XENV_KEY_UBOOT_GATEWAY) == 0) {
                    debug( "new gateway => %s\n", value );
                    setenv( "gatewayip", value );
                }
                else if (strcmp(name, XENV_KEY_UBOOT_SVRIP) == 0) {
                    debug( "new svr ip => %s\n", value );
                    setenv( "serverip", value );
                }

                if (binary) {
                    if (!is_valid_numeric_string(value)) {
                        printf("Not a valid numeric string (%s)\n", value);
                        return SHELL_ERROR_SYNTAX;
                    } else 
                        is_numeric = 1;
                }
                else { 
                    
                    if (is_valid_numeric_string(value)) {
                        printf("Warning: Detected valid numeric string (%s) to be stored as ASCII format.\n", value);
                        char ch;
repeat_continue:
                        printf("Continue? [Y/n] ");
                        
#if 0                        
                        while (!GETCHAR( DEFAULT_PORT, &ch)) ;
#else
                        while ( !(ch = getc()) );
#endif                        
                        
                        printf("%c\n", ch);
					
                        if (ch == 'N' || ch == 'n' || ch == CTRL_C) {
                            printf("setxenv command aborted!\n");
                            return 0 /*OK*/;
                        }
                        else if (ch != 'Y' && ch != 'y' && ch != CR)
                            goto repeat_continue;
                    }
				
                    sprintf(buf, "%s", value);
                    value = &buf[0];
                } 

                nval = MAXMUM_LENGTH;
                if (xenv_get((void *)tmp_xenv_mem, MAX_XENV_SIZE, name, tmp_buf, &nval) == RM_OK) {
                    printf("Original value: ");
                    xenv_record_value_display(tmp_buf, nval);
                    printf("\n");
                } else {
                    printf("New key %s, ", name);
                }

                if (is_numeric) {
                    if (!multi_binary) {	
                        nval = string2num(value);
                        if (xenv_set((u32*)tmp_xenv_mem, MAX_XENV_SIZE, name, &nval, 0, sizeof(uint32_t)) != RM_OK) {
                            printf("Set XENV (%s) (binary 0x%x) -- failed\n", name, nval); 
                            return SHELL_ERROR_FAILED;
                        } 
                        else {
                            printf("New value: 0x%08x\n", nval);
                        }
                    }
                    else {
                        if (xenv_set((u32*)tmp_xenv_mem, MAX_XENV_SIZE, name, buf, 0, binary_count*sizeof(uint32_t)) != RM_OK) {
                            printf("Set XENV (%s) to multiple binary values -- failed\n", name); 
                            return SHELL_ERROR_FAILED;
                        } 
                        else {
                            uint32_t i;
                            printf("New value:");
                            for (i=0; i < binary_count; i++) {
                                if ((i%4) == 0)
                                printf("\n");
                                printf("0x%08x ", *(uint32_t *)(buf+(i*sizeof(uint32_t))));
                            }
                            printf("\n");
                        }
                    }
                } 
                else {
                    if (xenv_set((u32*)tmp_xenv_mem, MAX_XENV_SIZE, name, value, 0, strlen(value)) != RM_OK) {
                        printf("Set XENV (%s) (ASCII %s) -- failed\n", name, value); 
                        return SHELL_ERROR_FAILED;
                    } 
                    else {
                        printf("New value: %s\n", value);
                    }
                }

                if( rc != 0 /*OK*/ )
                    return rc;

                rc = flush_xenv();
            }
        } 
        return rc;
    }
        
    return 0;
}


U_BOOT_CMD(
	setxenv,	20,	0,	do_setxenv,
	"set, delete or display xenv keys",
    "[-b] [-a <address>] [<key> [<value>]]\n"
    "setxenv [-mb] [-a <address>] [<key> [<value1> <value2> ... ]]\n"
    "setxenv [ -k <key> ] [ -m <address> -l <length> ]\n"
    "setxenv [ -k <key> [ -l <length> ] ]\n"
    "setxenv [ -d <key> ]\n\n"
    "Set a XENV key with the given options:\n"
    "   -b for inserting a key in binary mode.\n"
    "   -mb for inserting a key in multiple binary mode.\n"
    "   -a <address> for a specific XENV block in the memory address.\n"
    "   -m <address> is a starting address of the memory block to set a XENV key.\n"
    "   -k <key> is a special key to keep in mind the memory block data.\n"
    "   -l <length> is a length of the memory block.\n"
    "   The length must be specified to set a XENV key to append a memory block.\n"
    "   The maximum length should be less than the 4096 bytes long.\n"
    "Display XENV keys with the given options:\n"
    "   It will display all XENV keys, if no options are specified.\n"
    "   <key name> for display the key value.\n"
    "Delete a XENV key witht the given option.\n"
    "   -d <key name> for deleting a key.\n"
    "User modification can be saved in non-volatile memory with given command:\n"
    "   setxenv commit\n"
);
