#include "debug_test.h"
#include "app_config.h"
#include "fan_controller.h"
#include "system_status.h"
#if FEATURE_DEBUG_TEST
static const uint8_t test_duties[] = { 0U, 25U, 50U, 75U, 100U };
static uint8_t test_index;
static uint32_t test_started_ms;
#endif

void debug_test_init(void) {
#if FEATURE_DEBUG_TEST
    test_index = 0U;
    test_started_ms = system_millis();
    fan_controller_set_duty(test_duties[test_index]);
#endif
}

void debug_test_update(void) {
#if FEATURE_DEBUG_TEST
    if ((system_millis() - test_started_ms) >= 5000UL) {
        test_index++;
        if (test_index >= (uint8_t)(sizeof(test_duties) / sizeof(test_duties[0]))) {
            test_index = 0U;
        }
        fan_controller_set_duty(test_duties[test_index]);
        test_started_ms = system_millis();
    }
#endif
}
