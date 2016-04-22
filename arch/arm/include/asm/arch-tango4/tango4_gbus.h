
/*********************************************************************
 Copyright (C) 2001-2007
 Sigma Designs, Inc. 

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 as
 published by the Free Software Foundation.
 *********************************************************************/

#ifndef __TANGO4_GBUS_H
#define __TANGO4_GBUS_H

#ifndef __ASSEMBLY__

#include "rmdefs.h"
#include <asm/io.h>

struct gbus;
#define pGBus ((struct gbus *)1)
#define pgbus ((struct gbus *)0)

#define sys_sync() __iowmb()
#define __sync() sys_sync()

static inline RMuint32 gbus_read_uint32(struct gbus *h, RMuint32 address)
{
    return *((volatile RMuint32 *)address);
}

static inline RMuint16 gbus_read_uint16(struct gbus *h, RMuint32 address)
{
	return *((volatile RMuint16 *)address);
}
 
static inline RMuint8 gbus_read_uint8(struct gbus *h, RMuint32 address)
{
	return *((volatile RMuint8 *)address);
}
 
static inline void gbus_write_uint32(struct gbus *h, RMuint32 address, RMuint32 data)
{

	*((volatile RMuint32 *) address) = data;
	__sync();
}
 
static inline void gbus_write_uint16(struct gbus *h, RMuint32 address, RMuint16 data)
{
    *((volatile RMuint16 *)address) = data;
	__sync();
}
 
static inline void gbus_write_uint8(struct gbus *h, RMuint32 address, RMuint8 data)
{
	*((volatile RMuint8 *)address) = data;
	__sync();
}

#define gbus_read_reg32(r)      __raw_readl((volatile void __iomem *)r)
#define gbus_read_reg16(r)      __raw_readw((volatile void __iomem *)r)
#define gbus_read_reg8(r)       __raw_readb((volatile void __iomem *)r)
#define gbus_write_reg32(r, v)  __raw_writel(v, (volatile void __iomem *)r)
#define gbus_write_reg16(r, v)  __raw_writew(v, (volatile void __iomem *)r)
#define gbus_write_reg8(r, v)   __raw_writeb(v, (volatile void __iomem *)r)


/**
    gbus access macro
*/
#define RD_HOST_REG32(r)    \
        gbus_read_reg32(REG_BASE_host_interface + (r))

#define WR_HOST_REG32(r, v) \
        gbus_write_reg32(REG_BASE_host_interface + (r), (v))

#define RD_HOST_REG16(r)    \
        gbus_read_reg16(REG_BASE_host_interface + (r))

#define WR_HOST_REG16(r, v) \
        gbus_write_reg16(REG_BASE_host_interface + (r), (v))

#define RD_HOST_REG8(r) \
        gbus_read_reg8(REG_BASE_host_interface + (r))

#define WR_HOST_REG8(r, v)  \
        gbus_write_reg8(REG_BASE_host_interface + (r), (v))


#endif /* !__ASSEMBLY__ */
#endif /* __TANGO4_GBUS_H */

