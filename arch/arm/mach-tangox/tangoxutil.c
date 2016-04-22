
#include <common.h>
#include <linux/compiler.h>

#include <asm/io.h>
#include <asm/arch/tango4_gbus.h>
#include <asm/arch/emhwlib_registers_tango4.h>
#include <asm/arch/xenv.h>
#include <asm/arch/addrspace.h>
#include <div64.h>

/*
 * we will restore remap registers before rebooting
 */
#ifdef CONFIG_TANGO2
unsigned long em8xxx_remap_registers[5];
#elif defined(CONFIG_TANGO3) || defined(CONFIG_TANGO4)
unsigned long em8xxx_remap_registers[9];
unsigned long tangox_zxenv[MAX_XENV_SIZE/sizeof(unsigned long)] = { 0 };
#endif 

// TODO: this function should be reviewed and re-implemented for ARM arch later
unsigned long tangox_dma_address(unsigned long physaddr)
{

#if defined (CONFIG_MIPS)    
	unsigned long mask_64M, offset, remap_index, remap_addr;

	mask_64M = physaddr & ~(0x04000000-1);
	offset   = physaddr & (0x04000000-1);
	
	if (mask_64M == 0 || mask_64M == 7)
	    return physaddr;
	
	remap_index = (mask_64M/0x04000000)+1;

	remap_addr = gbus_read_uint32(pgbus, REG_BASE_cpu_block + CPU_remap + remap_index*4) | offset;

	return (remap_addr);
#else
    return physaddr;
#endif    
}


unsigned long tangox_chip_id(void)
{
	unsigned long chip_id = 0;
	if (chip_id == 0)
		chip_id = ((gbus_read_reg32(REG_BASE_host_interface + PCI_REG0) & 0xffff) << 16) |
				(gbus_read_reg32(REG_BASE_host_interface + PCI_REG1) & 0xff);
	return chip_id;
}

void tangox_probe_chip_rev(unsigned long *pchip_id, unsigned long *prev_id)
{
	*pchip_id = gbus_read_uint32(pgbus, REG_BASE_host_interface + PCI_REG0) & 0xfffe;
	*prev_id  = gbus_read_uint32(pgbus, REG_BASE_host_interface + PCI_REG1) & 0xf;	
}


//unsigned long _PHYSADDR(volatile void* kseg0_virtual_addr)
unsigned long tangox_phys_addr(volatile void* virtual_addr)
{
    unsigned long net_phy_addr;
    
#if defined (CONFIG_MIPS)    
    net_phy_addr = tangox_dma_address( (unsigned long)CPHYSADDR(virtual_addr) );
    //unsigned long tangox_chip_id, tangox_rev_id;

    // we don't need to support MIPS based chips so comment out this
    // however, somthing similar may  be necessary in the future.

    /* get chip id */
    tangox_probe_chip_rev( &tangox_chip_id, &tangox_rev_id );
    
    if ((tangox_chip_id == 0x8654) || (tangox_chip_id == 0x8644)) {
        if (tangox_rev_id <= 2) {
            if (net_phy_addr >= 0xc0000000)
                return (0x20000000 + (net_phy_addr - 0xc0000000));
            else if (net_phy_addr >= 0x80000000)
                return (0x10000000 + (net_phy_addr - 0x80000000));
            else
                return net_phy_addr;
        }
        else{ // ES3 and above
            return net_phy_addr;
        }
	} //else {
    	/* applicable to other newer chips (e.g. 8652) */
#else
    net_phy_addr = (unsigned long)virtual_addr;
#endif
	return net_phy_addr;
}



// TODO: the clock related functions below should be reviewed and 
//       re-implemented suitable to ARM arch
#define TANGOX_BASE_FREQUENCY	27000000 

#define RD_BASE_REG32(r)	\
		gbus_read_reg32(REG_BASE_system_block + (r))

