#include <common.h>
#include <command.h>
#include <asm/io.h>

#include <asm/armv7.h>
#include <asm/arch/reg_io.h>
#include <mmc.h>

/*
 *  * Board Configuration fields
 *   */
#define BOARD_VERSION		0
#define BOARD_ID                1
#define BOARD_USB_PORT          2
#define BOARD_PANEL_TYPE        3

#define BOARD_MAC_ADDRESS       4
#define BOARD_MAC_ADDRESS_SIZE  6

#define BOARD_FRCX_PANEL        14
#define BOARD_ESN               16
#define BOARD_ESN_SIZE          32

#define BOARD_POTK              48
#define BOARD_POTK_SIZE         16

#define BOARD_ULPK              64
#define BOARD_ULPK_SIZE         96

#define MAX_CONFIG_SIZE         256

#ifdef DEBUG
static void PrintMem(unsigned char *p, int size)
{
        unsigned char *ptr;
        int num =0;
        ptr = p;
        printf("Print mem :");
        while(size>0){
                int i;
                if(size > 4)
                        i = 4;
                else
                        i = size;
                printf("%d :", num);
                num = num + i;
                for(;i>0; i--, ptr++, size-- )
                {
                        printf("0x%04x  ",*ptr);
                }
                printf("\n");
        }
}
#endif

static volatile const unsigned char *board_config = (volatile unsigned char *)CONFIG_SPEC_DDR_ENTRY;

#if 0
/*
 * return board id , which indicate ModelName,Tuner,AMP,Backlight..
 */
static unsigned char do_get_board_id(void)
{
	return board_config[BOARD_ID];
}
#endif

/*
 return :
	1: USB1 for general purpose
	2: USB2 for general purpose
	0: invalid
 */
static unsigned char do_get_usb_port(void)
{
	char *s = getenv("usbport");

	if(s && ((*s == '1') || (*s == '2') || (*s == '3')))
		return (*s-0x30);

	if(board_config[BOARD_USB_PORT] != 1 &&  board_config[BOARD_USB_PORT] != 2){
		printf("invalid usb port num %d.\n",board_config[BOARD_USB_PORT]);
		return 0;
	}
	setenv_ulong("usbport",board_config[BOARD_USB_PORT]);	
	return board_config[BOARD_USB_PORT];
}

static unsigned char do_get_paneltype(void)
{
	unsigned char type = 0;
	char *s = getenv("paneltype");
	if (s) {
		setenv("panel_type", s);
		type = (unsigned char) simple_strtoul(s, NULL, 10);
	} else {
		setenv_ulong("panel_type", board_config[BOARD_PANEL_TYPE]);
		type = board_config[BOARD_PANEL_TYPE];
	}

	/*frcx panel*/
	s = getenv("frcxpanel");
	if (s) {
		setenv("frcx_panel", s);
	} else {
		setenv_ulong("frcx_panel", board_config[BOARD_FRCX_PANEL]);
	}

	return type;
}

#ifdef CONFIG_SYSLOG
/*
 * IO_SHARE 1500ee1a[6:4]
 * Select Function for IO:
 *  0:Host CPU UART1 Serial Transmit Data
 *  1:AV CPU UART Transmit Data
 *  2:Display CPU  Transmit Data
 *  6:General Purpose IO
 *
 * input params: state
 *  0 - log status decided by env 'logstate'
 *  1 - force on
 *  2 - force off
 */
static unsigned char do_set_syslog_state(int state)
{
	if (!state) {
		char *s = getenv("logstate");
		if (s) {
			if (*s == 'y') {
				MWriteRegByte(0x1500ee1a, 0, 0x70);	//[6:4]=3'b000
				CP15ISB;
				printf("turn on syslog\n");
			} else if (*s == 'n') {
				printf("turn off syslog\n");
				CP15DSB;
				MWriteRegByte(0x1500ee1a, 0x60, 0x70);	//[6:4]=3'b110
			}
		}
	} else if (1 == state) {
		//force on
		MWriteRegByte(0x1500ee1a, 0, 0x70);	//[6:4]=3'b000
		CP15ISB;
	} else if (2 == state) {
		//force off
		CP15DSB;
		MWriteRegByte(0x1500ee1a, 0x60, 0x70);	//[6:4]=3'b000
	} else {
		//invalid param
		printf("invalid input param, state=%d\n", state);
		return -1;
	}
	return 0;
}

