#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"

os_timer_t tick_timer;

void tick_cb(void *arg)
{
    os_printf("Tick...\n");
}

void user_init(void)
{
    uart_div_modify(0, UART_CLK_FREQ / 115200);
    os_printf("Hello from ESP8266 NON-OS SDK!\n");

    // Setup periodic 1 second timer
    os_timer_setfn(&tick_timer, tick_cb, NULL);
    os_timer_arm(&tick_timer, 1000, 1);   // 1000ms repeat
}

uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void)
{
    return 1019; // NodeMCU 4MB
}
