#include "stm32f10x.h"

volatile uint32_t cnt_snapshot = 0u;
volatile uint32_t duty_request = 250u;
volatile uint32_t duty_written = 250u;


int main(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    (void)RCC->APB2ENR;
    GPIOA->BRR = 1u << 2;
    GPIOA->CRL = (GPIOA->CRL & ~(0xFu << 8)) | (0xAu << 8);
    /* PA2: CNF=10 alternate-function push-pull, MODE=10 (2 MHz).
       Standalone reset project: TIM2_REMAP=00 -> CH3 on PA2.
       Disconnect external USART2 equipment from PA2 before using it. */
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
    TIM2->CCER = 1u << 8;   /* CC3E=1, CC3P=0: active-high output. */
    TIM2->CR1 |= 1u << 0;

    while (1) {
        uint32_t requested;
        cnt_snapshot = TIM2->CNT;
        requested = duty_request;
        if (requested > 1000u) {
            requested = 1000u;
        }
        if (requested != duty_written) {
            TIM2->CCR3 = requested; /* Writes preload, not active value. */
            duty_written = requested; /* Written does NOT mean applied. */
        }
    }
}