unsigned long tangox_get_pllclock(int pll)
{
	unsigned long sys_clkgen_pll, sysclk_mux;
	unsigned long n, m, freq, k, step;

    if(((tangox_chip_id()>>16)&0xffff) != 0x8734) {
        sysclk_mux = RD_BASE_REG32(SYS_sysclk_mux) & 0xf01;
        if ((sysclk_mux & 0x1) == 0) {
            freq = TANGOX_BASE_FREQUENCY;
            goto done;
        }
    }
    
	sys_clkgen_pll = RD_BASE_REG32(SYS_clkgen0_pll + (pll * 0x8));

	/* Not using XTAL_IN, cannot calculate */
	if ((sys_clkgen_pll & 0x07000000) != 0x01000000)
		goto freq_error;

#ifdef CONFIG_TANGO2
	m = (sys_clkgen_pll >> 16) & 0x1f;
	n = sys_clkgen_pll & 0x000003ff;
	k = (pll) ? 0 : ((sys_clkgen_pll >> 14) & 0x3); 
	step = 2;
#elif defined(CONFIG_TANGO3) || defined(CONFIG_TANGO4)
	if (pll != 0) { /* PLL1/PLL2 */
 		unsigned int chip_id = (tangox_chip_id() >> 16) & 0xfffe;
		if ((chip_id == 0x8646) || ((chip_id & 0xfff0) == 0x8670) || 
           ((chip_id & 0xff00) == 0x8900) || ((chip_id & 0xffff) == 0x8734))
			m = 0;
		else
			m = (sys_clkgen_pll >> 16) & 0x1;
		n = sys_clkgen_pll & 0x0000007f;
		k = (sys_clkgen_pll >> 13) & 0x7;
		step = 1;
	} else {
		m = (sys_clkgen_pll >> 16) & 0x1f;
		n = sys_clkgen_pll & 0x000003ff;
		k = (sys_clkgen_pll >> 14) & 0x3; 
		step = 2;
	}
#else
#error Unsupported platform.
#endif
	freq = ((TANGOX_BASE_FREQUENCY / (m + step)) * (n + step)) / (1 << k);

done:
	return(freq);

freq_error:
	printf("%s:%d don't know how to calculate the frequency ..\n", __FILE__, __LINE__);
	//BUG();
	return(0);
}

unsigned long tangox_get_clock(unsigned int clk_dom)
{

    unsigned long sysclk_mux, sysclk_premux;
	unsigned long div, mux, pll, pll_freq;
	static const unsigned char dividers[3][12] = {
		{ 2, 4, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4 },
		{ 2, 2, 2, 3, 3, 2, 3, 2, 4, 2, 4, 2 }, 
#ifdef CONFIG_TANGO2
		{ 2, 4, 3, 3, 3, 3, 2, 2, 4, 4, 2, 2 },
#else
		{ 2, 4, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4 },
#endif
	};

	sysclk_mux = RD_BASE_REG32(SYS_sysclk_mux) & 0xf01;
	sysclk_premux = RD_BASE_REG32(SYS_sysclk_premux);
	pll = sysclk_premux & 0x3;

	if (((pll_freq = tangox_get_pllclock(pll)) == 0) || (clk_dom >= 3))
		goto freq_error;
	else if ((mux = ((sysclk_mux >> 8) & 0xf)) >= 12)
		goto freq_error; /* invalid mux value */

	div = (unsigned long)dividers[clk_dom][mux];

	return(pll_freq / div);

freq_error:
	return(0);

}

