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
#include "xos2k_client.h"

#define CS_XOS2K_HELLO_ID 0
#define SC_XOS2K_HELLO_ID 1
#define CS_XOS2K_XBIND_ID 2
#define SC_XOS2K_XBIND_ID 3
#define CS_XOS2K_RANDOM_ID 4
#define SC_XOS2K_RANDOM_ID 5
#define CS_XOS2K_PBUSOPEN_ID 6
#define SC_XOS2K_PBUSOPEN_ID 7
#define CS_XOS2K_FORMATALL_ID 8
#define SC_XOS2K_FORMATALL_ID 9
#define CS_XOS2K_PROTREGS_ID 10
#define SC_XOS2K_PROTREGS_ID 11
#define CS_XOS2K_XPALLOC_ID 12
#define SC_XOS2K_XPALLOC_ID 13
#define CS_XOS2K_XPFREE_ID 14
#define SC_XOS2K_XPFREE_ID 15
#define CS_XOS2K_XLOAD_ID 16
#define SC_XOS2K_XLOAD_ID 17
#define CS_XOS2K_XUNLOAD_ID 18
#define SC_XOS2K_XUNLOAD_ID 19
#define CS_XOS2K_XSTART_ID 20
#define SC_XOS2K_XSTART_ID 21
#define CS_XOS2K_XKILL_ID 22
#define CS_XOS2K_USTART_ID 23
#define SC_XOS2K_USTART_ID 24
#define CS_XOS2K_UKILL_ID 25
#define CS_XOS2K_SHA1_NEWSTREAM_ID 26
#define SC_XOS2K_SHA1_STREAMID_ID 27
#define CS_XOS2K_SHA1_SEND_ID 28
#define CS_XOS2K_SHA1_SENDLAST_ID 29
#define SC_XOS2K_SHA1_HASH_ID 30
#define CS_XOS2K_UB_ID 31
#define CS_XOS2K_CHPLL_ID 32
#define SC_XOS2K_CHPLL_ID 33
#define CS_XOS2K_DST_ID 34
#define CS_XOS2K_XTOKEN_ID 35
#define SC_XOS2K_XTOKEN_ID 36

//#ifdef CONFIG_TANGO3
//#define memcpy      mips_memcopy_std
//extern void mips_memcopy_std(int *p_src, int *p_dst, int loopCnt);
//#endif

//void * memcpy(void *dest, const void *src, unsigned int count);

RMstatus cs_xos2k_hello(struct gbus *pgbus, struct channel_control *cs, RMuint32 protocol_version)
{
	struct {
		RMuint32 function_id;
		RMuint32 protocol_version;
	} _command;
	
	//(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_HELLO_ID;
	_command.protocol_version = protocol_version;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_xos2k_hello_u(struct gbus *pgbus, struct channel_control *sc, RMuint32 *protocol_version)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMuint32 protocol_version;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_XOS2K_HELLO_ID)
		return RM_INVALIDMODE;
	
	*protocol_version = _command.protocol_version;

	return RM_OK;
}

RMstatus cs_xos2k_xbind(struct gbus *pgbus, struct channel_control *cs, RMuint32 ga, RMuint32 size)
{
	struct {
		RMuint32 function_id;
		RMuint32 ga;
		RMuint32 size;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_XBIND_ID;
	_command.ga = ga;
	_command.size = size;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_xos2k_xbind_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_XOS2K_XBIND_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;

	return RM_OK;
}

RMstatus cs_xos2k_random(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_RANDOM_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_xos2k_random_u(struct gbus *pgbus, struct channel_control *sc, RMuint32 *x)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMuint32 x;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_XOS2K_RANDOM_ID)
		return RM_INVALIDMODE;
	
	*x = _command.x;

	return RM_OK;
}

RMstatus cs_xos2k_pbusopen(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_PBUSOPEN_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_xos2k_pbusopen_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_XOS2K_PBUSOPEN_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;

	return RM_OK;
}

RMstatus cs_xos2k_formatall(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_FORMATALL_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_xos2k_formatall_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_XOS2K_FORMATALL_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;

	return RM_OK;
}

