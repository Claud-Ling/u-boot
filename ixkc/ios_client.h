/*****************************************
 Copyright (C) 2007
 Sigma Designs, Inc. All Rights Reserved

 *****************************************/
#ifndef __IOS_CLIENT_H__
#define __IOS_CLIENT_H__

//#include "../../rmchannel/include/channel.h"


/*
  Do not delete / reorder the functions. You can only put new ones at the end, 
  and to HEAD branch only, then merge elsewhere.

  The order causes a precise numbering in the client and 
  server implementation (#define PROTOCOL_FUNC_ID <>)
  It is assumed to be binary backward compatible 
  [http://bugs.soft.sdesigns.com/twiki/bin/view/Main/CodingStyle#Lifecycle].

  So the client and server code only go enriching with obsolete material 
  if you don't design carefully. Welcome to backward compatibility ;-)

  It is advised to implement in the session layer an optional version handshake, 
  if you want a reserve for a large, non backward compatible change 
  after deployment.

  em 2007nov1st
*/

#define IOS_CS_COMMAND_SIZE (4+32)
#define IOS_SC_COMMAND_SIZE (4+13*4)


/* Q&A: load */
RMstatus cs_ios_load(struct gbus *pgbus, struct channel_control *cs, RMuint32 image_key, RMuint32 iload_address, RMuint32 iload_size);
RMstatus sc_ios_load_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc);


/* Q&A: unload */
RMstatus cs_ios_unload(struct gbus *pgbus, struct channel_control *cs, RMuint32 image_key);
RMstatus sc_ios_unload_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc);


#define ITASK_START_FLAG_DEBUG (1<<0)
struct itask_start_param {
	RMuint32 a0;
	RMuint32 a1;
	RMuint32 a2;
	RMuint32 a3;
};


/* Q&A: start */
RMstatus cs_ios_start(struct gbus *pgbus, struct channel_control *cs, RMuint32 itask_key, RMuint32 image_key, struct itask_start_param * param, RMuint32 priority, RMuint32 flags);
RMstatus sc_ios_start_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc);


/* Q&A: kill */
RMstatus cs_ios_kill(struct gbus *pgbus, struct channel_control *cs, RMuint32 itask_key);
RMstatus sc_ios_kill_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc);


/* Q&A: remove the itask from the scheduler. Returns RM_ERROR if the task is already suspended*/
RMstatus cs_ios_suspend(struct gbus *pgbus, struct channel_control *cs, RMuint32 itask_key);
RMstatus sc_ios_suspend_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc);


/* Q&A: resume a suspended itask. Be carefull: only resume itasks that have been previously (and 
successfully) suspended using itask_suspend. Resuming a sleeping task or a task waiting 
on a semaphore may have unpredictable results*/
RMstatus cs_ios_resume(struct gbus *pgbus, struct channel_control *cs, RMuint32 itask_key);
RMstatus sc_ios_resume_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc);


/* Q&A: setpriority */
RMstatus cs_ios_setpriority(struct gbus *pgbus, struct channel_control *cs, RMuint32 itask_key, RMuint32 priority);
RMstatus sc_ios_setpriority_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc);



enum ios_itask_state {
	ITASK_INFO_STATE_TERMINATED,
	ITASK_INFO_STATE_READY,
	ITASK_INFO_STATE_RUNNING,
	ITASK_INFO_STATE_SLEEPING,
	ITASK_INFO_STATE_ZOMBIE,
};

struct ios_itask_info {
	int tid;
	int vmid;
	RMuint32 image_key;
	int priority;
	enum ios_itask_state state;
	RMuint32 pc;
	int ro_size;
	int rw_size;
	int stack_size;
};

/* Q&A: info */
RMstatus cs_ios_getinfo(struct gbus *pgbus, struct channel_control *cs, RMuint32 itask_key);
RMstatus sc_ios_getinfo_u(struct gbus *pgbus, struct channel_control *sc, struct ios_itask_info *info);

/* Q&A: register_index is the index of the register in the context table */
RMstatus cs_ios_getregister(struct gbus *pgbus, struct channel_control *cs, RMuint32 itask_key, RMuint32 register_index);
RMstatus sc_ios_getregister_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc, RMuint32 *value);

struct ps_entry {
	RMbool last;
	RMuint32 key;
	struct ios_itask_info info;
};

/* Q&A: ps 
   list current itasks. call cs_ios_ps until ps.last is true.
   use only with iokc to list itasks 
*/
RMstatus cs_ios_ps(struct gbus *pgbus, struct channel_control *cs);
RMstatus sc_ios_ps_u(struct gbus *pgbus, struct channel_control *sc, struct ps_entry *entry);

/* Q&A: Cache dump */
RMstatus cs_ios_cachedump(struct gbus *pgbus, struct channel_control *cs, RMuint32 dump_ga);
RMstatus sc_ios_cachedump_u(struct gbus *pgbus, struct channel_control *sc);

/* Q&A: Get the ih log fifo address.
        Returns RM_ERROR in case of a KERNELONLY build (logging goes to uart) */
RMstatus cs_ios_getlogfifo(struct gbus *pgbus, struct channel_control *cs);
RMstatus sc_ios_getlogfifo_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc, RMuint32 *log_ga);

/* drop me to ub@ipu. dev only */
RMstatus cs_ios_ub(struct gbus *pgbus, struct channel_control *cs);

#define IOS_FDEBUG      (1<<31)
#define IOS_FDEBUGGER   (1<<30)
#define IOS_FKERNELONLY (1<<29)

/* Q&A: return ios version and EMHWLIB_VERSION. Could be different from cpu emhwlib because ios will typically be flashed */
RMstatus cs_ios_version(struct gbus *pgbus, struct channel_control *cs);
RMstatus sc_ios_version_u(struct gbus *pgbus, struct channel_control *sc, RMuint32 *version, RMuint32 *emhwlibVersion);

struct irq_monitoring_sample {
	RMuint32 measure_duration;
	RMuint32 irq_count[4], irq_time[4], irq_max_time[4];
};

/* monitoring interrupts */
RMstatus cs_ios_monitoring_stop(struct gbus *pgbus, struct channel_control *cs);
RMstatus cs_ios_monitoring_start(struct gbus *pgbus, struct channel_control *cs);

RMstatus cs_ios_monitoring_sample(struct gbus *pgbus, struct channel_control *cs);
RMstatus sc_ios_monitoring_sample_u(struct gbus *pgbus, struct channel_control *sc, struct irq_monitoring_sample *sample);

/* ios Debugger: Used by gdb_proxy only */
RMstatus cs_ios_debugger_start(struct gbus *pgbus, struct channel_control *cs);
RMstatus sc_ios_debugger_started_u(struct gbus *pgbus, struct channel_control *sc);

RMstatus cs_ios_debugger_stop(struct gbus *pgbus, struct channel_control *cs);

/* Pause the IPU. Use it prior to reloading ios */
RMstatus cs_ios_pause(struct gbus *pgbus, struct channel_control *cs);

/* Q&A: Meminfo (ios version >= 1.10) */
RMstatus cs_ios_meminfo(struct gbus *pgbus, struct channel_control *cs);
RMstatus sc_ios_meminfo_u(struct gbus *pgbus, struct channel_control *sc, int *usage, int *log2size);


#endif //__IOS_CLIENT_H__

