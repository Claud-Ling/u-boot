/*****************************************
 Copyright (C) 2007
 Sigma Designs, Inc. All Rights Reserved

 *****************************************/
// to enable or disable the debug messages of this source file, put 1 or 0 below
#if 0
#define LOCALDBG ENABLE
#else
#define LOCALDBG DISABLE
#endif

#include <common.h>
#include "channel.h"
#include "ios_client.h"

#define CS_IOS_LOAD_ID 0
#define SC_IOS_LOAD_ID 1
#define CS_IOS_UNLOAD_ID 2
#define SC_IOS_UNLOAD_ID 3
#define CS_IOS_START_ID 4
#define SC_IOS_START_ID 5
#define CS_IOS_KILL_ID 6
#define SC_IOS_KILL_ID 7
#define CS_IOS_SUSPEND_ID 8
#define SC_IOS_SUSPEND_ID 9
#define CS_IOS_RESUME_ID 10
#define SC_IOS_RESUME_ID 11
#define CS_IOS_SETPRIORITY_ID 12
#define SC_IOS_SETPRIORITY_ID 13
#define CS_IOS_GETINFO_ID 14
#define SC_IOS_GETINFO_ID 15
#define CS_IOS_GETREGISTER_ID 16
#define SC_IOS_GETREGISTER_ID 17
#define CS_IOS_PS_ID 18
#define SC_IOS_PS_ID 19
#define CS_IOS_CACHEDUMP_ID 20
#define SC_IOS_CACHEDUMP_ID 21
#define CS_IOS_GETLOGFIFO_ID 22
#define SC_IOS_GETLOGFIFO_ID 23
#define CS_IOS_UB_ID 24
#define CS_IOS_VERSION_ID 25
#define SC_IOS_VERSION_ID 26
#define CS_IOS_MONITORING_STOP_ID 27
#define CS_IOS_MONITORING_START_ID 28
#define CS_IOS_MONITORING_SAMPLE_ID 29
#define SC_IOS_MONITORING_SAMPLE_ID 30
#define CS_IOS_DEBUGGER_START_ID 31
#define SC_IOS_DEBUGGER_STARTED_ID 32
#define CS_IOS_DEBUGGER_STOP_ID 33
#define CS_IOS_PAUSE_ID 34
#define CS_IOS_MEMINFO_ID 35
#define SC_IOS_MEMINFO_ID 36


//#ifdef CONFIG_TANGO3
//#define memcpy(a, b, c)      mips_memcopy_std(a, b, c)
//extern void mips_memcopy_std(int *p_src, int *p_dst, int loopCnt);
//extern void memcpy(int *p_src, int *p_dst, int loopCnt);
//#endif

//void * memcpy(void *dest, const void *src, unsigned int count);

