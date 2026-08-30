#ifndef FAN_TACH_H
#define FAN_TACH_H
#include <stdint.h>
void fan_tach_init(void); void fan_tach_update(void); uint32_t fan_tach_get_rpm(void); uint32_t fan_tach_get_pulse_count(void);
#endif
