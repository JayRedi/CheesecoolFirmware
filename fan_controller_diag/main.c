#include "ch32x035.h"
#include "debug.h"
#include "fan_controller.h"
#include "system_dfu.h"

/* fan_controller.c exposes this getter even though TACH is not part of this
 * isolated diagnostic. Keep the diagnostic independent of fan_tach.c. */
uint32_t fan_tach_get_rpm(void)
{
    return 0U;
}

static void run_duty_cycle(void)
{
    fan_controller_set_duty(0U);
    Delay_Ms(3000);
    fan_controller_set_duty(25U);
    Delay_Ms(3000);
    fan_controller_set_duty(50U);
    Delay_Ms(3000);
    fan_controller_set_duty(75U);
    Delay_Ms(3000);
    fan_controller_set_duty(100U);
    Delay_Ms(3000);
}

int main(void)
{
    SystemCoreClockUpdate();
    Delay_Init();

    fan_controller_init();
    fan_controller_enable();

    run_duty_cycle();
    run_duty_cycle();

    system_request_dfu();
}
