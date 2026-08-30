#include "debug_test.h"
#include "app_config.h"
#include "fan_controller.h"
#include "system_status.h"
#include "system_dfu.h"

#if FEATURE_DEBUG_TEST && !FEATURE_DFU_TEST
static const uint8_t test_duties[] = { 0U, 25U, 50U, 75U, 100U };
static uint8_t test_index;
#elif FEATURE_DEBUG_TEST && FEATURE_DFU_TEST
static uint8_t dfu_test_stage;
#endif
static uint32_t test_started_ms;

void debug_test_init(void) {
#if FEATURE_DEBUG_TEST
#if FEATURE_DFU_TEST
    dfu_test_stage = 0U;
    test_started_ms = system_millis();
#else
    test_index = 0U;
    test_started_ms = system_millis();
    fan_controller_set_duty(test_duties[test_index]);
#endif
#endif
}

void debug_test_update(void) {
#if FEATURE_DEBUG_TEST
#if FEATURE_DFU_TEST
    uint32_t elapsed = system_millis() - test_started_ms;

    if (dfu_test_stage == 0U && elapsed >= 1000UL) {
        fan_controller_set_duty(0U);
        dfu_test_stage = 1U;
    } else if (dfu_test_stage == 1U && elapsed >= 2000UL) {
        fan_controller_set_duty(100U);
        dfu_test_stage = 2U;
    } else if (dfu_test_stage == 2U && elapsed >= 3000UL) {
        fan_controller_set_duty(0U);
        dfu_test_stage = 3U;
    } else if (dfu_test_stage == 3U && elapsed >= 4000UL) {
        system_request_dfu();
    }
#else
    if ((system_millis() - test_started_ms) >= 5000UL) {
        test_index++;
        if (test_index >= (uint8_t)(sizeof(test_duties) / sizeof(test_duties[0]))) {
            test_index = 0U;
        }
        fan_controller_set_duty(test_duties[test_index]);
        test_started_ms = system_millis();
    }
#endif
#endif
}
