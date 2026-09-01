#include "ch32x035.h"
#include "ch32x035_gpio.h"
#include "ch32x035_rcc.h"
#include "debug.h"
#include "debug_test.h"
#include "failsafe.h"
#include "fan_controller.h"
#include "fan_tach.h"
#include "power_monitor.h"
#include "system_status.h"
#include "usb_device.h"

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

static void diag_pulse(void)
{
    diag_gpio_write(Bit_SET);
    Delay_Ms(300);
    diag_gpio_write(Bit_RESET);
    Delay_Ms(300);
}

static void controller_diag_pulse(void)
{
    fan_controller_enable();
    fan_controller_set_duty(0U);
    Delay_Ms(500);
    fan_controller_set_duty(100U);
    Delay_Ms(500);
    Delay_Ms(300);
}

static void diag_startup_pattern(void)
{
    diag_gpio_write(Bit_SET);
    Delay_Ms(2000);
    diag_gpio_write(Bit_RESET);
    Delay_Ms(2000);
}

void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void NMI_Handler(void)
{
    while (1) {
        diag_gpio_write(Bit_SET);
        Delay_Ms(500);
        diag_gpio_write(Bit_RESET);
        Delay_Ms(500);
    }
}

void HardFault_Handler(void)
{
    while (1) {
        diag_gpio_write(Bit_SET);
        Delay_Ms(100);
        diag_gpio_write(Bit_RESET);
        Delay_Ms(100);
    }
}

int main(void)
{
    SystemCoreClockUpdate();
    Delay_Init();
    diag_gpio_init();

    diag_startup_pattern();

    system_status_init();
    diag_pulse();

    fan_controller_init();
    controller_diag_pulse();

    fan_tach_init();
    controller_diag_pulse();

    power_monitor_init();
    controller_diag_pulse();

    failsafe_init();
    controller_diag_pulse();

    usb_device_init();
    controller_diag_pulse();

    debug_test_init();
    controller_diag_pulse();

    fan_controller_enable();
    fan_controller_set_duty(0U);
    Delay_Ms(2000);
    fan_controller_set_duty(100U);
    Delay_Ms(2000);
    fan_controller_set_duty(0U);
    Delay_Ms(2000);
    fan_controller_set_duty(100U);
    Delay_Ms(2000);

    while (1) { }
}
