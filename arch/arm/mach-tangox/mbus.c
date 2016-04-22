//#define DEBUG

#include <common.h>
#include <malloc.h>

#include <asm/io.h>
#include <asm/errno.h>

#include <linux/compiler.h>

#include <asm/arch/setup.h>


#define MSG                 printf

/* max size for linear transfer */
#define MBUS_LINEAR_MAX		(0x2000 - 1)

/* the number of available channel pairs */
#if defined(CONFIG_TANGO3) || defined(CONFIG_TANGO4)
#define NUM_MBUS_CHNPS	3
#else
#define NUM_MBUS_CHNPS	2
#endif /* CONFIG_TANGO3 || CONFIG_TANGO4 */

/* let's use W1R1 */
#define WITH_MBUS_W1R1

// TODO: dmb() is declared in asm/io.h
#define iob()   dmb() //sys_sync()
#define wmb()   dmb()  //sys_sync()

// MBUS interface
#define MIF_add_offset              0x0
#define MIF_cnt_offset              (MIF_W0_CNT  - MIF_W0_ADD) //0x04
#define MIF_add2_skip_offset        (MIF_W0_SKIP - MIF_W0_ADD) //0x08
#define MIF_cmd_offset              (MIF_W0_CMD  - MIF_W0_ADD) //0x0c

extern unsigned long tangox_chip_id(void);

struct mbus_channel_pair
{
	unsigned int rx_base;	/* Rx base address */
	unsigned int wx_base;	/* Wx base address */
	const unsigned int shift;
	const unsigned int rx;
	const unsigned int idx;
	//const unsigned int rx_irq;
	//const unsigned int wx_irq;
	//const char *rx_irq_name;
	//const char *wx_irq_name;
	//spinlock_t lock;		/* spin_lock */
	//mbus_irq_handler_t handler;	
	//void *arg;
	int iface;			/* which interface it's connected */
	int fromdev;        /* from device? */
};

static struct mbus_channel_pair mchnp_list[NUM_MBUS_CHNPS] = {
	{ 
		.fromdev = 0,
		.rx_base = REG_BASE_host_interface + MIF_R0_ADD,
		.wx_base = REG_BASE_host_interface + MIF_W0_ADD, 
//		.handler = NULL,
//		.arg = NULL,
		.iface = 0xf,
		.shift = 0,
		.rx = 1,
		.idx = 0,
//#ifdef CONFIG_TANGO4
//		.rx_irq = LOG2_HOST_MBUS_R0_INT,
//		.wx_irq = LOG2_HOST_MBUS_W0_INT,
//#else
//		.rx_irq = LOG2_CPU_HOST_MBUS_R0_INT,
//		.wx_irq = LOG2_CPU_HOST_MBUS_W0_INT,
//#endif
//		.rx_irq_name = "tangox_mbus_r0",
//		.wx_irq_name = "tangox_mbus_w0",
	},
	{
		.fromdev = 0,
		.rx_base = REG_BASE_host_interface + MIF_R1_ADD,
		.wx_base = REG_BASE_host_interface + MIF_W1_ADD, 
//		.handler = NULL,
//		.arg = NULL,
#ifdef WITH_MBUS_W1R1
		.iface = 0xf,
#else
		.iface = -1,	/* cannot be allocated */
#endif
		.shift = 4,
		.rx = 2,
		.idx = 1,
//#ifdef CONFIG_TANGO4
//		.rx_irq = LOG2_HOST_MBUS_R1_INT,
//		.wx_irq = LOG2_HOST_MBUS_W1_INT,
//#else
//		.rx_irq = LOG2_CPU_HOST_MBUS_R1_INT,
//		.wx_irq = LOG2_CPU_HOST_MBUS_W1_INT,
//#endif
//		.rx_irq_name = "tangox_mbus_r1",
//		.wx_irq_name = "tangox_mbus_w1",
	},
#if defined(CONFIG_TANGO3) || defined(CONFIG_TANGO4)
	{
		.fromdev = 0,
		.rx_base = REG_BASE_host_interface + MIF_R2_ADD,
		.wx_base = REG_BASE_host_interface + MIF_W2_ADD, 
//		.handler = NULL,
//		.arg = NULL,
		.iface = 0xf,
		.shift = 32,
		.rx = 9,
		.idx = 2,
//#ifdef CONFIG_TANGO4
//		.rx_irq = LOG2_HOST_MBUS_R2_INT,
//		.wx_irq = LOG2_HOST_MBUS_W2_INT,
//#else
//		.rx_irq = LOG2_CPU_HOST_MBUS_R2_INT,
//		.wx_irq = LOG2_CPU_HOST_MBUS_W2_INT,
//#endif
//		.rx_irq_name = "tangox_mbus_r2",
//		.wx_irq_name = "tangox_mbus_w2",
	},
#endif
};

