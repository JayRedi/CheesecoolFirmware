#include "ch32x035.h"
#include "ch32x035_gpio.h"
#include "ch32x035_rcc.h"
#include "debug.h"

int main(void)
{
    GPIO_InitTypeDef gpio = {0};

    SystemCoreClockUpdate();
    Delay_Init();

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_0;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    while (1) {
        GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_SET);
        Delay_Ms(2000);
        GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET);
        Delay_Ms(2000);
    }
}
