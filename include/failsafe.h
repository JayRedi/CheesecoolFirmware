#ifndef FAILSAFE_H
#define FAILSAFE_H
#include <stdbool.h>
#include <stdint.h>
typedef enum {
    FAILSAFE_STATE_BOOT_WAIT = 0,
    FAILSAFE_STATE_HOST_ACTIVE,
    FAILSAFE_STATE_FAILSAFE
} failsafe_state_t;
void failsafe_init(void);
void failsafe_update(void);
void failsafe_host_activity(void);
bool failsafe_is_active(void);
failsafe_state_t failsafe_get_state(void);
bool failsafe_host_ever_seen(void);
uint8_t failsafe_get_duty(void);
bool failsafe_set_duty(uint8_t duty);
#endif
