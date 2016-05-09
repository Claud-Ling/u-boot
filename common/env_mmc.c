/*
 * (C) Copyright 2008-2011 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/* #define DEBUG */

#include <common.h>

#include <command.h>
#include <environment.h>
#include <linux/stddef.h>
#include <malloc.h>
#include <memalign.h>
#include <mmc.h>
#include <search.h>
#include <errno.h>
#ifdef CONFIG_ARCH_SIGMA_TRIX
#include <asm/arch/setup.h>
#include <linux/ctype.h>
#endif

#if defined(CONFIG_ENV_SIZE_REDUND) &&  \
	(CONFIG_ENV_SIZE_REDUND != CONFIG_ENV_SIZE)
#error CONFIG_ENV_SIZE_REDUND should be the same as CONFIG_ENV_SIZE
#endif

char *env_name_spec = "MMC";


int have_static_env = 0;
int is_double_env_group = 0;
int env_signature_check = 0;
int bank_switch = 0;

#ifdef CONFIG_ENV_AUTHENTICATION

#ifndef CONFIG_MAX_ENVVARS 
#define CONFIG_MAX_ENVVARS (512)
#endif

#ifndef CONFIG_SIZE_ALLOWED_VARS
#define CONFIG_SIZE_ALLOWED_VARS (1024)
#endif

char *dynenv_tokens[CONFIG_MAX_ENVVARS];
char allowed_vars[CONFIG_SIZE_ALLOWED_VARS];
static char *pdynvar = NULL;
static struct hsearch_data hs_tab;
#endif

#ifdef ENV_IS_EMBEDDED
env_t *env_ptr = &environment;
#else /* ! ENV_IS_EMBEDDED */
env_t *env_ptr;
#endif /* ENV_IS_EMBEDDED */

DECLARE_GLOBAL_DATA_PTR;

#if !defined(CONFIG_ENV_OFFSET)
#define CONFIG_ENV_OFFSET 0
#endif

enum {
	DYNAMIC_ENV1 = 0,
	DYNAMIC_ENV2,
	STATIC_ENV,
	INVALID_ENV
};
enum {
	ENV_GROUP1,
	ENV_GROUP2,
};
enum {
	SIG_CHECK_SUCESS = 0,
	SIG_CHECK_FAIL
};

#ifdef CONFIG_ENV_AUTHENTICATION
static void merge_env(env_t* env_a, env_t* env_b);
static int parse_dynvar_list(char *pdynvar, char *from); 
extern int mmc_send_ext_csd(struct mmc *mmc, char *ext_csd);

static int __mmc_get_env_group(struct mmc *mmc)
{
	int err = 0;
	ALLOC_CACHE_ALIGN_BUFFER(char, ext_csd, 512);
	
	if (!is_double_env_group) {
		return ENV_GROUP1;
	}

	err = mmc_send_ext_csd(mmc, ext_csd);

	if (!err) {
		return (((ext_csd[EXT_CSD_PART_CONF] >> 3) & 0x7) == \
			0x1 ? ENV_GROUP1 : ENV_GROUP2);
	}
	/*  IMPOSSIABLE !!
 	** if cann't access eMMC extcsd ,
	** alway select default group "ENV_GROUP1"
	*/
	printf("Can't access eMMC extcsd for ENV group judgement, is eMMC broken??\n");
	return ENV_GROUP1;
}
static int __mmc_get_double_redund_env_addr(struct mmc *mmc, 
					int type, u32 *env_addr)
{
	int group;

	group = __mmc_get_env_group(mmc);

	switch (type) {
	case DYNAMIC_ENV1:
		(*env_addr) = ((group == ENV_GROUP1) ? 
		CONFIG_ENV_GROUP1_DYN1_OFFSET : CONFIG_ENV_GROUP2_DYN1_OFFSET);
		break;
	case DYNAMIC_ENV2:
		(*env_addr) = ((group == ENV_GROUP1) ? 
		CONFIG_ENV_GROUP1_DYN2_OFFSET : CONFIG_ENV_GROUP2_DYN2_OFFSET);
		break;
	case STATIC_ENV:
		(*env_addr) = ((group == ENV_GROUP1) ? 
		CONFIG_ENV_GROUP1_STATIC_OFFSET : CONFIG_ENV_GROUP2_STATIC_OFFSET);
		break;
	default:
		(*env_addr) = 0;
		printf("Error: unsupport ENV type\n");
		break;
	}
	
	return 0;
}
#endif

