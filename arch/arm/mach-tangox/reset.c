#include <common.h>
#include <asm/arch/setup.h>

/* reset immediately */
void reset_cpu(ulong addr)
{
    /* configure watchdog */
    gbus_write_uint8(pgbus, REG_BASE_system_block+SYS_watchdog_configuration+3, 0x80);
    gbus_write_uint8(pgbus, REG_BASE_system_block+SYS_watchdog_configuration, 0x01);
    
    /* set watchdog counter */
    gbus_write_uint32(pgbus, REG_BASE_system_block+SYS_watchdog_counter,1);
    
    gbus_write_uint8(pgbus, REG_BASE_system_block+SYS_watchdog_configuration+3, 0x00);
}