static int mbus_free_cnt = 0; /* counter of available channel pairs */

/*
 * convert MBUS register address to channel pair
 */
#if defined(CONFIG_TANGO2) || defined(CONFIG_TANGO3) 
/* old mbus semantics */
static struct mbus_channel_pair *__old_mbus_reg2chnp(unsigned int regbase)
{
	int idx = (regbase - (REG_BASE_host_interface + MIF_W0_ADD)) / 0x40;
#if defined(CONFIG_TANGO3) 
	static const int chnps[6] = { 0, 1, 0, 1, 2, 2 };
#elif defined(CONFIG_TANGO2)
	static const int chnps[4] = { 0, 1, 0, 1 };
#endif
	return &mchnp_list[chnps[idx]];
}
#endif

#if defined(CONFIG_TANGO3) || defined(CONFIG_TANGO4)
static struct mbus_channel_pair *__new_mbus_reg2chnp(unsigned int regbase)
{
	int idx = (regbase - (REG_BASE_host_interface + MIF_W0_ADD)) / 0x40;
	static const int chnps[8] = { 0, 1, 2, 3, 0, 1, 2, 3 };
	BUG_ON(chnps[idx] > 2); /* No W3/R3 yet */
	return &mchnp_list[chnps[idx]];
}
#endif

static struct mbus_channel_pair *(*__mbus_reg2chnp)(unsigned int) = NULL;


#if defined(CONFIG_TANGO2) || defined(CONFIG_TANGO3)
/* old mbus semantics */
static int __old_mbus_idx2chn(unsigned int idx)
{
	return idx; /* no conversion needed */
}
#endif

#if defined(CONFIG_TANGO3) || defined(CONFIG_TANGO4) 
static int __new_mbus_idx2chn(unsigned int idx)
{
	/* converting w0,w1,w2,hole,r0,r1,r2,hole to w0,w1,r0,r1,w2,r2 index */
	static const int chn_conv[8] = { 0, 1, 4, -1, 2, 3, 5, -1 };
	return chn_conv[idx];
}
#endif

static int (*__mbus_idx2chn)(unsigned int) = NULL;


#if defined(CONFIG_TANGO2) || defined(CONFIG_TANGO3) 
/* old mbus semantics */
static void __old_mbus_linear(unsigned int rwbase, unsigned int flags)
{
	gbus_write_reg32(rwbase + MIF_cmd_offset, (flags<<2)|0x1);
}

static void __old_mbus_double(unsigned int rwbase, unsigned int flags)
{
	gbus_write_reg32(rwbase + MIF_cmd_offset, (flags<<2)|0x2);
}

static void __old_mbus_rectangle(unsigned int rwbase, unsigned int flags)
{
	gbus_write_reg32(rwbase + MIF_cmd_offset, (flags<<2)|0x3);
}

static void __old_mbus_void(unsigned int rwbase)
{
	gbus_write_reg32(rwbase + MIF_cmd_offset, 1<<2);
}
#endif

#if defined(CONFIG_TANGO3) || defined(CONFIG_TANGO4) 
/* new mbus semantics */
static void __new_mbus_linear(unsigned int rwbase, unsigned int flags)
{
	gbus_write_reg32(rwbase + MIF_cmd_offset, flags|0x2);
}

static void __new_mbus_double(unsigned int rwbase, unsigned int flags)
{
	gbus_write_reg32(rwbase + MIF_cmd_offset, flags|0x4);
}

static void __new_mbus_rectangle(unsigned int rwbase, unsigned int flags)
{
	gbus_write_reg32(rwbase + MIF_cmd_offset, flags|0x6);
}

static void __new_mbus_void(unsigned int rwbase)
{
	gbus_write_reg32(rwbase + MIF_cmd_offset, 1);
}
#endif

static void (*__mbus_linear_action)(unsigned int, unsigned int) = NULL;
static void (*__mbus_double_action)(unsigned int, unsigned int) = NULL;
static void (*__mbus_rectangle_action)(unsigned int, unsigned int) = NULL;
static void (*__mbus_void_action)(unsigned int) = NULL;

/*
 * setup mbus register to start a linear transfer (count bytes from
 * addr, where count < MBUS_LINEAR_MAX)
 */
