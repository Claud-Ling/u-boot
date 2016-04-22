#ifndef __TANGOX_SPI_H__
#define __TANGOX_SPI_H__

/* base : 0x6c400 */
#define REG_SPI_CTRL0		    0x000
#define   CSB_MASK              (0x7)
#define   CTRL0_CSB(x)          (((x) & 0x7))
#define   CS_LEN_MASK           (0x3f << 3)
#define   CTRL0_CS_LEN(x)       (((x) & 0x3f) << 3)
#define   CMODE_MASK            (0x3 << 9)
#define   CTRL0_CMODE(x)        (((x) & 0x3)  << 9)
#define   EXTMODE_MASK          (0x3 << 11)
#define   CTRL0_EXTMODE(x)      (((x) & 0x3)  << 11)
#define   CTRL0_DUAL_XFER(x)    (((x) & 0x1)  << 13)
#define   CTRL0_WI(x)           (((x) & 0x1)  << 14)
#define   CTRL0_RI(x)           (((x) & 0x1)  << 15)
#define   HALF_SCLK_MASK        (0xfff << 16)
#define   CTRL0_HALF_SCLK(x)    (((x) & 0xfff)<< 16)
#define   CTRL0_INT(x)          (((x) & 0x1)  << 28)
#define   CTRL0_EN(x)           (((x) & 0x1)  << 29)
#define   CTRL0_ENDIAN(x)       (((x) & 0x1)  << 30)
#define   CTRL0_ENDIAN_MASK     (0x1 << 30)
#define   CTRL0_START(x)        (((x) & 0x1)  << 31)
#define REG_SPI_CTRL1           0x004
#define   BURST_LEN_MASK        (0x3ff)
#define   CTRL1_BURST_LEN(x)    (((x) & 0x3ff))
#define   CTRL1_DL_RWN(x)       (((x) & 0x1)   << 10)
#define   CTRL1_DUAL(x)         (((x) & 0x1)   << 11)
#define   CTRL1_QUAD(x)         (((x) & 0x1)   << 12)
#define   CTRL1_HI_PERF(x)      (((x) & 0x1)   << 13)
#define   TCS_HI_MASK           (0xff << 16)
#define   CTRL1_TCS_HI(x)       (((x) & 0xff)  << 16)
#define   TCS_EN_MASK           (0xf << 24)
#define   CTRL1_TCS_EN(x)       (((x) & 0xf)   << 24)
#define   TCS_ST_MASK           (0xf << 28)
#define   CTRL1_TCS_ST(x)       (((x) & 0xf)   << 28)
#define REG_SPI_DW_LSB          0x008
#define REG_SPI_DW_MSB          0x00C
#define REG_SPI_DR_LSB          0x010
#define REG_SPI_DR_MSB          0x014
#define REG_SPI_RD_MEM_OFFSET   0xC00
#define REG_SPI_WR_MEM_OFFSET   0x1000

struct tangox_spi_slave {
	struct spi_slave slave;
    uint32_t    hz;
    uint32_t    sys_clk;
    uint32_t    cs;
    uint32_t    mode;
    uint32_t    qed_len;
    uint32_t    tot_len;
    uint8_t     *regs;

    /* spi read/write memory 1k for each */
    uint8_t    *rx_mem;
    uint8_t    *tx_mem;
};

static inline struct tangox_spi_slave *to_tangox_spi(struct spi_slave *slave)
{
	return container_of(slave, struct tangox_spi_slave, slave);
}

#endif