RMstatus cs_ios_load(struct gbus *pgbus, struct channel_control *cs, RMuint32 image_key, RMuint32 iload_address, RMuint32 iload_size)
{
	struct {
		RMuint32 function_id;
		RMuint32 image_key;
		RMuint32 iload_address;
		RMuint32 iload_size;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_LOAD_ID;
	_command.image_key = image_key;
	_command.iload_address = iload_address;
	_command.iload_size = iload_size;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_ios_load_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_IOS_LOAD_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;

	return RM_OK;
}

RMstatus cs_ios_unload(struct gbus *pgbus, struct channel_control *cs, RMuint32 image_key)
{
	struct {
		RMuint32 function_id;
		RMuint32 image_key;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_UNLOAD_ID;
	_command.image_key = image_key;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_ios_unload_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_IOS_UNLOAD_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;

	return RM_OK;
}

RMstatus cs_ios_start(struct gbus *pgbus, struct channel_control *cs, RMuint32 itask_key, RMuint32 image_key, struct itask_start_param * param, RMuint32 priority, RMuint32 flags)
{
	struct {
		RMuint32 function_id;
		RMuint32 itask_key;
		RMuint32 image_key;
		struct itask_start_param param;
		RMuint32 priority;
		RMuint32 flags;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_START_ID;
	_command.itask_key = itask_key;
	_command.image_key = image_key;
#if 1
	memcpy(&_command.param, param, sizeof(struct itask_start_param));
#else
    memcpy((int*)&_command.param, (int*)param, sizeof(struct itask_start_param));
#endif
	_command.priority = priority;
	_command.flags = flags;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_ios_start_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_IOS_START_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;

	return RM_OK;
}

RMstatus cs_ios_kill(struct gbus *pgbus, struct channel_control *cs, RMuint32 itask_key)
{
	struct {
		RMuint32 function_id;
		RMuint32 itask_key;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_KILL_ID;
	_command.itask_key = itask_key;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_ios_kill_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_IOS_KILL_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;

	return RM_OK;
}

RMstatus cs_ios_suspend(struct gbus *pgbus, struct channel_control *cs, RMuint32 itask_key)
{
	struct {
		RMuint32 function_id;
		RMuint32 itask_key;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_SUSPEND_ID;
	_command.itask_key = itask_key;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_ios_suspend_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_IOS_SUSPEND_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;

	return RM_OK;
}

RMstatus cs_ios_resume(struct gbus *pgbus, struct channel_control *cs, RMuint32 itask_key)
{
	struct {
		RMuint32 function_id;
		RMuint32 itask_key;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_RESUME_ID;
	_command.itask_key = itask_key;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_ios_resume_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_IOS_RESUME_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;

	return RM_OK;
}

RMstatus cs_ios_setpriority(struct gbus *pgbus, struct channel_control *cs, RMuint32 itask_key, RMuint32 priority)
{
	struct {
		RMuint32 function_id;
		RMuint32 itask_key;
		RMuint32 priority;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_SETPRIORITY_ID;
	_command.itask_key = itask_key;
	_command.priority = priority;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_ios_setpriority_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_IOS_SETPRIORITY_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;

	return RM_OK;
}

RMstatus cs_ios_getinfo(struct gbus *pgbus, struct channel_control *cs, RMuint32 itask_key)
{
	struct {
		RMuint32 function_id;
		RMuint32 itask_key;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_GETINFO_ID;
	_command.itask_key = itask_key;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_ios_getinfo_u(struct gbus *pgbus, struct channel_control *sc, struct ios_itask_info *info)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		struct ios_itask_info info;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_IOS_GETINFO_ID)
		return RM_INVALIDMODE;
	
	*info = _command.info;

	return RM_OK;
}

RMstatus cs_ios_getregister(struct gbus *pgbus, struct channel_control *cs, RMuint32 itask_key, RMuint32 register_index)
{
	struct {
		RMuint32 function_id;
		RMuint32 itask_key;
		RMuint32 register_index;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_GETREGISTER_ID;
	_command.itask_key = itask_key;
	_command.register_index = register_index;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_ios_getregister_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc, RMuint32 *value)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
		RMuint32 value;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_IOS_GETREGISTER_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;
	*value = _command.value;

	return RM_OK;
}

RMstatus cs_ios_ps(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_PS_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_ios_ps_u(struct gbus *pgbus, struct channel_control *sc, struct ps_entry *entry)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		struct ps_entry entry;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_IOS_PS_ID)
		return RM_INVALIDMODE;
	
	*entry = _command.entry;

	return RM_OK;
}

RMstatus cs_ios_cachedump(struct gbus *pgbus, struct channel_control *cs, RMuint32 dump_ga)
{
	struct {
		RMuint32 function_id;
		RMuint32 dump_ga;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_CACHEDUMP_ID;
	_command.dump_ga = dump_ga;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_ios_cachedump_u(struct gbus *pgbus, struct channel_control *sc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_IOS_CACHEDUMP_ID)
		return RM_INVALIDMODE;
	

	return RM_OK;
}

RMstatus cs_ios_getlogfifo(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_GETLOGFIFO_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_ios_getlogfifo_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc, RMuint32 *log_ga)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
		RMuint32 log_ga;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_IOS_GETLOGFIFO_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;
	*log_ga = _command.log_ga;

	return RM_OK;
}

RMstatus cs_ios_ub(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_UB_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}

RMstatus cs_ios_version(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_VERSION_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_ios_version_u(struct gbus *pgbus, struct channel_control *sc, RMuint32 *version, RMuint32 *emhwlibVersion)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMuint32 version;
		RMuint32 emhwlibVersion;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_IOS_VERSION_ID)
		return RM_INVALIDMODE;
	
	*version = _command.version;
	*emhwlibVersion = _command.emhwlibVersion;

	return RM_OK;
}

RMstatus cs_ios_monitoring_stop(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_MONITORING_STOP_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}

RMstatus cs_ios_monitoring_start(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_MONITORING_START_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}

RMstatus cs_ios_monitoring_sample(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_MONITORING_SAMPLE_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_ios_monitoring_sample_u(struct gbus *pgbus, struct channel_control *sc, struct irq_monitoring_sample *sample)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		struct irq_monitoring_sample sample;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_IOS_MONITORING_SAMPLE_ID)
		return RM_INVALIDMODE;
	
	*sample = _command.sample;

	return RM_OK;
}

RMstatus cs_ios_debugger_start(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_DEBUGGER_START_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_ios_debugger_started_u(struct gbus *pgbus, struct channel_control *sc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_IOS_DEBUGGER_STARTED_ID)
		return RM_INVALIDMODE;
	

	return RM_OK;
}

RMstatus cs_ios_debugger_stop(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_DEBUGGER_STOP_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}

RMstatus cs_ios_pause(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_PAUSE_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}

RMstatus cs_ios_meminfo(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_IOS_MEMINFO_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_ios_meminfo_u(struct gbus *pgbus, struct channel_control *sc, int *usage, int *log2size)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		int usage;
		int log2size;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),IOS_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_IOS_MEMINFO_ID)
		return RM_INVALIDMODE;
	
	*usage = _command.usage;
	*log2size = _command.log2size;

	return RM_OK;
}



