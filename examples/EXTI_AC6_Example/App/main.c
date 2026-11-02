#include "stm32f10x.h"

#define BUTTON_PIN  (1u << 0)
#define EXTI_LINE0  (1u << 0)

/* Counts handler services, not confirmed button presses. */
volatile unsigned long exti0_services = 0ul;

void EXTI0_IRQHandler(void)
{
    if ((EXTI->PR & EXTI_LINE0) != 0u) {
        EXTI->PR = EXTI_LINE0;  /* W1C: clear EXTI0 only */
        exti0_services++;
    }
}

int main(void)
{
    /* 1. Keep this IRQ disabled during initialization. */
    NVIC_DisableIRQ(EXTI0_IRQn);

    /* 2. Enable GPIOC and AFIO clocks. */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
    (void)RCC->APB2ENR;

    /* 3. PC0: input with internal pull-up. */
    GPIOC->BSRR = BUTTON_PIN;  /* ODR0 = 1: pull-up */
    GPIOC->CRL = (GPIOC->CRL & ~0xFu) | 0x8u;

    /* 4. Mask EXTI0 while configuring its route and edge. */
    EXTI->IMR &= ~EXTI_LINE0;

    /* EXTI0[3:0] = 0010: route PC0 to EXTI0. */
    AFIO->EXTICR[0] = (AFIO->EXTICR[0] & ~0xFu) | 0x2u;

    /* 5. Falling edge only. */
    EXTI->RTSR &= ~EXTI_LINE0;
    EXTI->FTSR |= EXTI_LINE0;

    /* 6. Discard stale pending state, then enable. */
    EXTI->PR = EXTI_LINE0;
    NVIC_ClearPendingIRQ(EXTI0_IRQn);
    NVIC_SetPriority(EXTI0_IRQn, 3u);
    EXTI->IMR |= EXTI_LINE0;
    NVIC_EnableIRQ(EXTI0_IRQn);

    while (1) {
        /* Observe exti0_services in debugger. */
    }
}