__weak int mmc_get_env_addr(struct mmc *mmc, int copy, u32 *env_addr)
{
	s64 offset;

	offset = CONFIG_ENV_OFFSET;
#ifdef CONFIG_ENV_AUTHENTICATION
	return __mmc_get_double_redund_env_addr(mmc, copy, env_addr);
#endif

#ifdef CONFIG_ENV_OFFSET_REDUND
	if (copy == DYNAMIC_ENV2)
		offset = CONFIG_ENV_OFFSET_REDUND;
#endif

	if (offset < 0)
		offset += mmc->capacity;

	*env_addr = offset;

	return 0;
}

__weak int mmc_get_env_dev(void)
{
	return CONFIG_SYS_MMC_ENV_DEV;
}

int env_init(void)
{
	/* use default */
	gd->env_addr	= (ulong)&default_environment[0];
	gd->env_valid	= 1;

	return 0;
}

#ifdef CONFIG_SYS_MMC_ENV_PART
__weak uint mmc_get_env_part(struct mmc *mmc)
{
	return CONFIG_SYS_MMC_ENV_PART;
}

static unsigned char env_mmc_orig_hwpart;

static int mmc_set_env_part(struct mmc *mmc)
{
	uint part = mmc_get_env_part(mmc);
	int dev = mmc_get_env_dev();
	int ret = 0;

#ifdef CONFIG_SPL_BUILD
	dev = 0;
#endif

	env_mmc_orig_hwpart = mmc->block_dev.hwpart;
	ret = mmc_select_hwpart(dev, part);
	if (ret)
		puts("MMC partition switch failed\n");

	return ret;
}
#else
static inline int mmc_set_env_part(struct mmc *mmc) {return 0; };
#endif

static const char *init_mmc_for_env(struct mmc *mmc)
{
	if (!mmc)
		return "!No MMC card found";

	if (mmc_init(mmc))
		return "!MMC init failed";

	if (mmc_set_env_part(mmc))
		return "!MMC partition switch failed";

	return NULL;
}

static void fini_mmc_for_env(struct mmc *mmc)
{
#ifdef CONFIG_SYS_MMC_ENV_PART
	int dev = mmc_get_env_dev();

#ifdef CONFIG_SPL_BUILD
	dev = 0;
#endif
	mmc_select_hwpart(dev, env_mmc_orig_hwpart);
#endif
}

#ifdef CONFIG_CMD_SAVEENV
static inline int write_env(struct mmc *mmc, unsigned long size,
			    unsigned long offset, const void *buffer)
{
	uint blk_start, blk_cnt, n;

	blk_start	= ALIGN(offset, mmc->write_bl_len) / mmc->write_bl_len;
	blk_cnt		= ALIGN(size, mmc->write_bl_len) / mmc->write_bl_len;

	n = mmc->block_dev.block_write(&mmc->block_dev, blk_start,
					blk_cnt, (u_char *)buffer);

	return (n == blk_cnt) ? 0 : -1;
}

#ifdef CONFIG_ENV_OFFSET_REDUND
static unsigned char env_flags;
#endif

int saveenv(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(env_t, env_new, 1);
	int dev = mmc_get_env_dev();
	struct mmc *mmc = find_mmc_device(dev);
	u32	offset;
	int	ret, copy = DYNAMIC_ENV1;
	const char *errmsg;

	errmsg = init_mmc_for_env(mmc);
	if (errmsg) {
		printf("%s\n", errmsg);
		return 1;
	}

	ret = env_export(env_new);
	if (ret)
		goto fini;

#ifdef CONFIG_ENV_OFFSET_REDUND
	env_new->flags	= ++env_flags; /* increase the serial */

	if (gd->env_valid == 1)
		copy = 1;
#endif

	if (mmc_get_env_addr(mmc, copy, &offset)) {
		ret = 1;
		goto fini;
	}

	printf("Writing to %sMMC(%d)... ", copy ? "redundant " : "", dev);
	if (write_env(mmc, CONFIG_ENV_SIZE, offset, (u_char *)env_new)) {
		puts("failed\n");
		ret = 1;
		goto fini;
	}

	puts("done\n");
	ret = 0;

#ifdef CONFIG_ENV_OFFSET_REDUND
	gd->env_valid = gd->env_valid == 2 ? 1 : 2;
#endif

fini:
	fini_mmc_for_env(mmc);
	return ret;
}
#endif /* CONFIG_CMD_SAVEENV */