static void __mbus_setup_dma_linear(struct mbus_channel_pair *chnpptr,
					unsigned int addr,
					unsigned int count,
					unsigned int flags,
					unsigned int regbase)
{
	unsigned int rwbase;
///#if !defined(CONFIG_SD_DIRECT_DMA) && !defined(CONFIG_HIGHMEM)
///	if ((tangox_inv_dma_address(addr) < CPHYSADDR(em8xxx_kmem_start)) || (tangox_inv_dma_address(addr) >= (CPHYSADDR(em8xxx_kmem_start) + em8xxx_kmem_size)))
///		printk("MBUS Warning (linear): bad transfer address 0x%08x\n", addr);
///#endif
	if (unlikely(chnpptr == NULL)) {
		MSG("%s:%d ...\n", __FILE__, __LINE__);
		BUG();
	}

#ifdef CONFIG_TANGO2
	MSG("(%d) %s:%d setup_dma_linear idx=%d, route=0x%x, flags=%d\n", smp_processor_id(), __FILE__, __LINE__, gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE), flags);
#else
	//MSG("(%d) %s:%d setup_dma_linear idx=%d, route=0x%x%08x, flags=%d\n", smp_processor_id(), __FILE__, __LINE__, chnpptr->idx, gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE2), gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE), flags);
    debug("%s:%d setup_dma_linear idx=%d, route=0x%x%08x, flags=%d\n", __FILE__, __LINE__, chnpptr->idx, gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE2), gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE), flags);
#endif
	rwbase = regbase ? regbase : (chnpptr->fromdev ? chnpptr->wx_base : chnpptr->rx_base);
	gbus_write_reg32(rwbase + MIF_add_offset, addr);
	gbus_write_reg32(rwbase + MIF_cnt_offset, count);
	iob();
	(*__mbus_linear_action)(rwbase, flags);
	iob();
}

/*
 * setup mbus register to start a double transfer (count bytes from
 * addr and count2 bytes from addr2, where count < MBUS_LINEAR_MAX and
 * count2 < MBUS_LINEAR_MAX)
 */
static void __mbus_setup_dma_double(struct mbus_channel_pair *chnpptr,
					unsigned int addr,
					unsigned int count,
					unsigned int addr2,
					unsigned int count2,
					unsigned int flags,
					unsigned int regbase)
{
	unsigned int rwbase;
///#if !defined(CONFIG_SD_DIRECT_DMA) && !defined(CONFIG_HIGHMEM)
///	if ((tangox_inv_dma_address(addr) < CPHYSADDR(em8xxx_kmem_start)) || (tangox_inv_dma_address(addr) >= (CPHYSADDR(em8xxx_kmem_start) + em8xxx_kmem_size)))
///		printk("MBUS Warning (double): bad transfer address 0x%08x\n", addr);
///	if ((tangox_inv_dma_address(addr2) < CPHYSADDR(em8xxx_kmem_start)) || (tangox_inv_dma_address(addr2) >= (CPHYSADDR(em8xxx_kmem_start) + em8xxx_kmem_size)))
///		printk("MBUS Warning (double): bad transfer address2 0x%08x\n", addr2);
///#endif
	if (unlikely(chnpptr == NULL)) {
		MSG("%s:%d ...\n", __FILE__, __LINE__);
		BUG();
	}

#ifdef CONFIG_TANGO2
	MSG("(%d) %s:%d setup_dma_dbl idx=%d, route=0x%x, flags=%d\n", smp_processor_id(), __FILE__, __LINE__, chnpptr->idx, gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE), flags);
#else
	debug("%s:%d setup_dma_dbl idx=%d, route=0x%x%08x, flags=%d\n", __FILE__, __LINE__, chnpptr->idx, gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE2), gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE), flags);
#endif
	rwbase = regbase ? regbase : (chnpptr->fromdev ? chnpptr->wx_base : chnpptr->rx_base);
	gbus_write_reg32(rwbase + MIF_add_offset, addr);
	gbus_write_reg32(rwbase + MIF_cnt_offset, (count2 << 16) | count);
	gbus_write_reg32(rwbase + MIF_add2_skip_offset, addr2);
	iob();
	(*__mbus_double_action)(rwbase, flags);
	iob();
}

/*
 * setup mbus register to start a rectangle transfer (horiz * lines
 * bytes from addr, where horiz < MBUS_LINEAR_MAX and lines  <
 * MBUS_LINEAR_MAX)
 */
