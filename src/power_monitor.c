#include "power_monitor.h"
#include "app_config.h"
#include "board_config.h"
#if FEATURE_POWER_FAULT
#include <ch32x035_gpio.h>
#include <ch32x035_rcc.h>
#endif
static bool fault;
void power_monitor_init(void) {
#if FEATURE_POWER_FAULT
    GPIO_InitTypeDef g={0}; RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE); g.GPIO_Pin=PIN_PWR_FAULT; g.GPIO_Mode=GPIO_Mode_IPU; g.GPIO_Speed=GPIO_Speed_50MHz; GPIO_Init(GPIOA,&g);
#endif
    fault=false;
}
void power_monitor_update(void) {
#if FEATURE_POWER_FAULT
    fault=(GPIO_ReadInputDataBit(GPIOA,PIN_PWR_FAULT)==PWR_FAULT_ACTIVE_LEVEL);
#endif
}
bool power_monitor_has_fault(void) { return fault; }