static inline int read_env(struct mmc *mmc, unsigned long size,
			   unsigned long offset, const void *buffer)
{
	uint blk_start, blk_cnt, n;

	blk_start	= ALIGN(offset, mmc->read_bl_len) / mmc->read_bl_len;
	blk_cnt		= ALIGN(size, mmc->read_bl_len) / mmc->read_bl_len;

	n = mmc->block_dev.block_read(&mmc->block_dev, blk_start, blk_cnt,
				      (uchar *)buffer);

	return (n == blk_cnt) ? 0 : -1;
}

#ifdef CONFIG_ENV_AUTHENTICATION
__attribute__((weak)) 
int signature_check(void *data, int len, int key_type, void *signature)
{
	printf("Not enable security check\n");
	return 0;
}
int authenticate_static_env(struct mmc *mmc)
{
	extern int signature_check(void *data, int len, int key_type, void *signature);
	int ret = -1;
	u32 env_offset;
	void *signature = NULL;

	if (env_signature_check == 0) {
		ret = SIG_CHECK_SUCESS;
		goto out;
	}
	ALLOC_CACHE_ALIGN_BUFFER(char, data, CONFIG_ENV_SIZE+512);
	signature =(void *)((unsigned int)data + CONFIG_ENV_SIZE);
	mmc_get_env_addr(mmc, STATIC_ENV, &env_offset);

	ret = read_env(mmc, CONFIG_ENV_SIZE+512, env_offset, data);
	if (ret) {
		printf("%s read env failed\n", __FUNCTION__);
		ret = SIG_CHECK_FAIL;
		goto out;
	}
	ret = signature_check(data, CONFIG_ENV_SIZE, 1, signature);
	if (ret) {
		printf("%s env signature check failed\n", __FUNCTION__);
		ret = SIG_CHECK_FAIL;
		goto out;
	}
	ret = SIG_CHECK_SUCESS;
out:
	return ret;
}
static int load_env_feature_from_boardcfg(struct mmc *mmc)
{
	//TODO:
	return 0;
}
static int load_env_feature_from_mmc(struct mmc *mmc)
{
	int read1_fail, read2_fail;
	int crc1_ok, crc2_ok;
	int valid_env = 0;
	u32 offset1=CONFIG_ENV_OFFSET;
	u32 offset2=CONFIG_ENV_OFFSET_REDUND;
	env_t *penv = NULL;
	ENTRY e, *ep;

	ALLOC_CACHE_ALIGN_BUFFER(env_t, tmp_env1, 1);
	ALLOC_CACHE_ALIGN_BUFFER(env_t, tmp_env2, 1);

	init_mmc_for_env(mmc);

	read1_fail = read_env(mmc, CONFIG_ENV_SIZE, \
					offset1, tmp_env1);
	read2_fail = read_env(mmc, CONFIG_ENV_SIZE, \
					offset2, tmp_env2);

	if (read1_fail) 
		printf("read1_fail\n");
	if (read2_fail) 
		printf("read2_fail\n");
	if (read1_fail && read2_fail) {
		puts("redundent ENV read faild\n");
		goto out;
	}
	crc1_ok = !read1_fail &&
		(crc32(0, tmp_env1->data, ENV_SIZE) == tmp_env1->crc);
	crc2_ok = !read2_fail &&
		(crc32(0, tmp_env2->data, ENV_SIZE) == tmp_env2->crc);

	
	if (!crc1_ok && !crc2_ok) {
		/*Do nothing! env is corrupted*/
		printf("crc1 & 2 failed\n");
		goto out;
	} else if (crc1_ok && !crc2_ok) {
		valid_env = 1;
	} else if (!crc1_ok && crc2_ok) {
		valid_env = 2;
	} else {
		/* both ok - check serial */
		if (tmp_env1->flags == 255 && tmp_env2->flags == 0)
			valid_env = 2;
		else if (tmp_env2->flags == 255 && tmp_env1->flags == 0)
			valid_env = 1;
		else if (tmp_env1->flags > tmp_env2->flags)
			valid_env = 1;
		else if (tmp_env2->flags > tmp_env1->flags)
			valid_env = 2;
		else /* flags are equal - almost impossible */
			valid_env = 1;
	}
	if (valid_env == 1) {
		penv = tmp_env1;
	} else {
		penv = tmp_env2;
	}

	himport_r(&hs_tab, (char *)penv->data, ENV_SIZE, '\0', 0, 0, 0, NULL);
	e.key = "have_double_env";
	e.data = NULL;
	hsearch_r(e, FIND, &ep, &hs_tab, 0);
	if (ep) {
		is_double_env_group = \
		((ep->data[0] == 'Y' || ep->data[0] == 'y') ? 1 : 0);
	}
	e.key = "have_static_env";
	e.data = NULL;
	hsearch_r(e, FIND, &ep, &hs_tab, 0);
	if (ep) {
		have_static_env = \
		((ep->data[0] == 'Y' || ep->data[0] == 'y') ? 1 : 0);
	}

	e.key = "env_signature_check";
	e.data = NULL;
	hsearch_r(e, FIND, &ep, &hs_tab, 0);
	if (ep) {
		env_signature_check = \
		((ep->data[0] == 'Y' || ep->data[0] == 'y') ? 1 : 0);
	}

	e.key = "bank_switch";
	e.data = NULL;
	hsearch_r(e, FIND, &ep, &hs_tab, 0);
	if (ep) {
		bank_switch = \
		((ep->data[0] == 'Y' || ep->data[0] == 'y') ? 1 : 0);
	}
out:
	return 0;
}