static void __mbus_setup_dma_rectangle(struct mbus_channel_pair *chnpptr,
					unsigned int addr,
					unsigned int horiz,
					unsigned int lines,
					unsigned int flags,
					unsigned int regbase)
{
	unsigned int rwbase;
///#if !defined(CONFIG_SD_DIRECT_DMA) && !defined(CONFIG_HIGHMEM)
///	if ((tangox_inv_dma_address(addr) < CPHYSADDR(em8xxx_kmem_start)) || (tangox_inv_dma_address(addr) >= (CPHYSADDR(em8xxx_kmem_start) + em8xxx_kmem_size)))
///		printk("MBUS Warning (rectangle): bad transfer address 0x%08x\n", addr);
///#endif
	if (unlikely(chnpptr == NULL)) {
		MSG("%s:%d ...\n", __FILE__, __LINE__);
		BUG();
	}

#ifdef CONFIG_TANGO2
	MSG("(%d) %s:%d setup_dma_rect idx=%d, route=0x%x, flags=%d\n", smp_processor_id(), __FILE__, __LINE__, chnpptr->idx, gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE), flags);
#else
	debug("%s:%d setup_dma_rect idx=%d, route=0x%x%08x, flags=%d\n", __FILE__, __LINE__, chnpptr->idx, gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE2), gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE), flags);
#endif
	rwbase = regbase ? regbase : (chnpptr->fromdev ? chnpptr->wx_base : chnpptr->rx_base);
	gbus_write_reg32(rwbase + MIF_add_offset, addr);
	gbus_write_reg32(rwbase + MIF_cnt_offset, (lines << 16) | horiz);
	gbus_write_reg32(rwbase + MIF_add2_skip_offset, horiz);
	iob();
	(*__mbus_rectangle_action)(rwbase, flags);
	iob();
}

/*
 * setup void transaction 
 */
static void __mbus_setup_dma_void(struct mbus_channel_pair *chnpptr,
					unsigned int regbase)
{
	unsigned int rwbase;

	if (unlikely(chnpptr == NULL)) {
		MSG("%s:%d ...\n", __FILE__, __LINE__);
		BUG();
	}

#ifdef CONFIG_TANGO2
	MSG("(%d) %s:%d setup_dma_void idx=%d, route=0x%x\n", smp_processor_id(), __FILE__, __LINE__, chnpptr->idx, gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE));
#else
    debug( "%s:%d setup_dma_void idx=%d, route=0x%x%08x\n", __FILE__, __LINE__, chnpptr->idx, gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE2), gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE));
#endif
	rwbase = regbase ? regbase : (chnpptr->fromdev ? chnpptr->wx_base : chnpptr->rx_base);
	(*__mbus_void_action)(rwbase);
	iob();
}


static inline int mbus_channel_rewire(struct mbus_channel_pair *chnpptr, int iface, int fromdev)
{
	u64 route = 0;
	int ret = -1;

	if (chnpptr->iface != 0xf) 
		goto done;

	chnpptr->iface = iface;	/* connected */
	chnpptr->fromdev = fromdev;
	route = ((u64)(iface + 1)) << chnpptr->shift;	/* hook up Wx */
	route |= (((u64)chnpptr->rx) << (iface * 4));	/* hook up Rx */
	wmb();
	
    debug("%s:%d connect route=0x%llx\n", __FILE__, __LINE__, route);
    
	gbus_write_reg32(REG_BASE_host_interface + SBOX_ROUTE, (u32)(route & 0xffffffff));
#if defined(CONFIG_TANGO3) || defined(CONFIG_TANGO4) 
	gbus_write_reg32(REG_BASE_host_interface + SBOX_ROUTE2, (u32)((route >> 32) & 0xffffffff));
#endif
	iob();
	
    debug("%s:%d allocate idx=%d\n", __FILE__, __LINE__, chnpptr->idx);
    
	ret = 0;

done:
	return ret;
}


/*
 * given SBOX interface, find/connect one of the available MBUS channel pairs
 * (i.e. Wx/Rx) and return the channel pair.
 */
static struct mbus_channel_pair *mbus_channel_pair_alloc(int iface, int fromdev, int canwait)
{
	int i;
	//unsigned long flags;
	struct mbus_channel_pair *ret = NULL, *chnpptr = NULL;
	static int order = 0;

	//spin_lock_irqsave(&mbus_lock, flags);
    
    debug("F: mbus_channel_pair_alloc\n");

	/* look for existed connection first */
	for (chnpptr = &mchnp_list[0], i = 0; i < NUM_MBUS_CHNPS; i++, chnpptr++) {
		if (chnpptr->iface == iface) { /* allocated to it already */
			chnpptr->fromdev = fromdev;
			ret = chnpptr;
			debug("%s:%d allocated idx=%d\n", __FILE__, __LINE__, chnpptr->idx);
			goto done;
		}
	}
    
    debug("mbus_free_cnt:%d\n", mbus_free_cnt );

