#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H
#include <ch32x035.h>
#include <ch32x035_gpio.h>
#define BOARD_MCU_NAME "CH32X035F8U6"
#define FAN_PWM_GPIO_PORT GPIOA
#define FAN_PWM_GPIO_PIN GPIO_Pin_0
#define FAN_PWM_TIMER TIM2
#define FAN_PWM_GPIO_CLOCK RCC_APB2Periph_GPIOA
#define FAN_PWM_TIMER_CLOCK RCC_APB1Periph_TIM2
#define FAN_TACH_GPIO_PORT GPIOA
#define FAN_TACH_GPIO_PIN GPIO_Pin_1
#define FAN_TACH_GPIO_CLOCK RCC_APB2Periph_GPIOA
#define FAN_TACH_EXTI_LINE EXTI_Line1
#define FAN_TACH_EXTI_IRQn EXTI7_0_IRQn
/* PWR_FAULT pin is not confirmed by the available project hardware data. */
#define PIN_PWR_FAULT 0U
#define PWR_FAULT_ACTIVE_LEVEL 0U
#define USB_BOOTLOADER_SUPPORTED 1U
#endif
