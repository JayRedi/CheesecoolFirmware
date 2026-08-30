#include <stdint.h>

#include <ch32x035.h>

#include "system_status.h"

static volatile uint32_t uptime_ms;

uint32_t system_millis(void)
{
    return uptime_ms;
}

void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void SysTick_Handler(void)
{
    /* CNTIF is cleared by writing zero to SR.  STRE performs the periodic
     * reload; the ISR must not rewrite CMP or CNT. */
    SysTick->SR = 0U;
    uptime_ms++;
}

void system_status_init(void)
{
    uptime_ms = 0U;

    SysTick->SR = 0U;
    SysTick->CNT = 0U;
    SysTick->CMP = (uint64_t)(SystemCoreClock / 1000UL) - 1ULL;
    SysTick->CTLR = 0x0FU;
    NVIC_SetPriority(SysTick_IRQn, 15U);
    NVIC_EnableIRQ(SysTick_IRQn);
}

void system_status_update(void)
{
}

const device_status_t *system_status_get(void)
{
    return (const device_status_t *)0;
}