	if (mbus_free_cnt == 0) { /* no channel pair available */

#if 0 // <===================================================================
#ifdef MBUS_WAIT_QUEUE
		if (canwait) {
			while (mbus_free_cnt == 0) {
				spin_unlock_irqrestore(&mbus_lock, flags);
				if (wait_event_interruptible(mbus_wq, mbus_free_cnt != 0)) 
					return NULL; /* being interrupted */
				spin_lock_irqsave(&mbus_lock, flags);
			}
		} else 
#endif
#endif // ==================================================================>
        MSG("%s:%d !Warning! mbus free count is 0\n", __FILE__, __LINE__);
        ret = NULL;    
        goto done;
	}
		
	/* no existed connection found, try forming new connection */
	if (order == 0) {
		for (chnpptr = &mchnp_list[0], i = 0; i < NUM_MBUS_CHNPS; i++, chnpptr++) {
			if (mbus_channel_rewire(chnpptr, iface, fromdev) == 0) {
				ret = chnpptr;
				break;
			}
		}
	} else {
		for (chnpptr = &mchnp_list[NUM_MBUS_CHNPS - 1], i = NUM_MBUS_CHNPS - 1; i >= 0; i--, chnpptr--) {
			if (mbus_channel_rewire(chnpptr, iface, fromdev) == 0) {
				ret = chnpptr;
				break;
			}
		}
	}
	if (ret) { /* new allocation is done */
		order = (order ? 0 : 1); /* swap order */
		--mbus_free_cnt;
		wmb();
	}

done:
	//spin_unlock_irqrestore(&mbus_lock, flags);
	return ret;
}

/*
 * free up allocated channel pair based on given SBOX interface.
 */
static int mbus_channel_pair_free(int iface)
{
	int i, ret = -1;
	//unsigned long flags;
	struct mbus_channel_pair *chnpptr = NULL;

	//spin_lock_irqsave(&mbus_lock, flags);
    
    debug("F: mbus_channel_pair_free\n");

	for (chnpptr = &mchnp_list[0], i = 0; i < NUM_MBUS_CHNPS; i++, chnpptr++) {
		if (chnpptr->iface == iface) { /* matched */
			u64 route = 0;
			chnpptr->fromdev = 0;
			chnpptr->iface = 0xf;	/* disconnected */
			route = ((u64)0xf) << chnpptr->shift;	/* hook up Wx */
			route |= (((u64)0xf) << (iface * 4));	/* hook up Rx */
			//MSG("(%d) %s:%d disconnect route=0x%llx\n", smp_processor_id(), __FILE__, __LINE__, route);
			wmb();
			gbus_write_reg32(REG_BASE_host_interface + SBOX_ROUTE, (u32)(route & 0xffffffff));
#if defined(CONFIG_TANGO3) || defined(CONFIG_TANGO4)
			gbus_write_reg32(REG_BASE_host_interface + SBOX_ROUTE2, (u32)((route >> 32) & 0xffffffff));
#endif
			iob();
			//MSG("(%d) %s:%d free idx=%d\n", smp_processor_id(), __FILE__, __LINE__, chnpptr->idx);
#ifdef CONFIG_TANGO2
			MSG("(%d) %s:%d route=0x%x\n", smp_processor_id(), __FILE__, __LINE__, gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE));
#else
			debug("%s:%d route=0x%x%08x\n",  __FILE__, __LINE__, gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE2), gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE));
            
#endif
			ret = 0;
			++mbus_free_cnt;
			wmb();
//#ifdef MBUS_WAIT_QUEUE
//			if (mbus_free_cnt == 1) /* one channel pair is available */
//				wake_up_interruptible(&mbus_wq);
//#endif
			break;
		}
	}

	//spin_unlock_irqrestore(&mbus_lock, flags);
	///BUG_ON(ret != 0);
	return ret;
}


/*
 * alloc mbus dma channels, need to be called before setup ...
 */
struct mbus_channel_pair *tangox_mbus_alloc_dma(int iface, int fromdev, int canwait)
{
	struct mbus_channel_pair *chnpptr = NULL;

	//chnpptr = mbus_channel_pair_alloc(iface, fromdev, 
	//		(in_atomic() || in_interrupt()) ? 0 : canwait);
            
    chnpptr = mbus_channel_pair_alloc(iface, fromdev, 0);

	return chnpptr;
}

/*
 * free mbus dma channels, need to be called after transfer is done ...
 */
void tangox_mbus_free_dma(struct mbus_channel_pair *chnpptr, int iface)
{
//	int tangox_mbus_wait(struct mbus_channel_pair *chnpptr, int iface);
//	tangox_mbus_wait(chnpptr, iface);

	if (mbus_channel_pair_free(iface) != 0) {
        MSG("%s:%d fail to free a channel!\n", __FILE__, __LINE__);
		BUG(); /* freeing non-allocated channels? */
    }
}


