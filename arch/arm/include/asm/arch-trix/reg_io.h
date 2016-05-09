
#ifndef __MONZA_IO__
#define __MONZA_IO__

#include <asm/io.h>

#define SIGMA_IO_ADDR(a) (((a) & 0xfffffff) | 0xf0000000)	//to SX6 IO addr

static inline  unsigned char ReadRegByte(unsigned int addr)
        {return readb(SIGMA_IO_ADDR(addr));}

static inline  unsigned short ReadRegHWord(unsigned int addr)
        {return readw(SIGMA_IO_ADDR(addr));}

static inline  unsigned int ReadRegWord(unsigned int addr)
        {return readl(SIGMA_IO_ADDR(addr));}

static inline  void  WriteRegByte(unsigned int addr, unsigned char val)
        {writeb(val,SIGMA_IO_ADDR(addr));}

static inline  void  WriteRegHWord(unsigned int addr, unsigned short val)
        {writew(val,SIGMA_IO_ADDR(addr));}

static inline  void  WriteRegWord(unsigned int addr, unsigned int val)
        {writel(val,SIGMA_IO_ADDR(addr));}

#define MWriteReg(op,a,v,m) do{			\
		typeof(v) _temp =ReadReg##op(a);\
		_temp &= ~(m);			\
		_temp |= ((v) & (m));		\
		WriteReg##op(a, _temp);		\
	}while(0)

static inline void MWriteRegByte(unsigned int addr, unsigned char val, unsigned char mask)
{
	MWriteReg(Byte, addr, val, mask);
}

static inline void MWriteRegHWord(unsigned int addr, unsigned short val, unsigned short mask)
{
	MWriteReg(HWord, addr, val, mask);
}

static inline void MWriteRegWord(unsigned int addr, unsigned int val, unsigned int mask)
{
	MWriteReg(Word, addr, val, mask);
}
#endif
