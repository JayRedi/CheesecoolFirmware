#include "fan_controller.h"
#include "fan_pwm.h"
#include "fan_tach.h"
#include "app_config.h"
static uint8_t enabled;
static fan_mode_t mode;
static fan_curve_point_t curve[FAN_CURVE_MAX_POINTS];
static uint8_t curve_count;
void fan_controller_init(void) { enabled=1U; mode=FAN_MODE_HOST_CONTROLLED; fan_pwm_init(); fan_pwm_set_duty(POWER_ON_DUTY_PERCENT); }
void fan_controller_update(void) { }
void fan_controller_set_duty(uint8_t d) { fan_pwm_set_duty(enabled?(d>100U?100U:d):0U); }
uint8_t fan_controller_get_duty(void) { return fan_pwm_get_duty(); }
uint32_t fan_controller_get_rpm(void) { return fan_tach_get_rpm(); }
void fan_controller_enable(void) { enabled=1U; }
void fan_controller_disable(void) { enabled=0U; fan_pwm_set_duty(0U); }
bool fan_controller_is_enabled(void) { return enabled!=0U; }
void fan_controller_set_mode(fan_mode_t new_mode) { mode=new_mode; if(new_mode==FAN_MODE_MAX) { fan_controller_enable(); fan_controller_set_duty(100U); } }
fan_mode_t fan_controller_get_mode(void) { return mode; }
bool fan_controller_set_curve(const fan_curve_point_t *points, uint8_t count) {
    if(points==0 || count==0U || count>FAN_CURVE_MAX_POINTS) return false;
    for(uint8_t i=1U;i<count;i++) if(points[i-1U].temperature_c>=points[i].temperature_c) return false;
    for(uint8_t i=0U;i<count;i++) curve[i]=points[i];
    curve_count=count;
    return true;
}
