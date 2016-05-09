#include <common.h>
#include <asm/io.h>

#define MIPS_COMM_BASEREG       0xf5000000 //0x15000000
#define CMD_MIPS_INTR_MCU_MASK  (1<<3)
#define CMD_MIPS_RES_MCU_MASK   (1<<4)

#define CMD_CTRL_TOTAL_MASK     (0xf<<4)
#define CMD_CTRL_STAMP_MASK     (0xf)
#define CMD_CTRL_ERR_MASK       (1<<4)
#define CMD_CTRL_LEN_MASK       (0xf)

#define CTRLSEG_LEN             (2)
#define DATASEG_LEN             (16 - CTRLSEG_LEN)

#define MIPS_REGFILE_START      0xf5000010
#define MIPS_CTRLSEG_START      MIPS_REGFILE_START
#define MIPS_DATASEG_START      (MIPS_REGFILE_START + CTRLSEG_LEN)

#define MCU_REGFILE_START       0xf5000030
#define MCU_CTRLSEG_START       MCU_REGFILE_START
#define MCU_DATASEG_START       (MCU_REGFILE_START + CTRLSEG_LEN)

#define MAX_PACKAGE_COUNT       (1<<4)  // 4 bit
#define MCU_SET_MIPS_RESET      0x27
#define MCU_COMM_BUFCOUNT_MAX   210


typedef struct _tag_mcu_comm_param{
    unsigned int buf_len;
    unsigned char buffer[MCU_COMM_BUFCOUNT_MAX];
}mcu_comm_param_t;


static void mips_int_mcu_reverse(void)
{
    unsigned char temp;

    temp = readb(MIPS_COMM_BASEREG);
    temp ^= CMD_MIPS_INTR_MCU_MASK; //reverse

    writeb( temp, MIPS_COMM_BASEREG );
}



static int protocol_wrap_puart(void *b1,void *b2,int len)
{
    unsigned char *buf1=b1;
    unsigned char *buf2=b2;
    int  i;

    buf1[0] = 0xff;
    buf1[len+2] = 0xfe;

    memcpy( buf1+1, buf2, len);

    buf1[len+1] = 0;

    for( i=0; i < len; i++ )
        buf1[len+1] += buf2[i];

    buf1[len+1] = (unsigned char)0 - buf1[len+1];

    return len+3;
}

static int send_one_frame(mcu_comm_param_t * param)
{
    unsigned char total, stamp, len, temp;
    unsigned int i;

    total = (param->buf_len + DATASEG_LEN - 1) / DATASEG_LEN;

    if ( total > MAX_PACKAGE_COUNT-1 ) {

        printf("task data is too large! len:%d\n", param->buf_len);
        return -1;
    }

    stamp = 1;  //init value;

    while( stamp <= total )
    {
        //fill in Control Segment
        temp = (total<<4) + stamp;

        writeb( temp, MIPS_CTRLSEG_START );

        len = ( min((int)(param->buf_len - (stamp-1)*DATASEG_LEN), DATASEG_LEN)) & CMD_CTRL_LEN_MASK;

        writeb( len, MIPS_CTRLSEG_START+1 );

        //fill in Data Segment
        for( i=0; i < len; i++ )
        {
            temp = param->buffer[(stamp-1)*DATASEG_LEN + i];
            writeb( temp, MIPS_DATASEG_START+i );
        }

        mips_int_mcu_reverse(); //reverse INIT bit to inform MCU

        stamp ++;
    }
    return 0;
}

static void mcu_send_reset( void )
{
    int len;
    unsigned char msg[5];
    unsigned char tmpbuf[32];

    mcu_comm_param_t mcu_comm_param;


    memset(msg, 0, sizeof(msg));

    msg[0] = MCU_SET_MIPS_RESET;

    len = protocol_wrap_puart(tmpbuf, msg, sizeof(msg));

    mcu_comm_param.buf_len = len;

    memcpy(mcu_comm_param.buffer, tmpbuf, len);

    send_one_frame( &mcu_comm_param );

}

/* reset immediately */
void reset_cpu(ulong addr)
{
    mcu_send_reset();
}
