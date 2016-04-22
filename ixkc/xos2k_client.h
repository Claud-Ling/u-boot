/*****************************************
 Copyright (C) 2007
 Sigma Designs, Inc. All Rights Reserved

 *****************************************/
#ifndef __XOS2K_CLIENT_H__
#define __XOS2K_CLIENT_H__


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
  after deployment. This is done as cs/sc_xos2k_hello() below.

  em 2007nov1st
*/

/*
  This file specifies how to interact with xos2 kernel on tango3.
  
  The prior way (xos on tango2) was remote procedure calls; 
  and suffered the following limitations:
  - almost mandatorily synchronous
  - forced use of a specific interrupt line
  - need for external allocation even for small things
  - by hand maintenance of the marshalling and unmarshalling code
  - polymorphic reuse of
	// parameters (input and output)
	RMuint32 param0;
	RMuint32 param1;
	RMuint32 param2;
	RMuint32 param3;
	RMuint32 param4;
  to carry various material

  ***

  We hope this new interface is clearer and more easily expandable.

  There are three types of requests:
  - client-to-server, acknowledge-less: you push it and kernel will process fifo.
  (for instance: reboot, kill).
  - Q&A (question-and-answer): once the cs-direction command is accepted, client cannot input
  another one of that type. It has to do something else (asynchronous) or 
  wait for the sc-direction answer (synchronous).
  - numbered stream of packets, see hardware SHA-1

  Points to clarify:
  - interspersing SHA-1 with the rest must be clarified em 
 */

#define XOS2K_PROTOCOL_VERSION 19

#define XOS2K_CS_COMMAND_SIZE 32
#define XOS2K_SC_COMMAND_SIZE 64

struct sha1digest {
	RMuint8 h[20];
};

struct protregs {
 	RMuint32 mem_ac[6];
 	RMuint32 reg_ac[5];
};
                  
/*********************************************************************************/
/* maintenance things                                                            */
/*********************************************************************************/

/* Q&A: if they don't agree, client and server have been built with 
   different XOS2K_PROTOCOL_VERSION */
RMstatus cs_xos2k_hello(struct gbus *pgbus, struct channel_control *cs, RMuint32 protocol_version);
RMstatus sc_xos2k_hello_u(struct gbus *pgbus, struct channel_control *sc, RMuint32 *protocol_version);

/* Q&A: xbind */
RMstatus cs_xos2k_xbind(struct gbus *pgbus, struct channel_control *cs, RMuint32 ga, RMuint32 size);
RMstatus sc_xos2k_xbind_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc);

/* hardware random --- give me 32bit of random */
RMstatus cs_xos2k_random(struct gbus *pgbus, struct channel_control *cs);
RMstatus sc_xos2k_random_u(struct gbus *pgbus, struct channel_control *sc, RMuint32 *x);

/* Q&A: used to be -P, -F */
RMstatus cs_xos2k_pbusopen(struct gbus *pgbus, struct channel_control *cs);
RMstatus sc_xos2k_pbusopen_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc);
RMstatus cs_xos2k_formatall(struct gbus *pgbus, struct channel_control *cs);
RMstatus sc_xos2k_formatall_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc);

/* Q&A: reg_base: 0x1 to 0xf */
RMstatus cs_xos2k_protregs(struct gbus *pgbus, struct channel_control *cs, RMuint32 reg_base);
RMstatus sc_xos2k_protregs_u(struct gbus *pgbus, struct channel_control *sc, struct protregs * r);

/* Q&A: allocate,free in xos public area 
   memory is public, but allocation structures are held by the kernel
 */
RMstatus cs_xos2k_xpalloc(struct gbus *pgbus, struct channel_control *cs, RMuint32 size);
RMstatus sc_xos2k_xpalloc_u(struct gbus *pgbus, struct channel_control *sc, RMuint32 *ga, RMuint32 *actualorder);
RMstatus cs_xos2k_xpfree(struct gbus *pgbus, struct channel_control *cs, RMuint32 ga);
RMstatus sc_xos2k_xpfree_u(struct gbus *pgbus, struct channel_control *sc);