unsigned long tangox_get_cdclock(unsigned int cd)
{
	unsigned long sysclk_premux = (RD_BASE_REG32(SYS_sysclk_premux) & 0x700) >> 8;
	unsigned long cddiv = RD_BASE_REG32(SYS_cleandiv0_div + (cd * 8));
	u64 pllfreq, temp;

    pllfreq = (u64)tangox_get_pllclock(sysclk_premux >> 1);

	if (sysclk_premux & 1)  /* from PLLx_1 */
		do_div(pllfreq, (unsigned int)(RD_BASE_REG32(SYS_clkgen0_div + ((sysclk_premux >> 1) * 8)) & 0xf));

	/* use 64 bit for better precision, from formula
	   cd_out = pllfreq / (2 + cddiv * (2 ^ -27)) 
	   => cd_out = pllfreq / (2 + (cddiv >> 27)) 
	   => cd_out = pllfreq << 27 / (2 + (cddiv >> 27)) <<27
	   => cd_out = pllfreq << 27 / (2 << 27 + cddiv) 
	   => cd_out = pllfreq << 27 / (1 << 28 + cddiv) */
	temp = pllfreq << 27;
	do_div(temp, (unsigned int)((1 << 28) + cddiv));

	if (RD_BASE_REG32(SYS_cleandiv0_div + 4 + (cd * 8)) & 1)
		return 0;
	else
		return (unsigned long)(temp & 0xffffffff);
}


unsigned long tangox_get_sysclock(void)
{
	return(tangox_get_clock(0));
}

unsigned long tangox_get_cpuclock(void)
{
	if(((tangox_chip_id()>>16)&0xfffe) != 0x8734)
		return tangox_get_clock(1);
	else {
        int div_ctrl = 0;
		/* get sys_cpuclk_div_ctrl */
		div_ctrl = RD_BASE_REG32(SYS_cpuclk_div_ctrl);
		if(div_ctrl & (1<<23)){
			/*by pass clk*/
			return tangox_get_clock(1);
		} else{
			/* get pll0_0 clock which feeded into  cpu clk dividder*/
			return  tangox_get_pllclock(0);
		}
    }
}

// -------------------------------------------------------------------
// TODO: the functions below also need to be examined and re-implemented
//       for ARM arch.
#define PA_TO_KVA1(pa)	((pa) | 0xa0000000)

RMuint32 sigmaremap(RMuint32 address)
{
	RMuint32 address31_16=address&(~((1<<16)-1)),
		address15_0=address&((1<<16)-1),
		address31_26,address25_0;
	
#ifdef __XOS_H__
#define RB REG_BASE_xpu_block
#else
#define RB REG_BASE_cpu_block
#endif

	if (address31_16==*(volatile RMuint32 *)PA_TO_KVA1(RB+CPU_remap1)) 
        return CPU_remap1_address|address15_0;

	address31_26 = address & (~((1<<26)-1));
	address25_0  = address & ((1<<26)-1);

	if (address31_26==*(volatile RMuint32 *)PA_TO_KVA1(RB+CPU_remap2)) 
        return CPU_remap2_address|address25_0;
	
    if (address31_26==*(volatile RMuint32 *)PA_TO_KVA1(RB+CPU_remap3)) 
        return CPU_remap3_address|address25_0;
	
    if (address31_26==*(volatile RMuint32 *)PA_TO_KVA1(RB+CPU_remap4)) 
        return CPU_remap4_address|address25_0;

#if defined (CONFIG_TANGO3) || defined (CONFIG_TANGO4)
	if (address31_26==*(volatile RMuint32 *)PA_TO_KVA1(RB+CPU_remap5)) 
        return CPU_remap5_address|address25_0;
	
    if (address31_26==*(volatile RMuint32 *)PA_TO_KVA1(RB+CPU_remap6)) 
        return CPU_remap6_address|address25_0;
	
    if (address31_26==*(volatile RMuint32 *)PA_TO_KVA1(RB+CPU_remap7)) 
        return CPU_remap7_address|address25_0;
#endif // CONFIG_TANGO3

#undef RB

	return -1;
}

// Inverted physical address mapping
unsigned long tangox_inv_dma_address(unsigned long gbusaddr)
{
#if defined (CONFIG_MIPS)        
    unsigned long paddr = sigmaremap( gbusaddr );
    
    return KSEG0ADDR(paddr);
#else
    return gbusaddr;
#endif    
}