/*
 * check if mbus is in use for given regbase
 */
static inline int __tangox_mbus_inuse(struct mbus_channel_pair *chnpptr, unsigned int rwbase)
{
	if (unlikely(chnpptr == NULL)) {
		MSG("%s:%d ...\n", __FILE__, __LINE__);
		BUG();
	}
    
	if (rwbase)
		return gbus_read_reg32(rwbase + MIF_cmd_offset) & 0x7; 
	else
		return gbus_read_reg32((chnpptr->fromdev ? chnpptr->wx_base : chnpptr->rx_base) + MIF_cmd_offset) & 0x7; 
}


/*
 * start a mbus dma, use this after a sucessfull call to
 * tangox_mbus_alloc_dma
 */
static int __tangox_mbus_setup_dma(struct mbus_channel_pair *chnpptr,unsigned int addr,
					unsigned int count, mbus_irq_handler_t handler,
					void *arg, unsigned int tflags, unsigned int regbase)
{
	//unsigned long flgs;
	unsigned int horiz, lines, sz;
	int idx;

	if (unlikely(chnpptr == NULL)) {
		MSG("%s:%d ...\n", __FILE__, __LINE__);
		BUG();
	}

	/*
	 * make sure no one uses the mbus before
	 */
	if (unlikely((sz = __tangox_mbus_inuse(chnpptr, regbase)) != 0)) {
		//printk(KERN_ERR "MBUS: error previous command is pending (%d:%d) ...\n", chnpptr->idx, sz);
        MSG( "MBUS: error previous command is pending (%d:%d) ...\n", chnpptr->idx, sz );
		return 1;
	}

	/* registering given handler */
	//spin_lock_irqsave(&chnpptr->lock, flgs);
	//chnpptr->handler = handler;
	//chnpptr->arg = arg;
	wmb();
	//MSG("(%d) %s:%d setup_dma idx=%d\n", smp_processor_id(), __FILE__, __LINE__, chnpptr->idx);
    debug( "%s:%d setup_dma idx=%d\n", __FILE__, __LINE__, chnpptr->idx );
	//spin_unlock_irqrestore(&chnpptr->lock, flgs);

	/*
	 * decide which dma function to use depending on count
	 */
	if (count == 0) {
		__mbus_setup_dma_void(chnpptr, regbase);
	} else if (count <= MBUS_LINEAR_MAX) {
		__mbus_setup_dma_linear(chnpptr, addr, count, tflags, regbase);
	} else if (count <= (MBUS_LINEAR_MAX * 2)) {
		__mbus_setup_dma_double(chnpptr, addr, MBUS_LINEAR_MAX,
				      addr + MBUS_LINEAR_MAX,
				      count - MBUS_LINEAR_MAX, tflags, regbase);
	} else {
		/*
		 * we need to use rectangle, compute  horiz & lines
		 * values to use
		 */
		for (idx = 0, horiz = 1, sz = count; (idx < 10) && ((sz & 0x01) == 0); ++idx, horiz <<= 1, sz >>= 1)
			;
		lines = count >> idx;
		if ((horiz > MBUS_LINEAR_MAX) || (lines > MBUS_LINEAR_MAX)) {
			//printk(KERN_ERR "MBUS: can't handle rectangle transfer "
		    //   		"of %d bytes (h: %d, v: %d)\n", count, horiz, lines);
			//BUG();
            MSG( "MBUS: can't handle rectangle transfer "
		       		"of %d bytes (h: %d, v: %d)\n", count, horiz, lines);
			return 1;
		}
		__mbus_setup_dma_rectangle(chnpptr, addr, horiz, lines, tflags, regbase);
	}
	return 0;
}

int tangox_mbus_setup_dma(struct mbus_channel_pair *chnpptr,unsigned int addr,
					unsigned int count, mbus_irq_handler_t handler,
					void *arg, unsigned int tflags)
{
	return __tangox_mbus_setup_dma(chnpptr, addr, count, handler, arg, tflags, 0);
}