/*********************************************************************************/
/* xtasks                                                                        */
/*********************************************************************************/

/* Q&A: xload. A lot more meaning in rc compared to 863x.
   For most scenarii, ga_dst is ignored (excepted zboot).
 */
RMstatus cs_xos2k_xload(struct gbus *pgbus, struct channel_control *cs, RMuint32 xl_cookie, RMuint32 ga_dst, RMuint32 ga_src, RMuint32 size);
RMstatus sc_xos2k_xload_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc);

/* Q&A: xunload */
RMstatus cs_xos2k_xunload(struct gbus *pgbus, struct channel_control *cs, RMuint32 xl_cookie);
RMstatus sc_xos2k_xunload_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc);

/* Q&A: xstart */
struct xstart_param {
	RMuint32 param[4];
};
RMstatus cs_xos2k_xstart(struct gbus *pgbus, struct channel_control *cs, RMuint32 xt_cookie, RMuint32 xl_cookie, struct xstart_param * param);
RMstatus sc_xos2k_xstart_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc);
/* Q: xkill (targeted to a task thread) */
RMstatus cs_xos2k_xkill(struct gbus *pgbus, struct channel_control *cs, RMuint32 xt_cookie, RMuint32 what);

/* Q&A: ustart */
// iptv:      0(v)        1(a)        2(d0) 3(d1) 
// t3shuttle: 0(v0) 1(v1) 2(a0) 3(a1) 4(d0) 5(d1) 
RMstatus cs_xos2k_ustart(struct gbus *pgbus, struct channel_control *cs, RMuint32 xl_cookie, RMuint32 dspid);
RMstatus sc_xos2k_ustart_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc);
/* Q: ukill */
RMstatus cs_xos2k_ukill(struct gbus *pgbus, struct channel_control *cs, RMuint32 dspid, RMuint32 what);

/*********************************************************************************/
/* crypto helpers --- the kernel can help you a bit with crypto without an xtask */
/* if you need heavy duty --- write an xtask                                     */
/*********************************************************************************/

/* hardware SHA-1 (90MByte/s) --- open a stream id */
RMstatus cs_xos2k_sha1_newstream(struct gbus *pgbus, struct channel_control *cs);
RMstatus sc_xos2k_sha1_streamid_u(struct gbus *pgbus, struct channel_control *sc, RMuint32 *sid);
/* send the packet addresses+size; sendlast causes completion; then server sends you the hash */
RMstatus cs_xos2k_sha1_send(struct gbus *pgbus, struct channel_control *cs, RMuint32 sid, RMuint32 ga, RMuint32 size);
RMstatus cs_xos2k_sha1_sendlast(struct gbus *pgbus, struct channel_control *cs, RMuint32 sid, RMuint32 ga, RMuint32 size);
RMstatus sc_xos2k_sha1_hash_u(struct gbus *pgbus, struct channel_control *sc, struct sha1digest * h);

/* drop me to ub@xpu. dev only */
RMstatus cs_xos2k_ub(struct gbus *pgbus, struct channel_control *cs);

RMstatus cs_xos2k_chpll(struct gbus *pgbus, struct channel_control *cs, RMuint32 pll0, RMuint32 mux);
RMstatus sc_xos2k_chpll_u(struct gbus *pgbus, struct channel_control *sc);

/* dump the static tables of xos2 (at the time of call) to lrro (image table xtask table etc) */
RMstatus cs_xos2k_dst(struct gbus *pgbus, struct channel_control *cs);

/* Load a token: oemtoken or iptoken */
/* The supported token types are dependant upon the server implementation */
RMstatus cs_xos2k_xtoken(struct gbus *pgbus, struct channel_control *cs, RMuint32 type, RMuint32 ga, RMuint32 size);
RMstatus sc_xos2k_xtoken_u(struct gbus *pgbus, struct channel_control *sc, RMstatus *rc);


#endif //__XOS2K_CLIENT_H__

