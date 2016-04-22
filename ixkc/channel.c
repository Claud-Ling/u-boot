/*****************************************
 Copyright (C) 2007
 Sigma Designs, Inc. All Rights Reserved

 *****************************************/

#include <asm/arch/tango4_gbus.h>
#include <asm/arch/emhwlib_lram_tango4.h>
#include <asm/arch/emhwlib_registers_tango4.h>

#include "channel.h"

//#include "util.h"

#define LOG2_SOFT_IRQ_CHANNEL   4
#define SOFT_IRQ_CHANNEL        (1<<LOG2_SOFT_IRQ_CHANNEL)

static inline void gbus_read_data32(struct gbus *h, RMuint32 byte_address, RMuint32 *data, RMuint32 count)
{
	RMuint32 i;
	for (i = 0; i < count; i++, data++)
		*data = gbus_read_uint32(h, byte_address + (4*i));
}

static inline void gbus_write_data32(struct gbus *h, RMuint32 byte_address, const RMuint32 *data, RMuint32 count)
{
	RMuint32 i;
	for (i = 0; i < count; i++, data++)
		gbus_write_uint32(h, byte_address + (4*i), *data);
}

#if 0
#define LOCALDBG ENABLE
#else
#define LOCALDBG DISABLE
#endif

void channel_api_initialize(struct gbus *pgbus, RMuint32 channel_index_addr)
{
	RMuint32 i;
	RMuint32 *channel_index = (RMuint32*)channel_index_addr;
	
	for (i=0 ; i<CHANNEL_COUNT ; i++)
		gbus_write_uint32(pgbus, (RMuint32) &channel_index[i], 0);
	
	// LR_CHANNEL_INDEX non zero means ready
	gbus_write_uint32(pgbus, REG_BASE_cpu_block+LR_CHANNEL_INDEX, channel_index_addr);
	
	RMDBGLOG((LOCALDBG,"channel_api_initialize: Channel API initialized. Index @ 0x%08lx\n", channel_index));
}

RMuint32 channel_mem_size(RMuint32 command_count, RMuint32 command_size)
{
	return sizeof(struct channel_control)+(command_count+1)*command_size;
}

struct channel_control *channel_create(struct gbus *pgbus, 
                                       RMuint32 command_count,
                                       RMuint32 max_command_size, 
                                       RMuint32 control_addr,
                                       enum channel_processor_id listener_id)
{
	struct channel_control *control=(struct channel_control*)control_addr;
	
	if (max_command_size%4) {
		RMDBGLOG((LOCALDBG,"channel_register: Error: Command size (%d) must be 4bytes aligned\n", max_command_size));
		return NULL;
	}
	
	gbus_write_uint32(pgbus, (RMuint32) &(control->listener), listener_id);
	gbus_write_uint32(pgbus, (RMuint32) &(control->version), CHANNEL_API_VERSION);
	gbus_write_uint32(pgbus, (RMuint32) &(control->status), CHANNEL_STATUS_CLEAR);
	gbus_write_uint32(pgbus, (RMuint32) &(control->command_size), max_command_size);
	
	gbus_fifo_open(pgbus, (RMuint32) (control+1), (command_count+1)*max_command_size, 
			(RMuint32) &(control->command_fifo));
	 
	return control;
}

RMstatus channel_register(struct gbus *pgbus, RMuint32 channel_id, struct channel_control *c)
{
	RMuint32* channel_index;
	
	if (channel_id >= CHANNEL_COUNT) {
		RMDBGLOG((LOCALDBG,"channel_register: Error: %d is not a valid channel id\n", channel_id));
		return RM_ERROR;
	}
	
	channel_index = (RMuint32*)gbus_read_uint32(pgbus, REG_BASE_cpu_block+LR_CHANNEL_INDEX);
	
	if (gbus_read_uint32(pgbus, (RMuint32) &channel_index[channel_id])) {
		RMDBGLOG((LOCALDBG,"channel_register: Error: channel #%d already registered\n", channel_id));
		return RM_ERROR;
	}
	
	gbus_write_uint32(pgbus, (RMuint32) &channel_index[channel_id], (RMuint32) c);
	
	RMDBGLOG((LOCALDBG,"channel_register: channel #%d registered\n", channel_id));
	
	return RM_OK;
}

RMstatus channel_unregister(struct gbus *pgbus, RMuint32 channel_id)
{
	RMuint32* channel_index = (RMuint32*)gbus_read_uint32(pgbus, REG_BASE_cpu_block+LR_CHANNEL_INDEX);
	gbus_write_uint32(pgbus, (RMuint32) &channel_index[channel_id], 0);
	return RM_OK;
}

struct channel_control *channel_lookup(struct gbus* pgbus, RMuint32 channel_id)
{
	struct channel_control *control;
	RMuint32* channel_index;
	
	if (channel_id >= CHANNEL_COUNT) {
		RMDBGLOG((LOCALDBG,"channel_lookup: Error: %d is not a valid channel id\n", channel_id));
		return NULL;
	}
	