static int __judgement_for_mmc_env_feature(struct mmc *mmc)
{
	load_env_feature_from_boardcfg(mmc);
	env_signature_check = SIMGA_TRIX_SECURITY_STATE;
	load_env_feature_from_mmc(mmc);
	return 0;
}
#endif

#ifdef CONFIG_ENV_OFFSET_REDUND
void env_relocate_spec(void)
{
#if !defined(ENV_IS_EMBEDDED)
	struct mmc *mmc;
	u32 offset1, offset2;
	int read1_fail = 0, read2_fail = 0;
	int crc1_ok = 0, crc2_ok = 0;
	env_t *ep;
	int ret;
	int dev = mmc_get_env_dev();
	const char *errmsg = NULL;

	int crc_staticenv_ok = 0;
#ifdef CONFIG_ENV_AUTHENTICATION
	u32 staticenv_offset;
	int read_staticenv_fail = 0;
	int check_status = SIG_CHECK_FAIL;

	ALLOC_CACHE_ALIGN_BUFFER(env_t, static_env, 1);
#endif
	ALLOC_CACHE_ALIGN_BUFFER(env_t, tmp_env1, 1);
	ALLOC_CACHE_ALIGN_BUFFER(env_t, tmp_env2, 1);

#ifdef CONFIG_SPL_BUILD
	dev = 0;
#endif

	mmc = find_mmc_device(dev);

	errmsg = init_mmc_for_env(mmc);
	if (errmsg) {
		ret = 1;
		goto err;
	}

#ifdef CONFIG_ENV_AUTHENTICATION
	__judgement_for_mmc_env_feature(mmc);
#endif

	if (mmc_get_env_addr(mmc, DYNAMIC_ENV1, &offset1) ||
	    mmc_get_env_addr(mmc, DYNAMIC_ENV2, &offset2)) {
		ret = 1;
		goto fini;
	}
#ifdef CONFIG_ENV_AUTHENTICATION
	if (mmc_get_env_addr(mmc, STATIC_ENV, &staticenv_offset)) {
		ret = 1;
		goto fini;
	}
#endif
	read1_fail = read_env(mmc, CONFIG_ENV_SIZE, offset1, tmp_env1);
	read2_fail = read_env(mmc, CONFIG_ENV_SIZE, offset2, tmp_env2);
#ifdef CONFIG_ENV_AUTHENTICATION
	if (have_static_env) {
		read_staticenv_fail = read_env(mmc, CONFIG_ENV_SIZE, staticenv_offset, static_env);
	}
#endif

	if (read1_fail && read2_fail)
		puts("*** Error - No Valid Environment Area found\n");
	else if (read1_fail || read2_fail)
		puts("*** Warning - some problems detected "
		     "reading environment; recovered successfully\n");

	crc1_ok = !read1_fail &&
		(crc32(0, tmp_env1->data, ENV_SIZE) == tmp_env1->crc);
	crc2_ok = !read2_fail &&
		(crc32(0, tmp_env2->data, ENV_SIZE) == tmp_env2->crc);
#ifdef CONFIG_ENV_AUTHENTICATION
	if (have_static_env) {
		crc_staticenv_ok = !read_staticenv_fail &&
			(crc32(0, static_env->data, ENV_SIZE) == static_env->crc);
		printf("Static env crc %s\n", (crc_staticenv_ok ? "OK!!":"Failed!!"));
		static_env->flags = (crc_staticenv_ok ? 0 : 1);

		check_status = authenticate_static_env(mmc);

		if (check_status == SIG_CHECK_FAIL) {
			ret = 1;
			goto fini;
		}
	}
#endif

	if (!crc1_ok && !crc2_ok) {
		errmsg = "!bad CRC";
		ret = 1;
		if (!have_static_env || \
			(have_static_env && !crc_staticenv_ok)) {

			goto fini;
		}
	} else if (crc1_ok && !crc2_ok) {
		gd->env_valid = 1;
	} else if (!crc1_ok && crc2_ok) {
		gd->env_valid = 2;
	} else {
		/* both ok - check serial */
		if (tmp_env1->flags == 255 && tmp_env2->flags == 0)
			gd->env_valid = 2;
		else if (tmp_env2->flags == 255 && tmp_env1->flags == 0)
			gd->env_valid = 1;
		else if (tmp_env1->flags > tmp_env2->flags)
			gd->env_valid = 1;
		else if (tmp_env2->flags > tmp_env1->flags)
			gd->env_valid = 2;
		else /* flags are equal - almost impossible */
			gd->env_valid = 1;
	}

	free(env_ptr);

	if (gd->env_valid == 1)
		ep = tmp_env1;
	else
		ep = tmp_env2;

#ifdef CONFIG_ENV_AUTHENTICATION
	if (have_static_env)
		merge_env(ep, static_env);
#endif
	env_flags = ep->flags;
	env_import((char *)ep, 0);
	ret = 0;

fini:
	fini_mmc_for_env(mmc);
err:
	if (ret)
		set_default_env(errmsg);
#endif
}
#else /* ! CONFIG_ENV_OFFSET_REDUND */
void env_relocate_spec(void)
{
#if !defined(ENV_IS_EMBEDDED)
	ALLOC_CACHE_ALIGN_BUFFER(char, buf, CONFIG_ENV_SIZE);
	struct mmc *mmc;
	u32 offset;
	int ret;
	int dev = mmc_get_env_dev();
	const char *errmsg;

#ifdef CONFIG_SPL_BUILD
	dev = 0;
#endif

	mmc = find_mmc_device(dev);

	errmsg = init_mmc_for_env(mmc);
	if (errmsg) {
		ret = 1;
		goto err;
	}

	if (mmc_get_env_addr(mmc, DYNAMIC_ENV1, &offset)) {
		ret = 1;
		goto fini;
	}

	if (read_env(mmc, CONFIG_ENV_SIZE, offset, buf)) {
		errmsg = "!read failed";
		ret = 1;
		goto fini;
	}

	env_import(buf, 1);
	ret = 0;

fini:
	fini_mmc_for_env(mmc);
err:
	if (ret)
		set_default_env(errmsg);
#endif
}
#endif /* CONFIG_ENV_OFFSET_REDUND */

