#include <ch32x035.h>
#include "system_dfu.h"
#include "fan_controller.h"
#include "dfu_config.h"

void system_request_dfu(void)
{
    fan_controller_enable();
    fan_controller_set_duty(100U);
    *(volatile uint32_t *)DFU_BOOT_FLAG_ADDR = DFU_BOOT_MAGIC;
    __asm volatile ("fence rw, rw" ::: "memory");
    NVIC_SystemReset();
    while (1) { }
}