static unsigned int do_get_syslog_state(void)
{
	return ((ReadRegByte(0x1500ee1a) & 0x70) == 0x00);
}
#endif
/*************************************************
 * TODO
 *************************************************/

void do_get_mac_addr(unsigned char *mac_addr)
{
	int i;
	for(i = 0;i < BOARD_MAC_ADDRESS_SIZE ;i++)
		mac_addr[i] = board_config[BOARD_MAC_ADDRESS + i];

	return;		
}
void do_get_esn(unsigned char *esn)
{
	int i;
	for(i = 0;i < BOARD_ESN_SIZE ;i++)
		esn[0] = board_config[BOARD_ESN + i];

	return;		
}

void do_get_potk(unsigned char *potk)
{
	int i;
	for(i = 0;i < BOARD_POTK_SIZE ;i++)
		potk[0] = board_config[BOARD_POTK + i];

	return;		
}

void do_get_ULPK(unsigned char *ulpk)
{
	int i;
	for(i = 0;i < BOARD_ULPK_SIZE ;i++)
		ulpk[0] = board_config[BOARD_ULPK + i];

	return;		
}


#define MCU_BOOT_MODE_A     0xaa
#define MCU_BOOT_MODE_REG   0xf5000010
#define MCU_POWER_STATE_REG 0xf5000013

#ifdef CONFIG_WAIT_MCU_READY
#define IS_MCU_READY()		(0)	/*TODO*/
static int wait_mcu_ready(void)
{
	char *s;
	int delay, abort=0;

	s = getenv("mcudelay");
	delay = s ? (int)simple_strtoul(s, NULL, 10) : CONFIG_MCU_DELAY;

	printf("Hit any key to stop wait: %2d ", delay);
	/*
	 * Check if key already pressed
	 * Don't check if delay < 0
	 */
	if (delay >= 0) {
		if (IS_MCU_READY()) {
			abort = 1;
		} else if (tstc()) {	/* we got a key press	*/
			(void) getc();  /* consume input	*/
			puts ("\b\b\b 0");
			abort = 2;	/* don't auto boot	*/
		}
	}

	while ((delay > 0) && (!abort)) {
		int i;

		--delay;
		/* delay 100 * 10ms */
		for (i=0; !abort && i<100; ++i) {
			if (IS_MCU_READY()) {
				abort = 1;
				break;
			} else if (tstc()) {	/* we got a key press	*/
				abort  = 2;	/* don't auto boot	*/
				delay = 0;	/* no more delay	*/
				(void) getc();  /* consume input	*/
				break;
			}
			udelay(10000);
		}

		printf("\b\b\b%2d ", delay);
	}

	putc('\n');

	return abort;
}
#endif /*CONFIG_WAIT_MCU_READY*/

static void release_8051(void)
{    
        puts("release MCU \n");        
        writeb(0x0,0xf5000000); //before 8051 fetch code,reset 8051
        writeb(MCU_BOOT_MODE_A, MCU_BOOT_MODE_REG); //tell mcu don't reset ARM
        writeb(0x1,0xf5000000);

#ifdef CONFIG_WAIT_MCU_READY
	/* this is required by SHARP-953 at least in case A9 boots twice from AC ON.
 	 * we shall NOT go ahead until mcu reset us. Otherwise some GPIO jitter will
 	 * be observed during a reset cycle (by mcu).
 	 * however put a delay mechanism here just to avoid mcu crash causing uboot hang:
 	 *   env 'mcudelay' (or CONFIG_MCU_DELAY) specifies seconds will be delayed
 	 *   before go ahead, or press any key to abort wait.
 	 */
	wait_mcu_ready();
#endif
}

static void set_mcu_power_stat(void)
{
	char *s = getenv("mcupower");
	unsigned char mcupower = 0;

	if(s)
		mcupower = (unsigned char)simple_strtoul(s, NULL, 16); 

	printf("mcupower = 0x%x ...\n", mcupower);
	/*
 	 * Below code slices is related with mcu logic. Check with mcu sw owner before change it
 	 * cmd[1] register agreement
 	 *   bit[7] set to 1 for valid power state
 	 *   bit[1] cec power state
 	 *   bit[0] standby power state
 	 */
	mcupower |= 0x80;	//set bit[7], indicates now it's valid value
	writeb(mcupower, MCU_POWER_STATE_REG);
	return ;
}

