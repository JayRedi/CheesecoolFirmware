#include <ch32x035.h>
#include "system_status.h"
#include "fan_controller.h"
#include "failsafe.h"
#include "power_monitor.h"
static volatile uint32_t uptime_ms; static device_status_t status;
uint32_t system_millis(void) { return uptime_ms; }
void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void) { SysTick->SR=0; uptime_ms++; }
void system_status_init(void) {
    uptime_ms=0;
    SysTick->SR=0;
    SysTick->CNT=0;
    SysTick->CMP=(uint64_t)(SystemCoreClock/1000UL)-1ULL;
    SysTick->CTLR=0x0FU;
    NVIC_SetPriority(SysTick_IRQn, 15U);
    NVIC_EnableIRQ(SysTick_IRQn);
    status.state=SYSTEM_STATE_BOOT;
}
void system_status_update(void) {
    status.mode=(uint8_t)fan_controller_get_mode();
    status.fan_enabled=fan_controller_is_enabled();
    status.fan_duty=fan_controller_get_duty();
    status.fan_rpm=fan_controller_get_rpm();
    status.failsafe_active=failsafe_is_active();
    status.power_fault=power_monitor_has_fault();
    status.uptime_ms=uptime_ms;
    if (status.power_fault) status.state=SYSTEM_STATE_FAULT;
    else if (failsafe_get_state()==FAILSAFE_STATE_BOOT_WAIT) status.state=SYSTEM_STATE_BOOT;
    else if (status.failsafe_active) status.state=SYSTEM_STATE_FAILSAFE;
    else status.state=SYSTEM_STATE_HOST_CONTROLLED;
}
const device_status_t *system_status_get(void) { return &status; }
