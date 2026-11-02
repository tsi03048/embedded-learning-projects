#include "stm32f10x.h"

volatile unsigned long timer_services = 0ul;
volatile unsigned long press_count = 0ul;
volatile unsigned char raw_high = 1u;
volatile unsigned char stable_high = 1u;
static unsigned char different_count = 0u;

void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & TIM_SR_UIF) != 0u) {
        TIM2->SR = (unsigned short)~TIM_SR_UIF;
        ++timer_services;
        raw_high = ((GPIOC->IDR & 1u) != 0u) ? 1u : 0u;
        if (raw_high == stable_high) {
            different_count = 0u;
        } else if (++different_count >= 20u) {
            different_count = 0u;
            stable_high = raw_high;
            if (stable_high == 0u) ++press_count;
        }
    }
}

int main(void)
{
    NVIC_DisableIRQ(TIM2_IRQn);
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    (void)RCC->APB2ENR;
    GPIOC->BSRR = 1u;
    GPIOC->CRL = (GPIOC->CRL & ~0xFu) | 0x8u;
    stable_high = ((GPIOC->IDR & 1u) != 0u) ? 1u : 0u;
    raw_high = stable_high;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;
    RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;
    TIM2->PSC = 7u;
    TIM2->ARR = 999u;
    TIM2->CNT = 0u;
    TIM2->EGR = TIM_EGR_UG;
    while ((TIM2->SR & TIM_SR_UIF) == 0u) { }
    TIM2->SR = (unsigned short)~TIM_SR_UIF;
    NVIC_ClearPendingIRQ(TIM2_IRQn);
    NVIC_SetPriority(TIM2_IRQn, 3u);
    TIM2->DIER |= TIM_DIER_UIE;
    NVIC_EnableIRQ(TIM2_IRQn);
    TIM2->CR1 |= TIM_CR1_CEN;
    while (1) { }
}
