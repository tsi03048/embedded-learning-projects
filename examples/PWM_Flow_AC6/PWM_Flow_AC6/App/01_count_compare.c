#include "stm32f10x.h"

volatile uint32_t cnt_snapshot = 0u;
volatile uint32_t expected_active = 0u;


int main(void)
{
    /* Supplied SystemInit keeps HSI nominal 8 MHz, APB1 /1. */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;
    RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;

    TIM2->CR1 = 1u << 7;    /* ARPE=1; stopped, up-count, continuous. */
    TIM2->DIER = 0u;        /* No interrupt or DMA request. */
    TIM2->CCER = 0u;        /* Channels off while changing CC3S. */
    TIM2->CCMR2 = (6u << 4) | (1u << 3);
    /* CC3S=00 output; OC3M=110 PWM1; OC3PE=1 preload. */
    TIM2->PSC = 7999u;
    TIM2->ARR = 999u;
    TIM2->CCR3 = 250u;
    TIM2->CNT = 0u;
    TIM2->EGR = 1u << 0;    /* UG loads PSC, ARR and CCR3 before start. */
    while ((TIM2->SR & (1u << 0)) == 0u) {
        /* Initial UG status; URS=0 and UDIS=0. */
    }
    TIM2->SR = 0u;          /* All flags cleared; TIM2 is used exclusively. */
    TIM2->CR1 |= 1u << 0;

    while (1) {
        cnt_snapshot = TIM2->CNT;
        /* Software prediction from a fixed CCR3, NOT an OC3REF read. */
        if (cnt_snapshot < TIM2->CCR3) {
            expected_active = 1u;
        } else {
            expected_active = 0u;
        }
    }
}
