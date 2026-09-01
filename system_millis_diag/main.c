#include <stdint.h>

#include <ch32x035.h>

#include "fan_controller.h"
#include "system_status.h"

/* fan_controller.c exposes this dependency even though this diagnostic does
 * not initialize or sample the tachometer. */
uint32_t fan_tach_get_rpm(void)
{
    return 0U;
}

int main(void)
{
    SystemCoreClockUpdate();
    system_status_init();
    fan_controller_init();
    fan_controller_enable();

    uint32_t start_ms = system_millis();
    uint8_t stage = 0U;
    fan_controller_set_duty(0U);

    while (1) {
        uint32_t elapsed_ms = system_millis() - start_ms;

        if (stage == 0U && elapsed_ms >= 2000UL) {
            fan_controller_set_duty(100U);
            stage = 1U;
        } else if (stage == 1U && elapsed_ms >= 4000UL) {
            fan_controller_set_duty(0U);
            stage = 2U;
        } else if (stage == 2U && elapsed_ms >= 6000UL) {
            fan_controller_set_duty(100U);
            stage = 3U;
        } else if (stage == 3U && elapsed_ms >= 8000UL) {
            stage = 4U;
        }
    }
}
