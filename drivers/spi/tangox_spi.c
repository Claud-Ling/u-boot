/*
 * Copyright (C) 2014 Sigma Designs - http://www.sigmadesigns.com/
 *
 * Driver for SPI controller on SMP87xx series.
 * Based on Davinchi SPI by TI
 *
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

//#define DEBUG
//#define DEBUG_DUMPDATA

#include <common.h>
#include <spi.h>
#include <malloc.h>
#include <asm/arch/setup.h>

#include "tangox_spi.h"

static uint8_t spi_scratch_buffer[16];

extern unsigned long tangox_get_sysclock(void);

#ifdef DEBUG_DUMPDATA
static inline unsigned char SHOW_ASCII(unsigned char x)
{
	if(x > 126)
		return 46;

	if(x < 33)
		return 46;

	return x;
}

void dump_memory(const unsigned char* buffer, unsigned int dump_length, char* title )
{
	unsigned int line = 0;
	unsigned int left = 0;
	unsigned int i = 0, j = 0;

	if(buffer == NULL)
	{
		printf("Buffer is not assigned\n");
		return;
	}

    if ( title != NULL )
    {
        printf("[ %s ]\n", title );
    }

	line = dump_length / 16;
	left = dump_length - (line * 16);

	/* Main Body */
	for(i = 0 ; i < line ; i++)
	{
		for(j = 0 ; j < 16 ; j++)
		{
			if(j == 0)
				printf("\n%08x | ", (i*16 + j));

			printf("%02x ", buffer[i*16 + j]);
		}

		for(j = 0 ; j < 16 ; j++)
		{
			if(j == 0)
				printf("| ");

			printf("%c", SHOW_ASCII(buffer[i*16 + j]));
		}
	}

	/* Left */
	for(j = 0 ; (j < 16) && (left > 0) ; j++)
	{
		if(j == 0)
			printf("\n%08x | ", (i*16 + j));

		if(j < left)
			printf("%02x ", buffer[i*16 + j]);
		else
			printf("   ");

	}

	for(j = 0 ; j < left ; j++)
	{
		if(j == 0)
			printf("| ");

		printf("%c", SHOW_ASCII(buffer[i*16 + j]));
	}

	printf("\n");

	return;
}
#define DUMP_MEM(a, b, c)  dump_memory(a, b, c)
#else
#define DUMP_MEM(a, b, c)
#endif




void spi_init(void)
{
    /*
     * program the pins for SPI/UART configuration
     * CPU_UART0_base    : 0xc100
     * CPU_UART_GPIOMODE : 0x38
     */
    gbus_write_reg32( REG_BASE_cpu_block + CPU_UART0_base + CPU_UART_GPIOMODE, 0xff00 );

    /*
     * program the pins for SPI configuration, single-mode, multiple Chip Selects
     */
    gbus_write_reg32( REG_BASE_cpu_block + CPU_UART0_base + CPU_UART_SPIMODE, 0x2 );
}

int spi_claim_bus(struct spi_slave *slave)
{
    return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
}

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
			unsigned int max_hz, unsigned int mode)
{
    struct tangox_spi_slave *ts;
    uint32_t val;

    if (!spi_cs_is_valid(bus, cs))
		return NULL;

    ts = spi_alloc_slave(struct tangox_spi_slave, bus, cs);

	if (!ts)
		return NULL;

	ts->regs   = (uint8_t *)(REG_BASE_cpu_block + CPU_spi_ctrl0);
	ts->hz     = max_hz;
    ts->cs     = cs;
    ts->mode   = mode;

    ts->rx_mem = (uint8_t*)(REG_BASE_cpu_block + CPU_spi_ctrl0 + REG_SPI_RD_MEM_OFFSET);
    ts->tx_mem = (uint8_t*)(REG_BASE_cpu_block + CPU_spi_ctrl0 + REG_SPI_WR_MEM_OFFSET);

    val = gbus_read_reg32( ts->regs + REG_SPI_CTRL1 );

    /* tcs start */
    val = val & ~TCS_ST_MASK;
    val = val | CTRL1_TCS_ST(0xf);

    /* tcs end */
    val = val & ~TCS_EN_MASK;
    val = val | CTRL1_TCS_EN(0xf);

    /* tcs high */
    val = val & ~TCS_HI_MASK;
    val = val | CTRL1_TCS_HI(0xf);

    gbus_write_reg32( ts->regs + REG_SPI_CTRL1, val );

    return &ts->slave;
}