RMstatus cs_xos2k_protregs(struct gbus *pgbus, struct channel_control *cs, RMuint32 reg_base)
{
	struct {
		RMuint32 function_id;
		RMuint32 reg_base;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_PROTREGS_ID;
	_command.reg_base = reg_base;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_xos2k_protregs_u(struct gbus *pgbus, struct channel_control *sc, struct protregs * r)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		struct protregs r;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_XOS2K_PROTREGS_ID)
		return RM_INVALIDMODE;
	
	*r = _command.r;

	return RM_OK;
}

RMstatus cs_xos2k_xpalloc(struct gbus *pgbus, struct channel_control *cs, RMuint32 size)
{
	struct {
		RMuint32 function_id;
		RMuint32 size;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_XPALLOC_ID;
	_command.size = size;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_xos2k_xpalloc_u(struct gbus *pgbus, struct channel_control *sc, RMuint32 *ga, RMuint32 *actualorder)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMuint32 ga;
		RMuint32 actualorder;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_XOS2K_XPALLOC_ID)
		return RM_INVALIDMODE;
	
	*ga = _command.ga;
	*actualorder = _command.actualorder;

	return RM_OK;
}

RMstatus cs_xos2k_xpfree(struct gbus *pgbus, struct channel_control *cs, RMuint32 ga)
{
	struct {
		RMuint32 function_id;
		RMuint32 ga;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_XPFREE_ID;
	_command.ga = ga;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_xos2k_xpfree_u(struct gbus *pgbus, struct channel_control *sc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_XOS2K_XPFREE_ID)
		return RM_INVALIDMODE;
	

	return RM_OK;
}

RMstatus cs_xos2k_xload(struct gbus *pgbus, struct channel_control *cs, RMuint32 xl_cookie, RMuint32 ga_dst, RMuint32 ga_src, RMuint32 size)
{
	struct {
		RMuint32 function_id;
		RMuint32 xl_cookie;
		RMuint32 ga_dst;
		RMuint32 ga_src;
		RMuint32 size;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_XLOAD_ID;
	_command.xl_cookie = xl_cookie;
	_command.ga_dst = ga_dst;
	_command.ga_src = ga_src;
	_command.size = size;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_xos2k_xload_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_XOS2K_XLOAD_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;

	return RM_OK;
}

RMstatus cs_xos2k_xunload(struct gbus *pgbus, struct channel_control *cs, RMuint32 xl_cookie)
{
	struct {
		RMuint32 function_id;
		RMuint32 xl_cookie;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_XUNLOAD_ID;
	_command.xl_cookie = xl_cookie;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_xos2k_xunload_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_XOS2K_XUNLOAD_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;

	return RM_OK;
}

RMstatus cs_xos2k_xstart(struct gbus *pgbus, struct channel_control *cs, RMuint32 xt_cookie, RMuint32 xl_cookie, struct xstart_param * param)
{
	struct {
		RMuint32 function_id;
		RMuint32 xt_cookie;
		RMuint32 xl_cookie;
		struct xstart_param param;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_XSTART_ID;
	_command.xt_cookie = xt_cookie;
	_command.xl_cookie = xl_cookie;
#if 0
	memcpy(&_command.param, param, sizeof(struct xstart_param));
#else
	 memcpy((int*)&_command.param, (int*)param, sizeof(struct xstart_param));
#endif

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_xos2k_xstart_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_XOS2K_XSTART_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;

	return RM_OK;
}

RMstatus cs_xos2k_xkill(struct gbus *pgbus, struct channel_control *cs, RMuint32 xt_cookie, RMuint32 what)
{
	struct {
		RMuint32 function_id;
		RMuint32 xt_cookie;
		RMuint32 what;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_XKILL_ID;
	_command.xt_cookie = xt_cookie;
	_command.what = what;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}

RMstatus cs_xos2k_ustart(struct gbus *pgbus, struct channel_control *cs, RMuint32 xl_cookie, RMuint32 dspid)
{
	struct {
		RMuint32 function_id;
		RMuint32 xl_cookie;
		RMuint32 dspid;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_USTART_ID;
	_command.xl_cookie = xl_cookie;
	_command.dspid = dspid;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_xos2k_ustart_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_XOS2K_USTART_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;

	return RM_OK;
}

RMstatus cs_xos2k_ukill(struct gbus *pgbus, struct channel_control *cs, RMuint32 dspid, RMuint32 what)
{
	struct {
		RMuint32 function_id;
		RMuint32 dspid;
		RMuint32 what;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_UKILL_ID;
	_command.dspid = dspid;
	_command.what = what;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}

RMstatus cs_xos2k_sha1_newstream(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_SHA1_NEWSTREAM_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_xos2k_sha1_streamid_u(struct gbus *pgbus, struct channel_control *sc, RMuint32 *sid)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMuint32 sid;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_XOS2K_SHA1_STREAMID_ID)
		return RM_INVALIDMODE;
	
	*sid = _command.sid;

	return RM_OK;
}

RMstatus cs_xos2k_sha1_send(struct gbus *pgbus, struct channel_control *cs, RMuint32 sid, RMuint32 ga, RMuint32 size)
{
	struct {
		RMuint32 function_id;
		RMuint32 sid;
		RMuint32 ga;
		RMuint32 size;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_SHA1_SEND_ID;
	_command.sid = sid;
	_command.ga = ga;
	_command.size = size;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}

RMstatus cs_xos2k_sha1_sendlast(struct gbus *pgbus, struct channel_control *cs, RMuint32 sid, RMuint32 ga, RMuint32 size)
{
	struct {
		RMuint32 function_id;
		RMuint32 sid;
		RMuint32 ga;
		RMuint32 size;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_SHA1_SENDLAST_ID;
	_command.sid = sid;
	_command.ga = ga;
	_command.size = size;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_xos2k_sha1_hash_u(struct gbus *pgbus, struct channel_control *sc, struct sha1digest * h)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		struct sha1digest h;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_XOS2K_SHA1_HASH_ID)
		return RM_INVALIDMODE;
	
	*h = _command.h;

	return RM_OK;
}

RMstatus cs_xos2k_ub(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_UB_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}

RMstatus cs_xos2k_chpll(struct gbus *pgbus, struct channel_control *cs, RMuint32 pll0, RMuint32 mux)
{
	struct {
		RMuint32 function_id;
		RMuint32 pll0;
		RMuint32 mux;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_CHPLL_ID;
	_command.pll0 = pll0;
	_command.mux = mux;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_xos2k_chpll_u(struct gbus *pgbus, struct channel_control *sc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_XOS2K_CHPLL_ID)
		return RM_INVALIDMODE;
	

	return RM_OK;
}

RMstatus cs_xos2k_dst(struct gbus *pgbus, struct channel_control *cs)
{
	struct {
		RMuint32 function_id;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_DST_ID;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}

RMstatus cs_xos2k_xtoken(struct gbus *pgbus, struct channel_control *cs, RMuint32 type, RMuint32 ga, RMuint32 size)
{
	struct {
		RMuint32 function_id;
		RMuint32 type;
		RMuint32 ga;
		RMuint32 size;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_CS_COMMAND_SIZE,seed);

	_command.function_id = CS_XOS2K_XTOKEN_ID;
	_command.type = type;
	_command.ga = ga;
	_command.size = size;

	
	return channel_send_command(pgbus, cs, (RMuint32*)&_command, sizeof(_command));
}


RMstatus sc_xos2k_xtoken_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc)
{
	RMstatus _status;
	struct {
		RMuint32 function_id;
		RMstatus rc;
	} _command;
	
	//RMleftMustBeSmaller(sizeof(_command),XOS2K_SC_COMMAND_SIZE,seed);
	
	_status = channel_get_command(pgbus, sc, (RMuint32*)&_command, sizeof(_command));

	if (_status != RM_OK)
		return _status;

	if (_command.function_id != SC_XOS2K_XTOKEN_ID)
		return RM_INVALIDMODE;
	
	*rc = _command.rc;

	return RM_OK;
}