#ifdef CONFIG_ENV_AUTHENTICATION
int is_dyn_var(char *item)
{
	int chg_ok = 0;

	if (strcmp(dynenv_tokens[0], "<all>") == 0) {
		chg_ok = 1;
	} else {
		int i=0;		
		while (dynenv_tokens[i]) {
			if (strcmp(dynenv_tokens[i], item) == 0) {
				// found it
				chg_ok = 1;
				break;
			}

			i++;
		}
	}

	return chg_ok;
}


/* Parse the dynamic env var list from *from. 
 * Input:
 *    from: the orginal list as from getenv().
 *    pdynvar: copy of from, with multiple space removed.
 * Output: 
 *    dynenv_tokens: a pointer array, each point to a dyn var. 
 */
static int parse_dynvar_list(char *pdynvar, char *from) 
{
	char *p, *cp;
	int i, len, k;

	/* For no __DYNAMIC_VARS variable in static ENV case */
	if (from == NULL) {
		dynenv_tokens[0] = "<all>";
		return 0;
	}

	p = from;
	len = strlen(p) + 1; //including '\0'
	
	// Replace \t with space; remove extra spaces....;
	i = 0; 
	k = 0;
	while (i<(len - 1)) {
		// replace first occurrence of tab, return char with space		
		if ( isspace(p[i]) ) {
			if (k>0) {
				pdynvar[k] = ' ';
				//debug("==>pdynvar[%d]=%c\n", k, pdynvar[k]);
				k++;
			}
			
			i++;
			// skip subsequence space, tab, return chars		
			while ( isspace(p[i]) ) {
				//debug("i=%d skip 0x%02x\n", i, p[i]);
				i++;
			}
		} else {
			pdynvar[k] = p[i];
			//debug("==>pdynvar[%d]=%c\n", k, pdynvar[k]);
			k++;
			i++;
		}
	}
	pdynvar[k] = 0;
	
	// Remove the trailing "space"
	//len = strlen(pdynvar) - 1;
	len = k - 1;
	while ( isspace(pdynvar[len]) ) { 
		len--;
	}
	pdynvar[len+1] = 0x0;	

	i = 0;
	cp = pdynvar;
	while ( cp && *cp) {
		dynenv_tokens[i++] = cp;

		// Find the next space
		cp = strpbrk(cp, " ");
		if( cp ) {
		*cp = 0;
		cp ++;
	}

	}
	dynenv_tokens[i] = NULL;

	return i;
}

