#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H
#include <stdint.h>
#include <stdbool.h>
#define FAN_CURVE_MAX_POINTS 29U
typedef struct { uint8_t temperature_c; uint8_t duty_percent; } fan_curve_point_t;
typedef enum { FAN_MODE_HOST_CONTROLLED=0, FAN_MODE_MAX=1 } fan_mode_t;
void fan_controller_init(void); void fan_controller_update(void); void fan_controller_set_duty(uint8_t duty); uint8_t fan_controller_get_duty(void); uint32_t fan_controller_get_rpm(void); void fan_controller_enable(void); void fan_controller_disable(void); bool fan_controller_is_enabled(void);
void fan_controller_set_mode(fan_mode_t mode); fan_mode_t fan_controller_get_mode(void);
bool fan_controller_set_curve(const fan_curve_point_t *points, uint8_t count);
#endif
