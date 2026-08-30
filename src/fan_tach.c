#include <ch32x035.h>
#include <ch32x035_gpio.h>
#include <ch32x035_exti.h>
#include <ch32x035_rcc.h>
#include "board_config.h"
#include "app_config.h"
#include "fan_tach.h"
#include "system_status.h"
static volatile uint32_t pulses; static uint32_t last_pulses,last_measurement,rpm;
void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI7_0_IRQHandler(void) { if(EXTI_GetITStatus(FAN_TACH_EXTI_LINE)!=RESET) { pulses++; EXTI_ClearITPendingBit(FAN_TACH_EXTI_LINE); } }
void fan_tach_init(void) { GPIO_InitTypeDef g={0}; EXTI_InitTypeDef e={0}; RCC_APB2PeriphClockCmd(FAN_TACH_GPIO_CLOCK|RCC_APB2Periph_AFIO,ENABLE); g.GPIO_Pin=FAN_TACH_GPIO_PIN; g.GPIO_Mode=GPIO_Mode_IPU; g.GPIO_Speed=GPIO_Speed_50MHz; GPIO_Init(FAN_TACH_GPIO_PORT,&g); GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource1); e.EXTI_Line=FAN_TACH_EXTI_LINE; e.EXTI_Mode=EXTI_Mode_Interrupt; e.EXTI_Trigger=EXTI_Trigger_Falling; e.EXTI_LineCmd=ENABLE; EXTI_Init(&e); NVIC_EnableIRQ(FAN_TACH_EXTI_IRQn); last_measurement=system_millis(); }
void fan_tach_update(void) { uint32_t now=system_millis(), elapsed=now-last_measurement; if(elapsed>=TACH_MEASUREMENT_WINDOW_MS) { uint32_t count=pulses-last_pulses; last_pulses=pulses; last_measurement=now; rpm=(count*60000UL)/(FAN_TACH_PULSES_PER_REV*elapsed); } }
uint32_t fan_tach_get_rpm(void) { return rpm; } uint32_t fan_tach_get_pulse_count(void) { return pulses; }
