#include "stm32f10x.h"
#include "debounce.h"

#define BUTTON_PIN (1u << 0)
#define EXTI_LINE0 (1u << 0)

volatile uint32_t exti_services = 0u;
volatile uint32_t confirmed_presses = 0u;
volatile uint8_t raw_level = 1u;
volatile uint8_t stable_level = 1u;

void EXTI0_IRQHandler(void)
{
    if ((EXTI->PR & EXTI_LINE0) != 0u) {
        EXTI->PR = EXTI_LINE0;
        ++exti_services;
    }
}

int main(void)
{
    Debounce button;
    NVIC_DisableIRQ(EXTI0_IRQn);
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
    (void)RCC->APB2ENR;
    GPIOC->BSRR = BUTTON_PIN;
    GPIOC->CRL = (GPIOC->CRL & ~0xFu) | 0x8u;
    EXTI->IMR &= ~EXTI_LINE0;
    AFIO->EXTICR[0] = (AFIO->EXTICR[0] & ~0xFu) | 0x2u;
    EXTI->RTSR &= ~EXTI_LINE0;
    EXTI->FTSR |= EXTI_LINE0;
    EXTI->PR = EXTI_LINE0;
    NVIC_ClearPendingIRQ(EXTI0_IRQn);
    raw_level = ((GPIOC->IDR & BUTTON_PIN) != 0u) ? 1u : 0u;
    debounce_init(&button, raw_level);
    stable_level = button.stable;
    SystemCoreClockUpdate();
    SysTick->CTRL = 0u;
    SysTick->LOAD = SystemCoreClock / 1000u - 1u;
    SysTick->VAL = 0u;
    SysTick->CTRL = 5u;
    NVIC_SetPriority(EXTI0_IRQn, 3u);
    EXTI->IMR |= EXTI_LINE0;
    NVIC_EnableIRQ(EXTI0_IRQn);
    for (;;) {
        if ((SysTick->CTRL & (1u << 16)) == 0u) continue;
        raw_level = ((GPIOC->IDR & BUTTON_PIN) != 0u) ? 1u : 0u;
        if (debounce_step(&button, raw_level)) ++confirmed_presses;
        stable_level = button.stable;
    }
}
