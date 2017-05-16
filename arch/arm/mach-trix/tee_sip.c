/**
  @file   tee_sip.c
  @brief
	This file decribes ree glue codes for sigma designs defined sip services, which is headed for ARMv8 projects.

  @author Tony He, tony_he@sigmadesigns.com
  @date   2017-04-25
  */

#include <common.h>
#include <errno.h>
#include <memalign.h>
#include <asm/arch/setup.h>
#include <asm/arch/reg_io.h>
#include "tee_service.h"
#include "sd_sip_svc.h"

#define PHYADDR(a) ((uintptr_t)(a))	/* u-boot utilizes idmap */

#ifdef DEBUG
# define NOISE_ON_FAIL(sret) do {				\
	if ((sret) != SD_SIP_E_SUCCESS) {			\
		error("%s failed ret %d\n", __func__, (int)sret);	\
	}							\
}while(0)
#else
# define NOISE_ON_FAIL(sret) do{}while(0)
#endif

typedef void (*sip_invoke_fn)(unsigned long, unsigned long, unsigned long,
			unsigned long, unsigned long, unsigned long,
			unsigned long, unsigned long,
			struct arm_smccc_res *);

static int security_state = EXEC_STATE_NORMAL;	/* non-secure always */
static sip_invoke_fn invoke_fn = NULL;

static inline void reg_pair_from_64(uint32_t *reg0, uint32_t *reg1, uint64_t val)
{
	*reg0 = val >> 32;
	*reg1 = val;
}

static int sip_to_tee_ret(const int sret)
{
	if (sret == SD_SIP_E_SUCCESS)
		return TEE_SVC_E_OK;
	else if (sret == SD_SIP_E_INVALID_PARAM)
		return TEE_SVC_E_INVAL;
	else if (sret == SD_SIP_E_NOT_SUPPORTED)
		return TEE_SVC_E_NOT_SUPPORT;
	else if (sret == SD_SIP_E_INVALID_RANGE)
		return TEE_SVC_E_INVALID_RANGE;
	else if (sret == SD_SIP_E_PERMISSION_DENY)
		return TEE_SVC_E_PERMISSION_DENY;
	else if (sret == SD_SIP_E_LOCK_FAIL)
		return TEE_SVC_E_LOCK_FAIL;
	else if (sret == SD_SIP_E_SMALL_BUFFER)
		return TEE_SVC_E_SMALL_BUFFER;
	else
		return TEE_SVC_E_ERROR;
}

static bool sip_api_uid_is_sd_api(sip_invoke_fn fn)
{
	struct arm_smccc_res res;
	fn(SIP_SVC_UID, 0, 0, 0, 0, 0, 0, 0, &res);

	debug("SIP UID %lx %lx %lx %lx\n", res.a0, res.a1, res.a2, res.a3);
	if (res.a0 == SD_SIP_UID_0 && res.a1 == SD_SIP_UID_1 &&
	    res.a2 == SD_SIP_UID_2 && res.a3 == SD_SIP_UID_3)
		return true;
	return false;
}

static bool sip_api_revision_is_compatible(sip_invoke_fn fn)
{
	struct arm_smccc_res res;
	fn(SIP_SVC_VERSION, 0, 0, 0, 0, 0, 0, 0, &res);

	if (res.a0 == SD_SIP_SVC_VERSION_MAJOR &&
	    res.a1 >= SD_SIP_SVC_VERSION_MINOR)
		return true;
	return false;
}

static int sd_sip_svc_probe(void)
{
	sip_invoke_fn fn = arm_smccc_smc;
	if (!sip_api_uid_is_sd_api(fn)) {
		error("api uid mismatch\n");
		return -EINVAL;
	}

	if (!sip_api_revision_is_compatible(fn)) {
		error("api revision mismatch\n");
		return -EINVAL;
	}

	invoke_fn = fn;
	return 0;
}

static int sd_sip_get_security_state(void)
{
	return security_state;
}

static int sd_sip_set_mem_protection(unsigned long va, unsigned long sz)
{
	struct arm_smccc_res res;
	uint32_t high, low;
	BUG_ON(invoke_fn == NULL);
	reg_pair_from_64(&high, &low, PHYADDR(va));
	invoke_fn(SD_SIP_FUNC_N_SET_PST, high, low, sz, 0, 0, 0, 0, &res);
	NOISE_ON_FAIL(res.a0);
	return sip_to_tee_ret(res.a0);
}

static int sd_sip_mmio(unsigned long mode, unsigned long pa, unsigned long a2, unsigned long a3, unsigned int wnr)
{
	unsigned long op;
	struct arm_smccc_res res;
	uint32_t high, low;
	BUG_ON(invoke_fn == NULL);
	reg_pair_from_64(&high, &low, pa);
	op = ((mode & SEC_MMIO_MODE_MASK) << SEC_MMIO_MODE_SHIFT) |
		(wnr & SEC_MMIO_CMD_WNR);
	invoke_fn(SD_SIP_FUNC_N_MMIO, op, high, low, a2, a3, 0, 0, &res);
	NOISE_ON_FAIL(res.a0);
	if (SD_SIP_E_SUCCESS == res.a0 &&
	    !(op & SEC_MMIO_CMD_WNR)) {
		BUG_ON(a2 == 0);
		*(uint32_t*)a2 = res.a1;
	}
	return sip_to_tee_ret(res.a0);
}

static int sd_sip_fuse_read(unsigned long ofs, unsigned long va, unsigned int *size, unsigned int *pprot)
{
	struct arm_smccc_res res;
	uint32_t high, low;
	BUG_ON(invoke_fn == NULL);
	if (NULL == size)
		return TEE_SVC_E_INVAL;
	reg_pair_from_64(&high, &low, PHYADDR(va));
	invoke_fn(SD_SIP_FUNC_C_OTP_READ, ofs, high, low, *size, 0, 0, 0, &res);
	NOISE_ON_FAIL(res.a0);
	if (SD_SIP_E_SUCCESS == res.a0) {
		if (pprot != NULL)
			*pprot = res.a1;
		*size = res.a2;
	}
	return sip_to_tee_ret(res.a0);
}

static int sd_sip_get_rsa_key(unsigned long va, unsigned long len)
{
	struct arm_smccc_res res;
	uint32_t high, low;
	BUG_ON(invoke_fn == NULL);
	reg_pair_from_64(&high, &low, PHYADDR(va));
	invoke_fn(SD_SIP_FUNC_N_RSA_KEY, high, low, len, 0, 0, 0, 0, &res);
	NOISE_ON_FAIL(res.a0);
	return sip_to_tee_ret(res.a0);
}

static int sd_sip_get_mem_state(unsigned long pa, unsigned long len, unsigned int *pstate)
{
	struct arm_smccc_res res;
	uint32_t high, low;
	BUG_ON(invoke_fn == NULL);
	reg_pair_from_64(&high, &low, pa);
	invoke_fn(SD_SIP_FUNC_C_MEM_STATE, high, low, len, 0, 0, 0, 0, &res);
	NOISE_ON_FAIL(res.a0);
	if (SD_SIP_E_SUCCESS == res.a0) {
		BUG_ON(pstate == NULL);
		*pstate = res.a1;
	}
	return sip_to_tee_ret(res.a0);
}


TEE_OPS("sd sip",
	sd_sip_svc_probe,
	sd_sip_get_security_state,
	sd_sip_set_mem_protection,
	NULL,
	sd_sip_mmio,
	sd_sip_fuse_read,
	sd_sip_get_rsa_key,
	sd_sip_get_mem_state)
