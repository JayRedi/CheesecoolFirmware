#ifndef SYSTEM_STATUS_H
#define SYSTEM_STATUS_H
#include <stdint.h>
typedef enum { SYSTEM_STATE_BOOT=0, SYSTEM_STATE_FAILSAFE, SYSTEM_STATE_HOST_CONTROLLED, SYSTEM_STATE_FAULT } system_state_t;
typedef struct { system_state_t state; uint8_t mode; uint8_t fan_enabled; uint8_t fan_duty; uint32_t fan_rpm; uint8_t failsafe_active; uint8_t power_fault; uint32_t uptime_ms; } device_status_t;
void system_status_init(void); void system_status_update(void); const device_status_t *system_status_get(void); uint32_t system_millis(void);
#endif