void spi_free_slave(struct spi_slave *slave)
{
    struct tangox_spi_slave *ts = to_tangox_spi(slave);
	free(ts);
}


inline void spi_aligned_cp( uint8_t *iomem, uint8_t *scratch, uint32_t len )
{
    uint32_t i;
    uint32_t val;

    for ( i = 0; i < len; i +=4 ) {
        val = *((u32*)(scratch+i));
        *((u32*)(iomem + i)) = val;
    }
}

static void spi_setup_transfer( struct spi_slave *slave )
{
    struct tangox_spi_slave *ts = to_tangox_spi(slave);

    uint32_t    val;
    uint32_t    hsclk;

    ts->sys_clk = tangox_get_sysclock();
    hsclk = ts->sys_clk / ts->hz / 2;

    val = gbus_read_reg32( ts->regs + REG_SPI_CTRL0 );

    /* enable spi and clear interrupt */
    val = val | CTRL0_EN(0b1);
    val = val | CTRL0_INT(0b1);

    val = val & ~HALF_SCLK_MASK;
    val = val | CTRL0_HALF_SCLK(hsclk);
    val = val & ~CSB_MASK;
    val = val | CTRL0_CSB( ~(1 << ts->cs) );
    val = val & ~CMODE_MASK;
    val = val | CTRL0_CMODE( ts->mode );
    val = val & ~CTRL0_ENDIAN_MASK;

    gbus_write_reg32( ts->regs + REG_SPI_CTRL0, val );

}

static void spi_load_data( struct  spi_slave *slave, const uint8_t *data_out )
{
    struct tangox_spi_slave *ts = to_tangox_spi(slave);

    if ( ts->tot_len > 8 ) {

        debug("    qed_len: %d\n", ts->qed_len );
        if ( ts->qed_len > 0  ) {
            spi_aligned_cp(
                ts->tx_mem,
                spi_scratch_buffer,
                ts->qed_len );
        }

        if ( data_out != NULL ) {
            spi_aligned_cp(
                ts->tx_mem + ts->qed_len,
                (uint8_t*)data_out,
                (ts->tot_len - ts->qed_len) );
        }
    }
    else {

        u32 lsb = 0;
        u32 msb = 0;
        u32 i, len;
        uint8_t *data;

        /* when we get to here all data is in scratch buffer */
        data = spi_scratch_buffer;

        /* shift and copy the data */
        len = ( ts->qed_len > 4 ) ? 4 : ts->qed_len;
        for ( i = 0; i < len; i++ ) {
            msb = msb | (*(data+i) << (32-(8*(i+1))) );
        }

        len = ( ts->qed_len > 4 ) ? (ts->qed_len - 4) : 0;
        for ( i = 0; i < len; i++ ) {
            lsb = lsb | (*(data+i+4) << (32-(8*(i+1))) );
        }

        debug( "tx ==> lsb: 0x%x, msb: 0x%x\n", lsb, msb );

        /* set tx registers */
        gbus_write_reg32( ts->regs + REG_SPI_DW_LSB, lsb );
        gbus_write_reg32( ts->regs + REG_SPI_DW_MSB, msb );

    }
}

