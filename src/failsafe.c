#include "failsafe.h"
#include "app_config.h"
#include "fan_controller.h"
#include "system_status.h"

static failsafe_state_t state;
static bool active;
static bool host_ever_seen;
static uint32_t last_activity;
static uint8_t configured_duty;

void failsafe_init(void)
{
    state = FAILSAFE_STATE_BOOT_WAIT;
    active = false;
    host_ever_seen = false;
    last_activity = system_millis();
    failsafe_set_duty(FAILSAFE_DUTY_DEFAULT);
}

void failsafe_host_activity(void)
{
    last_activity = system_millis();
    host_ever_seen = true;
    if (active) {
        active = false;
        state = FAILSAFE_STATE_HOST_ACTIVE;
        /* HOST_CONTROLLED keeps the safe duty until a new host duty arrives;
         * MAX immediately restores its invariant. */
        if (fan_controller_get_mode() == FAN_MODE_MAX) {
            fan_controller_set_duty(100U);
        }
    } else {
        state = FAILSAFE_STATE_HOST_ACTIVE;
    }
}

void failsafe_update(void)
{
    uint32_t now = system_millis();
    uint32_t timeout = host_ever_seen ? HOST_COMM_TIMEOUT_MS : BOOT_CONNECTION_TIMEOUT_MS;

    if (active) {
        /* Safety output has priority over debug tests and other callers. */
        fan_controller_set_duty(failsafe_get_duty());
    } else if ((uint32_t)(now - last_activity) >= timeout) {
        active = true;
        state = FAILSAFE_STATE_FAILSAFE;
        fan_controller_set_duty(failsafe_get_duty());
    } else if (!active) {
        state = host_ever_seen ? FAILSAFE_STATE_HOST_ACTIVE : FAILSAFE_STATE_BOOT_WAIT;
    }
}

bool failsafe_is_active(void) { return active; }
failsafe_state_t failsafe_get_state(void) { return state; }
bool failsafe_host_ever_seen(void) { return host_ever_seen; }
uint8_t failsafe_get_duty(void) { return configured_duty; }

bool failsafe_set_duty(uint8_t duty)
{
    if (duty < FAILSAFE_DUTY_MIN || duty > FAILSAFE_DUTY_MAX) return false;
    configured_duty = duty;
    if (active) fan_controller_set_duty(configured_duty);
    return true;
}
