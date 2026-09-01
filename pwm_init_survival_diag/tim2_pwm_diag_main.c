#include "ch32x035.h"
#include "ch32x035_gpio.h"
#include "ch32x035_rcc.h"
#include "ch32x035_tim.h"
#include "debug.h"
#include "fan_controller.h"

#define PWM_FREQUENCY_HZ 25000UL

static uint8_t controller_enabled;

static void tim2_pwm_set_duty(uint8_t percent)
{
    uint32_t period = TIM2->ATRLR;

    if(percent > 100U)
    {
        percent = 100U;
    }

    TIM_SetCompare1(TIM2, (uint16_t)((period * (100U - percent)) / 100U));
}

static void tim2_pwm_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    TIM_TimeBaseInitTypeDef base = {0};
    TIM_OCInitTypeDef output = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_0;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    base.TIM_Prescaler = 0U;
    base.TIM_CounterMode = TIM_CounterMode_Up;
    base.TIM_Period = (uint16_t)((SystemCoreClock / PWM_FREQUENCY_HZ) - 1UL);
    base.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &base);

    output.TIM_OCMode = TIM_OCMode_PWM1;
    output.TIM_OutputState = TIM_OutputState_Enable;
    output.TIM_Pulse = base.TIM_Period;
    output.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM2, &output);
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM2, ENABLE);

    /* CH32X035 TIM2 is an advanced-control timer; enable its main output. */
    TIM_CtrlPWMOutputs(TIM2, ENABLE);
    tim2_pwm_set_duty(100U);
    TIM_Cmd(TIM2, ENABLE);
}

void fan_controller_enable(void)
{
    controller_enabled = 1U;
}

void fan_controller_set_duty(uint8_t duty)
{
    if(controller_enabled != 0U)
    {
        tim2_pwm_set_duty(duty);
    }
}

uint32_t fan_tach_get_rpm(void)
{
    return 0U;
}

int main(void)
{
    SystemCoreClockUpdate();
    Delay_Init();
    tim2_pwm_init();

    for(uint8_t round = 0U; round < 2U; ++round)
    {
        tim2_pwm_set_duty(0U);
        Delay_Ms(3000U);
        tim2_pwm_set_duty(25U);
        Delay_Ms(3000U);
        tim2_pwm_set_duty(50U);
        Delay_Ms(3000U);
        tim2_pwm_set_duty(75U);
        Delay_Ms(3000U);
        tim2_pwm_set_duty(100U);
        Delay_Ms(3000U);
    }

    while (1) { }
}