static void spi_start_transfer( struct  spi_slave *slave )
{
    struct tangox_spi_slave *ts = to_tangox_spi(slave);

    uint32_t ctrl0, ctrl1;

    ctrl0 = gbus_read_reg32( ts->regs + REG_SPI_CTRL0 );
    ctrl1 = gbus_read_reg32( ts->regs + REG_SPI_CTRL1 );

    ctrl1 = ctrl1 & ~BURST_LEN_MASK;
    ctrl1 = ctrl1 | CTRL1_BURST_LEN( ts->tot_len );

    if ( ts->tot_len <= 8 ) {
        /* set the length and start transaction */
        ctrl0 = ctrl0 & ~EXTMODE_MASK;
        ctrl0 = ctrl0 & ~CS_LEN_MASK;
        ctrl0 = ctrl0 | CTRL0_CS_LEN( ts->tot_len * 8 );
    }
    else {
        /* set burst len */
        gbus_write_reg32( ts->regs + REG_SPI_CTRL1, ctrl1 );

        /* set ext mode */
        ctrl0 = ctrl0 | CTRL0_EXTMODE(0b01);
        ctrl0 = ctrl0 & ~CS_LEN_MASK;
    }

    /* start transaction */
    ctrl0 = ctrl0 | CTRL0_START(0b1);

    debug("about to start, ctrl0: 0x%x\n\n", ctrl0 );

    gbus_write_reg32( ts->regs + REG_SPI_CTRL0, ctrl0 );
}

static void spi_wait_completion( struct  spi_slave *slave )
{
    struct tangox_spi_slave *ts = to_tangox_spi(slave);
    uint32_t val;
    uint32_t timeout = 3;

    while( timeout ) {

        val = gbus_read_reg32(ts->regs + REG_SPI_CTRL0);

        if ( val & CTRL0_INT(0b1) ) {
            /* clear interrupt */
            gbus_write_reg32(ts->regs + REG_SPI_CTRL0, (val | CTRL0_INT(0b1)) );
            break;
        }

        udelay(500);
        timeout--;
    }

    if ( !timeout )
        printf("spi transfer timeout!\n");
}

void spi_read_data( struct spi_slave *slave, uint8_t *data_in )
{
    struct tangox_spi_slave *ts = to_tangox_spi(slave);

    /* copy data to user buffer */
    if ( ts->tot_len > 8 ) {

        uint32_t i, j,word;
        uint32_t k = 0;
        uint32_t dummy = ts->qed_len;

        /* read data from spi rx mem */
        debug( "read data, ts->tot_len: %d, ts->qed_len: %d\n", ts->tot_len, ts->qed_len );
        for ( i = 0; i < (ts->tot_len & ~0b11) ; i +=4 ) {
            word = *((uint32_t*)(ts->rx_mem+i));
            for ( j = 0; j < 4; j++ ) {
                if ( dummy > 0 )
                    dummy--;
                else {
                    *(data_in + k) = (word >> j*8) & 0xff;
                    k++;
                }
            }
        }

        /* unaligned last word handling */
        if ( (ts->tot_len & 0b11) > 0 ) {
            uint32_t u = 4 - (ts->tot_len & 0b11);
            word = *((uint32_t*)(ts->rx_mem+i));
            for ( j = u; j < 4; j++ ) {
                *(data_in + k) = (word >> j*8) & 0xff;
                k++;
            }
        }
    }
    else {
        /*
         * - when data is less than 8, spi_dr_lsb and spi_dr_msb are used.
         * - data filled spi_dr_lsb first than spi_dr_msb (the reg name is based on tango cpu block pg. 30)
         * - each register filled with low to high bits.
         *   31                                    0
         *   +-------------------------------------+
         *   |              <--- data shift ----   |
         *   +-------------------------------------+
         *
         *   ex) rx data len = 1
         *   byte addr : 3   2   1  0
         *                          0x12
         *
         *   rx data len = 4
         *   byte addr : 3    2    1    0
         *               0x12 0x34 0x56 0x78
         *
         */
        uint32_t lsb = 0;
        uint32_t msb = 0;
        uint32_t i = 0;
        uint32_t rx_len;

        msb = gbus_read_reg32( ts->regs + REG_SPI_DR_MSB );
        lsb = gbus_read_reg32( ts->regs + REG_SPI_DR_LSB );

        rx_len = ts->tot_len - ts->qed_len;

        debug("rx <== lsb: 0x%x, msb: 0x%x, rx len: %d\n", lsb, msb, rx_len );

        if ( rx_len > 4 ) {
            /* adjust msb for 4 byte alignment */
            msb = msb << ((4 - (rx_len & 0x3)) * 8);
            debug(" adj msb: 0x%08x\n", msb );

            for( i = 0; i < rx_len-4; i++ ) {
                *(data_in+i) = (msb >> (32-(i+1)*8)) & 0xff;
            }
            for( i = 0; i < 4; i++ ) {
                *(data_in + (rx_len-4) + i) = (lsb >> (32-(i+1)*8)) & 0xff;
            }
        }
        else {
            lsb = lsb << ((4 - (rx_len & 0x3)) * 8);
            debug(" adj lsb: 0x%08x\n", lsb );
            for( i = 0; i < rx_len; i++ ) {
                *(data_in + i) = (lsb >> (32-(i+1)*8)) & 0xff;
            }
        }
    }
}

