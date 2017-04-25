/**
  @file   tee_service.h
  @brief
	This file decribes sigma designs defined tee services

  @author Tony He, tony_he@sigmadesigns.com
  @date   2017-04-25
  */

#ifndef __TEE_SERVICE_H__
#define __TEE_SERVICE_H__

#define EXEC_STATE_NORMAL	0
#define EXEC_STATE_SECURE	(!EXEC_STATE_NORMAL)

/*
 * TEE services call error code
 */
enum{
	TEE_SVC_E_OK = 0,
	TEE_SVC_E_INVAL = -1,	/* invalid parameter */
	TEE_SVC_E_NOT_SUPPORT = -2,
	TEE_SVC_E_INVALID_RANGE = -3,
	TEE_SVC_E_PERMISSION_DENY = -4,
	TEE_SVC_E_LOCK_FAIL = -5,
	TEE_SVC_E_SMALL_BUFFER = -6,
	TEE_SVC_E_ERROR = -7,
};

#ifndef __ASSEMBLY__

struct tee_operations {
	const char* name;
	int (*probe)(void);
	int (*get_secure_state)(void);
	int (*set_mem_protection)(unsigned long tva, unsigned long sz);
	int (*set_l2x_reg)(unsigned long ofs, unsigned long val);
	int (*secure_mmio)(unsigned long op, unsigned long pa, unsigned long a2, unsigned long a3, unsigned int wnr);
	int (*fuse_read)(unsigned long ofs, unsigned long va, unsigned long len, unsigned int *pprot);
	int (*get_rsa_key)(unsigned long va, unsigned long len);
};

extern struct tee_operations tee_ops;

#define TEE_OPS(tag, probe_fn, secure_state_fn,	\
		set_pst_fn, set_l2x_fn,		\
		mmio_fn, fusread_fn,		\
		rsa_key_fn)			\
struct tee_operations tee_ops = {		\
	.name = tag,				\
	.probe = probe_fn,			\
	.get_secure_state = secure_state_fn,	\
	.set_mem_protection = set_pst_fn,	\
	.set_l2x_reg = set_l2x_fn,		\
	.secure_mmio = mmio_fn,			\
	.fuse_read = fusread_fn,		\
	.get_rsa_key = rsa_key_fn,		\
};

#endif /* !__ASSEMBLY__ */
#endif /* __TEE_SERVICE_H__ */