static void sbox_init(void)
{
	/* Resetting all (or most) channels first */
	int i;
	for (i = 0; i < 2; i++) {
#if defined(CONFIG_TANGO3) || defined(CONFIG_TANGO4) 
#ifdef WITH_MBUS_W1R1
		gbus_write_reg32(REG_BASE_host_interface + SBOX_FIFO_RESET, 0xffffffff);
		iob();
		gbus_write_reg32(REG_BASE_host_interface + SBOX_FIFO_RESET, 0xff00ff00);
#else
		/* Leave W1/R1 alone. */
		gbus_write_reg32(REG_BASE_host_interface + SBOX_FIFO_RESET, 0xfdfdfdfd);
		iob();
		gbus_write_reg32(REG_BASE_host_interface + SBOX_FIFO_RESET, 0xfd00fd00);
#endif
		gbus_write_reg32(REG_BASE_host_interface + SBOX_FIFO_RESET2, 0x03030303);
		iob();
		gbus_write_reg32(REG_BASE_host_interface + SBOX_FIFO_RESET2, 0x03000300);
#else
#ifdef WITH_MBUS_W1R1
		gbus_write_reg32(REG_BASE_host_interface + SBOX_FIFO_RESET, 0x7f7f7f7f);
		iob();
		gbus_write_reg32(REG_BASE_host_interface + SBOX_FIFO_RESET, 0x7f007f00);
#else
		/* Leave W1/R1 alone. */
		gbus_write_reg32(REG_BASE_host_interface + SBOX_FIFO_RESET, 0x7d7d7d7d);
		iob();
		gbus_write_reg32(REG_BASE_host_interface + SBOX_FIFO_RESET, 0x7d007d00);
#endif
#endif
		iob();
		udelay(2); /* at least 256 system cycles, 128MHz or above */
	}

	/* Disconnect all (or most) channels */
#if defined(CONFIG_TANGO4)
#ifdef WITH_MBUS_W1R1
	gbus_write_reg32(REG_BASE_host_interface + SBOX_ROUTE, 0xffffffff);
#else
	gbus_write_reg32(REG_BASE_host_interface + SBOX_ROUTE, 0xffffff0f);
#endif
	gbus_write_reg32(REG_BASE_host_interface + SBOX_ROUTE2, 0x000000ff);
#elif defined(CONFIG_TANGO3)
#ifdef WITH_MBUS_W1R1
	gbus_write_reg32(REG_BASE_host_interface + SBOX_ROUTE, 0xffff4fff);
#else
	gbus_write_reg32(REG_BASE_host_interface + SBOX_ROUTE, 0xffff4f0f);
#endif
	gbus_write_reg32(REG_BASE_host_interface + SBOX_ROUTE2, 0x000000ff);
#else
#ifdef WITH_MBUS_W1R1
	gbus_write_reg32(REG_BASE_host_interface + SBOX_ROUTE, 0xffff4fff);
#else
	gbus_write_reg32(REG_BASE_host_interface + SBOX_ROUTE, 0xffff4f0f);
#endif
#endif
	iob();
#ifdef CONFIG_TANGO2
	MSG("(%d) %s:%d route=0x%x\n", smp_processor_id(), __FILE__, __LINE__, gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE));
#else
	///MSG("(%d) %s:%d route=0x%x%08x\n", smp_processor_id(), __FILE__, __LINE__, gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE2), gbus_read_reg32(REG_BASE_host_interface + SBOX_ROUTE));
    // TODO: write compatible debug() call
#endif
}