/*
 * flags : comination of SPI_XFER_BEGIN, SPI_XFER_END bits
 */
int spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *data_out,
		void *data_in, unsigned long flags)
{
    struct tangox_spi_slave *ts = to_tangox_spi(slave);
    size_t data_len = bitlen / 8;
    uint32_t need_for_align = 0;

    debug( "[%s] len: %d, flags: 0x%lx\n", __FUNCTION__, data_len, flags );

#ifdef DEBUG_DUMPDATA
    if ( data_out != NULL )
        DUMP_MEM( data_out, data_len, "data_out");
#endif

    spi_setup_transfer( slave );

    switch ( flags ) {
        case (SPI_XFER_BEGIN | SPI_XFER_END):

            debug( "flag => BEGIN|END\n");

            // TODO: we may need to handle the case below
            //if ( ts->qed_len > 8 )
            //    printf( "   length > 8\n");

            ts->qed_len = data_len;
            ts->tot_len = data_len;

            if ( data_out != NULL )
                memcpy( spi_scratch_buffer, data_out, data_len );

            /* load data and fire it */
            spi_load_data( slave, (uint8_t *)data_out );
            spi_start_transfer( slave );
            spi_wait_completion( slave );

            if ( data_in != NULL ) {
                /* read rx reg or mem, can we really know the rx size???  */
                printf( "   Unexpected read data..(%s,%d)\n", __FUNCTION__, __LINE__ );
            }
            break;

        case SPI_XFER_BEGIN:

            debug( "flag => BEGIN\n");
            /* load data and fire when next SPI_XFER_END is set */
            ts->qed_len = data_len;
            ts->tot_len = data_len;

            if ( data_out != NULL ) {
                debug( "   copy %d byte to scratch\n", data_len );
                memcpy( spi_scratch_buffer, data_out, data_len );
            }
            else {
                printf("    Wrong XFER_BEGIN...(%s,%d)\n", __FUNCTION__, __LINE__ );
            }
            break;

        case SPI_XFER_END:

            debug( "flag => END\n");

            ts->tot_len += data_len;

            debug( "   tot len: %d, data_in: %p, data_out: %p\n", ts->tot_len, data_in, data_out );

            if ( data_out != NULL ) {

                //need_for_align = 4 - (ts->qed_len - ((ts->qed_len >> 2) << 2));
                if ( (ts->qed_len & 0b11) )
                    need_for_align = 4 - (ts->qed_len & 0b11);
                else
                    need_for_align = 0;

                if ( need_for_align < data_len ) {
                    debug( "   copy %d byte to scratch for align\n", need_for_align );
                    memcpy( (spi_scratch_buffer + ts->qed_len), data_out, need_for_align );
                    ts->qed_len += need_for_align;
                }
                else {
                    memcpy( (spi_scratch_buffer + ts->qed_len), data_out, data_len );
                    ts->qed_len += data_len;
                }
            }

            /* load data and fire it */
            spi_load_data( slave, (uint8_t *)data_out );
            spi_start_transfer( slave );
            spi_wait_completion( slave );

            if ( data_in != NULL ) {
                /* read rx reg or buffer */
                spi_read_data( slave, data_in );
            }
            else {
                debug("    data_in is NULL\n" );
            }
            break;

        default:
            debug( "flag => DEFAULT... flags is neither BEGIN nor END\n");
            break;
    }

    return 0;
}

int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
    return ( bus == CONFIG_TANGOX_SPI_BUS && cs == CONFIG_TANGOX_SPI_CS );
}

void spi_cs_activate(struct spi_slave *slave)
{
}

void spi_cs_deactivate(struct spi_slave *slave)
{
}