	channel_index = (RMuint32*)gbus_read_uint32(pgbus, REG_BASE_cpu_block+LR_CHANNEL_INDEX);
	
	control = (struct channel_control*)gbus_read_uint32(pgbus, (RMuint32) &channel_index[channel_id]);
	if (!control) {
		RMDBGLOG((DISABLE,"channel_lookup: Error: channel #%d is not opened\n", channel_id));
		return NULL;	
	}
	
	if (CHANNEL_API_VERSION != gbus_read_uint32(pgbus, (RMuint32) &(control->version))) {
		RMDBGLOG((LOCALDBG,"channel_lookup: Error: API versions do not match ctr(%p)=%p id#%d i=%p ct=%p\n", &control, control, channel_id, channel_index, gbus_read_uint32(pgbus, (RMuint32) &channel_index[channel_id])));
		return NULL;
	}
	
	return control;
}

RMstatus channel_get_command(struct gbus *pgbus, struct channel_control *control, RMuint32 *command, RMuint32 command_size)
{
	RMuint32 rd_ptr1, rd_size1, rd_ptr2, channel_command_size;

	channel_command_size = gbus_read_uint32(pgbus, (RMuint32)&(control->command_size));

	if (command_size > channel_command_size) {
		RMDBGLOG((LOCALDBG,"channel_get_command: Command size is too big\n"));
		return RM_ERROR;
	}
	
	gbus_fifo_get_readable_size(pgbus, &(control->command_fifo), &rd_ptr1, &rd_size1, &rd_ptr2);
	
	if (rd_size1 < channel_command_size)
		return RM_PENDING;
	
	gbus_read_data32(pgbus, rd_ptr1, command, command_size/sizeof(RMuint32));
	gbus_fifo_incr_read_ptr(pgbus, &(control->command_fifo), channel_command_size);

	return RM_OK;
}

RMstatus channel_send_command(struct gbus *pgbus, struct channel_control *control, RMuint32 *command, RMuint32 command_size)
{
	RMuint32 wr_ptr1, wr_size1, wr_ptr2, channel_command_size;
	enum channel_processor_id listener_processor;
	
	channel_command_size = gbus_read_uint32(pgbus, (RMuint32)&(control->command_size));
	
	if (command_size > channel_command_size) {
		RMDBGLOG((LOCALDBG,"channel_send_command: Command size is too big\n"));
		return RM_ERROR;
	}
	
	gbus_fifo_get_writable_size(pgbus, &(control->command_fifo), &wr_ptr1, &wr_size1, &wr_ptr2);
	
	if (wr_size1 < channel_command_size) 
		return RM_PENDING;
	
	gbus_write_data32(pgbus, wr_ptr1, command, command_size/sizeof(RMuint32));
	gbus_fifo_incr_write_ptr(pgbus, &(control->command_fifo), channel_command_size);
	gbus_write_uint32(pgbus, (RMuint32) &(control->status), CHANNEL_STATUS_DIRTY);
	
	listener_processor = gbus_read_uint32(pgbus, (RMuint32) &(control->listener));
	
	switch (listener_processor) {
	case CHANNEL_PROCESSOR_CPU:
		gbus_write_uint32(pgbus, REG_BASE_cpu_block+CPU_irq_softset, SOFT_IRQ_CHANNEL);
		break;
#if (EM86XX_CHIP >= EM86XX_CHIPID_TANGO2)
	case CHANNEL_PROCESSOR_XPU:
		gbus_write_uint32(pgbus, REG_BASE_xpu_block+CPU_irq_softset, SOFT_IRQ_CHANNEL);
		break;
#endif
#if (EM86XX_CHIP >= EM86XX_CHIPID_TANGO3)
#if (!defined IOS_XPU) && (!defined IOS_CPU)
	case CHANNEL_PROCESSOR_IPU:
		gbus_write_uint32(pgbus, REG_BASE_ipu_block+CPU_irq_softset, SOFT_IRQ_CHANNEL);
		break;
#endif
#endif
	default:
		break;
	}

	return RM_OK;
}

RMuint32 channel_get_writable_count(struct gbus *pgbus, struct channel_control *control)
{
	RMuint32 wr_ptr1, wr_size1, wr_ptr2, command_size, size;
	
	command_size=gbus_read_uint32(pgbus, (RMuint32) &(control->command_size));
	size=gbus_fifo_get_writable_size(pgbus, &(control->command_fifo), &wr_ptr1, &wr_size1, &wr_ptr2);

	return (size/command_size);
}

RMuint32 channel_get_readable_count(struct gbus *pgbus, struct channel_control *control)
{
	RMuint32 rd_ptr1, rd_size1, rd_ptr2, command_size, size;
	
	command_size=gbus_read_uint32(pgbus, (RMuint32)&(control->command_size));
	size=gbus_fifo_get_readable_size(pgbus, &(control->command_fifo), &rd_ptr1, &rd_size1, &rd_ptr2);
	
	return (size/command_size);
}


