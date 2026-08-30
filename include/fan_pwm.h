#ifndef FAN_PWM_H
#define FAN_PWM_H
#include <stdint.h>
void fan_pwm_init(void); void fan_pwm_set_duty(uint8_t percent); uint8_t fan_pwm_get_duty(void);
#endif
