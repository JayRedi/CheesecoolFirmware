#include <ch32x035.h>
#include <ch32x035_flash.h>
#include "system_bootloader.h"
#include "fan_controller.h"
void system_enter_bootloader(void)
{
    fan_controller_enable();
    fan_controller_set_duty(100U);
    __disable_irq();
    SystemReset_StartMode(Start_Mode_BOOT);

    /* Clear reset status flags before software reset.
     * RCC->RSTSCKR bit24 = RMVF. */
    RCC->RSTSCKR |= (1UL << 24);

    NVIC_SystemReset();
    while (1) { }
}
