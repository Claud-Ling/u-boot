/*****************************************
 Copyright (C) 2007
 Sigma Designs, Inc. All Rights Reserved

 *****************************************/
#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#define ALLOW_OS_CODE 1

#include <asm/arch/tango4.h>
#undef pgbus
#include <asm/arch/rmdbg.h>
#include "gbus_fifo.h"

#define CHANNEL_COUNT 		48
#define CHANNEL_API_VERSION	2

//#if (EM86XX_CHIP == EM86XX_CHIPID_TANGO2)
//#define LR_CHANNEL_INDEX        LR_GNET_MAC
//#endif

enum channel_status {
	CHANNEL_STATUS_CLEAR,
	CHANNEL_STATUS_DIRTY, /* someone wrote a command */
};

enum channel_processor_id {
	CHANNEL_PROCESSOR_CPU,
	CHANNEL_PROCESSOR_XPU,
	CHANNEL_PROCESSOR_IPU,
	CHANNEL_PROCESSOR_HOST, // withhost mode
	CHANNEL_PROCESSOR_INVALID,
};

/*
  we don't set that structure as opaque, but it makes no sense to use fields directly.

  Exception: if you write a server that cares about interrupts / clearing, you need
  to test status==dirty and write status=clear. Just proceed.

  e.m. 2007nov9th
 */
struct channel_control {
	struct gbus_fifo command_fifo;
	RMuint32 version;
	enum channel_status status; 
	RMuint32 command_size;
	enum channel_processor_id listener;
};

/*
  *** Binary backward compatibility ***
  *** Binary backward compatibility ***
  *** Binary backward compatibility ***

  This table is additive only on HEAD. All branches should mirror from HEAD.
  Do not reorder, do not remove entries.
 
  Authoritative location is set_rua_test/rmchannel/include/channel.h

  em 2007nov3rd
 */
#define CHANNEL_ZLIB_C2I	2
#define CHANNEL_ZLIB_I2C	3
#define CHANNEL_IOS_API_C2I	4
#define CHANNEL_IOS_API_I2C	5
#define CHANNEL_IOS_DEBUG_C2I	6
#define CHANNEL_IOS_DEBUG_I2C	7
#define CHANNEL_XOS2_API_C2X	8
#define CHANNEL_XOS2_API_X2C	9
#define CHANNEL_XOS2_API_I2X	10
#define CHANNEL_XOS2_API_X2I	11

#define CHANNEL_USR0_API_C2X    12
#define CHANNEL_USR0_API_X2C    13
#define CHANNEL_USR1_API_C2X    14
#define CHANNEL_USR1_API_X2C    15
#define CHANNEL_USR2_API_C2X    16
#define CHANNEL_USR2_API_X2C    17

#define CHANNEL_USR3_API_C2I    18
#define CHANNEL_USR3_API_I2C    19
#define CHANNEL_USR4_API_C2I    20
#define CHANNEL_USR4_API_I2C    21
#define CHANNEL_USR5_API_C2I    22
#define CHANNEL_USR5_API_I2C    23

#define CHANNEL_USR6_API_I2X    24
#define CHANNEL_USR6_API_X2I    25
#define CHANNEL_USR7_API_I2X    26
#define CHANNEL_USR7_API_X2I    27
#define CHANNEL_USR8_API_I2X    28
#define CHANNEL_USR8_API_X2I    29

/*
  this one is called once by the system entity that boots first (xpu).
  you would typically never call it a second time. It creates the channel index table.

  It is relied upon that LR_CHANNEL_INDEX==0 means uninitialized index table (early xos2 sets to zero),
  and non zero means ready.
 */
void channel_api_initialize (struct gbus *pgbus, RMuint32 channel_index_addr);

RMuint32 channel_mem_size(RMuint32 command_count, RMuint32 command_size);

/**
   tbd

   @param pgbus 
   @param command_count 
   @param max_command_size      
   @param control_addr  
   @param channel_id    
   @param listener_id	
*/
struct channel_control *channel_create(struct gbus *pgbus, 
                                       RMuint32 command_count,
                                       RMuint32 max_command_size, 
                                       RMuint32 control_addr,
                                       enum channel_processor_id listener_id);


RMstatus channel_get_command(struct gbus *pgbus, struct channel_control *control, RMuint32 *command, RMuint32 command_size);

RMstatus channel_send_command(struct gbus *pgbus, struct channel_control *control, RMuint32 *command, RMuint32 command_size);


RMuint32 channel_get_writable_count(struct gbus *pgbus, struct channel_control *control);

RMuint32 channel_get_readable_count(struct gbus *pgbus, struct channel_control *control);



RMstatus channel_register(struct gbus *pgbus, RMuint32 channel_id, struct channel_control *c);

struct channel_control *channel_lookup(struct gbus* pgbus, RMuint32 channel_id);

RMstatus channel_unregister(struct gbus *pgbus, RMuint32 channel_id);






#endif //__CHANNEL_H__