#if 0
static void show_dynvars(void) 
{
	int i = 0;

	debug("dynenv_tokens@%p\n", dynenv_tokens);
	while (dynenv_tokens[i] != NULL) {
		debug("dynenv_tokens[%d]=*%s*\n", i, dynenv_tokens[i]);
		i++;
	}
}
#endif

static void merge_env(env_t* env_a, env_t* env_b) {

	int	i;
	int	nxt;
	char *p = NULL;
	ENTRY e, *ep;

	if (env_b->flags == 1) {
	/* static env is broken, skip merge operation */
		goto error;
	}

	himport_r(&hs_tab, (char *)env_b->data, ENV_SIZE, '\0', 0, 0, 0, NULL);

	e.key = "__DYNAMIC_VARS";
	e.data = NULL;
	hsearch_r(e, FIND, &ep, &hs_tab, 0);

	if (ep)
		p = ep->data;


	pdynvar = allowed_vars;
	parse_dynvar_list(pdynvar, p);

	/* iterate env_a */
	for (i = 0; env_a->data[i] != '\0'; i = nxt + 1) {
		char*	item;
		char*	val;

		item = val = (char*)&env_a->data[i];

		/* pre-find next item.. */
		for (nxt = i; env_a->data[nxt] != '\0'; ++nxt) {
			if ( (item==val)  &&  (env_a->data[nxt] == '=') ) {
				val = (char*)&env_a->data[nxt];
				*val++ = '\0';	
			}
			
			if (nxt >= CONFIG_ENV_SIZE)
				goto MERGE_ENV_DONE;
		}

		if (item != val) {
			debug("--->env_a['%s']='%s'\n", item, val);

			if (is_dyn_var(item)) {
			/*
 			 *	if p = NULL, that means there is no variable
 			 *	nanmed __DYNAMIC_VARS in static ENV, just merge
 			 *	all variables form dynamic env to static env	
 			 */
				e.key = item;
				e.data = val;
				hsearch_r(e, ENTER, &ep, &hs_tab, 0);
				if (!ep)
					printf("%s, %d: Error inserting %svariable", __FUNCTION__, __LINE__, item);

			} else {
				debug("Warning: does not allow to change %s to %s\n", item, val);
			}
		}
	}

MERGE_ENV_DONE:
	/* destroy the embedded environment so that we dont process it twice
	 * or process an environment that is stale.. we dont like stale milk,
	 * same goes for environments :-) */
	env_a->crc = ~0;
	p = (char *)&env_a->data;
	hexport_r(&hs_tab, '\0', 0, &p, ENV_SIZE, 0, NULL);
	debug("export finished\n");

error:
	printf("merge done!\n");
}
#endif
