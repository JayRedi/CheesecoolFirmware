#include "ch32x035.h"
#include "ch32x035_gpio.h"
#include "ch32x035_rcc.h"
#include "debug.h"
#include "fan_controller.h"
#include "fan_pwm.h"
#include "system_dfu.h"

volatile uint32_t pwm_diag_stage;

/* fan_controller.c exposes this getter, but TACH is intentionally absent. */
uint32_t fan_tach_get_rpm(void)
{
    return 0U;
}

static void diag_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_0;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
}

static void diag_gpio_write(BitAction state)
{
    diag_gpio_init();
    GPIO_WriteBit(GPIOA, GPIO_Pin_0, state);
}

void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void NMI_Handler(void)
{
    pwm_diag_stage = 0xF2U;
    while (1) { }
}

void HardFault_Handler(void)
{
    pwm_diag_stage = 0xF1U;
    while (1) { }
}

int main(void)
{
    SystemCoreClockUpdate();
    Delay_Init();

    pwm_diag_stage = 0xA1U;
    diag_gpio_write(Bit_SET);
    Delay_Ms(2000);
    diag_gpio_write(Bit_RESET);
    Delay_Ms(2000);

    pwm_diag_stage = 0xA2U;
    fan_pwm_init();
    pwm_diag_stage = 0xA3U;
    Delay_Ms(2000);

    fan_pwm_set_duty(0U);
    pwm_diag_stage = 0xA4U;
    Delay_Ms(2000);

    fan_pwm_set_duty(100U);
    pwm_diag_stage = 0xA5U;
    Delay_Ms(2000);

    fan_pwm_set_duty(50U);
    pwm_diag_stage = 0xA6U;
    Delay_Ms(2000);

    pwm_diag_stage = 0xA7U;
    system_request_dfu();
}