int tangox_mbus_init(void)
{
	
	struct mbus_channel_pair *chnpptr = NULL;
	//unsigned long flags;
	int i;

	/* give better MBUS bandwidth for Wx/Rx channels */
#if defined(CONFIG_TANGO3) || defined(CONFIG_TANGO4)
	gbus_write_reg32(REG_BASE_system_block + MARB_mid01_cfg, 0x12005);
	gbus_write_reg32(REG_BASE_system_block + MARB_mid21_cfg, 0x12005);
	gbus_write_reg32(REG_BASE_system_block + MARB_mid03_cfg, 0x12005);
	gbus_write_reg32(REG_BASE_system_block + MARB_mid23_cfg, 0x12005);
#ifdef WITH_MBUS_W1R1
	gbus_write_reg32(REG_BASE_system_block + MARB_mid02_cfg, 0x12005);
	gbus_write_reg32(REG_BASE_system_block + MARB_mid22_cfg, 0x12005);
#endif
#else
	gbus_write_reg32(REG_BASE_system_block + MARB_mid02_cfg, 0x11f1f);
	gbus_write_reg32(REG_BASE_system_block + MARB_mid22_cfg, 0x11f1f);
#endif

	//spin_lock_irqsave(&mbus_lock, flags);

	/* how many channel pairs are available? */
	for (chnpptr = &mchnp_list[0], mbus_free_cnt = i = 0; i < NUM_MBUS_CHNPS; i++, chnpptr++) {
		//spin_lock_init(&chnpptr->lock);
		if (chnpptr->iface == 0xf)
			mbus_free_cnt++; /* count the number of available channel pairs */
	}

	/* reset sbox to default values */
	sbox_init();

#if defined (CONFIG_TANGO4)
	__mbus_void_action = __new_mbus_void;
	__mbus_linear_action = __new_mbus_linear;
	__mbus_double_action = __new_mbus_double;
	__mbus_rectangle_action = __new_mbus_rectangle;
	__mbus_idx2chn = __new_mbus_idx2chn;
	__mbus_reg2chnp = __new_mbus_reg2chnp;
#else
#ifdef CONFIG_TANGO3
	if ((((tangox_chip_id() >> 16) & 0xfffe) == 0x8672) || (((tangox_chip_id() >> 16) & 0xfffe) == 0x8674)) { /* 8672/8674 uses new semantics */
		__mbus_void_action = __new_mbus_void;
		__mbus_linear_action = __new_mbus_linear;
		__mbus_double_action = __new_mbus_double;
		__mbus_rectangle_action = __new_mbus_rectangle;
		__mbus_idx2chn = __new_mbus_idx2chn;
		__mbus_reg2chnp = __new_mbus_reg2chnp;
		/* mbus registers for 8672/8674 is the same as TANGO4 ... */
		mchnp_list[0].rx_base = REG_BASE_host_interface + 0xb100;
		mchnp_list[0].wx_base = REG_BASE_host_interface + 0xb000;
		mchnp_list[1].rx_base = REG_BASE_host_interface + 0xb140;
		mchnp_list[1].wx_base = REG_BASE_host_interface + 0xb040;
		mchnp_list[2].rx_base = REG_BASE_host_interface + 0xb180;
		mchnp_list[2].wx_base = REG_BASE_host_interface + 0xb080;
	} else {
#endif
		__mbus_void_action = __old_mbus_void;
		__mbus_linear_action = __old_mbus_linear;
		__mbus_double_action = __old_mbus_double;
		__mbus_rectangle_action = __old_mbus_rectangle;
		__mbus_idx2chn = __old_mbus_idx2chn;
		__mbus_reg2chnp = __old_mbus_reg2chnp;
#ifdef CONFIG_TANGO3
	}
#endif
#endif

	///printk("registering mbus interrupt routines.\n");
	//mbus_register_intr();

	//spin_unlock_irqrestore(&mbus_lock, flags);

	return 0;
}


int em86xx_mbus_alloc_dma(int iface, int fromdev, unsigned long *pregbase, int *pirq, int canwait)
{
	struct mbus_channel_pair *chnpptr;
    
	if ((chnpptr = tangox_mbus_alloc_dma(iface, fromdev, canwait)) == NULL)
		return -1;

	//if (pirq)
	//	*pirq = (fromdev ? chnpptr->wx_irq : chnpptr->rx_irq);
	if (pregbase)
		*pregbase = chnpptr->fromdev ? chnpptr->wx_base : chnpptr->rx_base;
	
	return 0;
}

/* TODO: TO BE RELPLACED */
void em86xx_mbus_free_dma(unsigned long regbase, int iface)
{
	struct mbus_channel_pair *chnpptr = (*__mbus_reg2chnp)(regbase);

    if (regbase == 0) {
		MSG("%s:%d regbase=0x%x\n", __FILE__, __LINE__, (unsigned int)regbase);
		BUG();
	} else if (chnpptr == NULL) {
		MSG("%s:%d regbase=0x%x\n", __FILE__, __LINE__, (unsigned int)regbase);
		BUG();
	} else if ((regbase != chnpptr->wx_base) && (regbase != chnpptr->rx_base)) {
		MSG("%s:%d regbase=0x%x\n", __FILE__, __LINE__, (unsigned int)regbase);
		BUG();
	}

	tangox_mbus_free_dma(chnpptr, iface);
}


/* TODO: TO BE RELPLACED */
int em86xx_mbus_setup_dma(unsigned int regbase, unsigned int addr,
			  unsigned int count, mbus_irq_handler_t handler,
			  void *arg, unsigned int tflags)
{
	struct mbus_channel_pair *chnpptr = (*__mbus_reg2chnp)(regbase);

	if (regbase == 0) {
		MSG("%s:%d regbase=0x%x\n", __FILE__, __LINE__, regbase);
		BUG();
	} else if (chnpptr == NULL) {
		MSG("%s:%d regbase=0x%x\n", __FILE__, __LINE__, regbase);
		BUG();
	} else if ((regbase != chnpptr->wx_base) && (regbase != chnpptr->rx_base)) {
		MSG("%s:%d regbase=0x%x\n", __FILE__, __LINE__, regbase);
		BUG();
	}

	return tangox_mbus_setup_dma(chnpptr, addr, count, handler, arg, tflags);
}




