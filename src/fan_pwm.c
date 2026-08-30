#include <ch32x035.h>
#include <ch32x035_gpio.h>
#include <ch32x035_rcc.h>
#include <ch32x035_tim.h>
#include "board_config.h"
#include "app_config.h"
#include "fan_pwm.h"
static uint8_t duty;
void fan_pwm_init(void) { GPIO_InitTypeDef g={0}; TIM_TimeBaseInitTypeDef b={0}; TIM_OCInitTypeDef o={0}; RCC_APB2PeriphClockCmd(FAN_PWM_GPIO_CLOCK,ENABLE); RCC_APB1PeriphClockCmd(FAN_PWM_TIMER_CLOCK,ENABLE); g.GPIO_Pin=FAN_PWM_GPIO_PIN; g.GPIO_Mode=GPIO_Mode_AF_PP; g.GPIO_Speed=GPIO_Speed_50MHz; GPIO_Init(FAN_PWM_GPIO_PORT,&g); b.TIM_Prescaler=0; b.TIM_CounterMode=TIM_CounterMode_Up; b.TIM_Period=(uint16_t)((SystemCoreClock/FAN_PWM_FREQUENCY_HZ)-1UL); b.TIM_ClockDivision=TIM_CKD_DIV1; TIM_TimeBaseInit(FAN_PWM_TIMER,&b); o.TIM_OCMode=TIM_OCMode_PWM1; o.TIM_OutputState=TIM_OutputState_Enable; o.TIM_Pulse=b.TIM_Period; o.TIM_OCPolarity=TIM_OCPolarity_High; TIM_OC1Init(FAN_PWM_TIMER,&o); TIM_OC1PreloadConfig(FAN_PWM_TIMER,TIM_OCPreload_Enable); TIM_ARRPreloadConfig(FAN_PWM_TIMER,ENABLE); TIM_CtrlPWMOutputs(FAN_PWM_TIMER,ENABLE); fan_pwm_set_duty(100U); TIM_Cmd(FAN_PWM_TIMER,ENABLE); }
void fan_pwm_set_duty(uint8_t percent) { uint32_t period=FAN_PWM_TIMER->ATRLR; if(percent>MAX_DUTY_PERCENT) percent=MAX_DUTY_PERCENT; duty=percent; TIM_SetCompare1(FAN_PWM_TIMER,(uint16_t)((period*(100U-percent))/100U)); }
uint8_t fan_pwm_get_duty(void) { return duty; }
