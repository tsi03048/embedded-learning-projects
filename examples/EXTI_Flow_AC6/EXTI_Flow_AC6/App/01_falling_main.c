#include "stm32f10x.h"

volatile uint32_t exti0_services = 0u;
volatile uint32_t main_loops = 0u;

/* The vector table selects this function; main does not call it. */
void EXTI0_IRQHandler(void)
{
    if ((EXTI->PR & (1u << 0)) != 0u) {
        EXTI->PR = 1u << 0;   /* Write 1: clear EXTI0 pending only. */
        exti0_services++;    /* Count services, not button presses. */
    }
}

int main(void)
{
    /* 1. Keep IRQ6 and EXTI0 masked during setup. */
    NVIC->ICER[0] = 1u << 6;

    /* 2. The same PC0 internal pull-up input as the GPIO lesson. */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
    (void)RCC->APB2ENR;
    GPIOC->BSRR = 1u << 0;
    GPIOC->CRL &= ~(0xFu << 0);
    GPIOC->CRL |=  (0x8u << 0);

    EXTI->IMR &= ~(1u << 0);
    EXTI->EMR &= ~(1u << 0);  /* This example uses interrupts. */

    /* 3. EXTICR1 EXTI0[3:0] = 0010: select port C, pin 0. */
    AFIO->EXTICR[0] &= ~(0xFu << 0);
    AFIO->EXTICR[0] |=  (0x2u << 0);

    /* 4. Falling only: released HIGH -> pressed LOW. */
    EXTI->RTSR &= ~(1u << 0);
    EXTI->FTSR |=   1u << 0;

    /* 5. Discard old requests before starting this observation. */
    EXTI->PR = 1u << 0;
    NVIC->ICPR[0] = 1u << 6;

    /* 6. EXTI line 0, then its NVIC IRQ6. CPU mask is reset state. */
    EXTI->IMR |= 1u << 0;
    NVIC->ISER[0] = 1u << 6;

    while (1) {
        main_loops++;        /* Main resumes here after handling. */
    }
}