/*
 * This is for SPI+eMMC or SPI+NAND case
 * SPI preboot can't help load board config due to lack of code size
 * so, load this by self, don't count on others
 */
static void loadcfg_bdcfg_init(void)
{
#if CONFIG_TRIX_MMC
	int err, n, blk_cnt, blk_start;
	struct mmc *mmc;

	blk_cnt = (CONFIG_BDCFG_SIZE / 512);
	blk_start = (CONFIG_BDCFG_OFFSET / 512);

	mmc = find_mmc_device(0);
	if (!mmc) {
		printf("Loadcfg: no mmc device\n");
		return;
	}

	if (mmc_init(mmc))
		return;

	/*Switch bootpart 1, refer to JESD84-B451.pdf section 7.4.47 */
	err =  mmc_switch_part(mmc->block_dev.dev, 1);
	if (err) {
		printf("Loadcfg: swtich to boot partition failed\n");
		return;
	}

	n = mmc->block_dev.block_read(&mmc->block_dev, blk_start, blk_cnt, (void *)board_config);
	if (n != blk_cnt) {
		printf("Loadcfg: read board config failed\n");
	}

	/*Switch back to user partition , refer to JESD84-B451.pdf section 7.4.47 */
	err =  mmc_switch_part(mmc->block_dev.dev, 0);
	if (err) {
		printf("Loadcfg: swtich back to user partition failed\n");
		return;
	}
#else

	printf("Loadcfg: FIXME: Not support load board config from NAND yet\n");

#endif

	return;
}

static int do_loadcfg(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
        if (argc > 1)
                goto usage;

	/*
	 * Load board config just in case preboot doesn't do this.
	 * i.e. in case of SPI boot scenario.
	 */
	loadcfg_bdcfg_init();

#ifdef DEBUG
	PrintMem(board_config,MAX_CONFIG_SIZE);
#endif

	do_get_usb_port();
	do_get_paneltype();
#ifdef CONFIG_SYSLOG
	do_set_syslog_state(0);
#endif

	if (((readb(0xf5000020) & 0x1) == 0) || 
		readb(MCU_BOOT_MODE_REG) == MCU_BOOT_MODE_A)
	{
		//only for ModeA
		//check with bootmcu.c in preboot code
		//TODO: this is only needed in case of AC on.
		// however condition here can't guarantee that as 
		// cmd[0] might be written with the same 
		// magic value by mcu_comm before enter standby, even
		// though it is rare to see. 
		// Whatever who cares we send power stat again in that
		// specific situation... 
		set_mcu_power_stat();
	}

	if((readb(0xf5000020) & 0x1) == 0){
		//in case 8051 in reset state (ModeA without SSC patch)
		release_8051();
	}

	return 0;
usage:
                return CMD_RET_USAGE;
}

U_BOOT_CMD(
	loadcfg, 1,   1,      do_loadcfg,
	"load board specification bytes",
	"\n"
	"    - load board configurate data"
);

#ifdef CONFIG_CMD_SYSLOG
static int parse_argv(const char *s)
{
	if (strcmp(s, "on") == 0) {
                return (1);
        } else if (strcmp(s, "off") == 0) {
                return (2);
        }
        return (-1);
}

static int do_syslog(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	switch (argc) {
	case 2:
		switch (parse_argv(argv[1])) {
		case 1: do_set_syslog_state(1);	/*on*/
			break;
		case 2: do_set_syslog_state(2);	/*off*/
			break;
		default:
			return CMD_RET_USAGE;
		}
		//break;
	case 1:
		printf("syslog state is %s\n", do_get_syslog_state() ? "ON" : "OFF");
		return 0;
	default:
		return CMD_RET_USAGE;
	}
	return 0;
}

U_BOOT_CMD(
	syslog, 2,  1, do_syslog,
	"enable or disable system logs",
	" [on, off] \n"
	"    - enable, disable system logs"
);
#endif
