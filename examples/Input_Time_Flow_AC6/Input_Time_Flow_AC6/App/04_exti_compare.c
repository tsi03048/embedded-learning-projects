#include "stm32f10x.h"

volatile uint32_t ms_tick = 0u; /* Serviced TIM2 update count, nominal1ms each. */
volatile uint32_t led_state = 0u;
volatile uint32_t raw_level = 1u;
volatile uint32_t candidate_level = 1u;
volatile uint32_t stable_level = 1u;
volatile uint32_t candidate_since = 0u;
volatile uint32_t sample_gaps = 0u;
volatile uint32_t press_count = 0u;
volatile uint32_t exti_services = 0u; /* Handler services, NOT physical edge count. */

void EXTI0_IRQHandler(void)
{
    if ((EXTI->PR & (1u << 0)) != 0u) {
        EXTI->PR = 1u << 0; /* EXTI pending is cleared by writing1. */
        exti_services++;
        /* No press_count increment, LED toggle or debounce delay here. */
    }
}

void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & (1u << 0)) != 0u) {
        TIM2->SR = 0u; /* Dedicated TIM2 uses update only. Clear status by0. */
        ms_tick++;
    }
}

int main(void)
{
    uint32_t now;
    uint32_t gap;
    uint32_t last_sample = 0u;
    RCC->APB2ENR |= 1u << 3; /* GPIOB clock. */
    (void)RCC->APB2ENR;
    GPIOB->BSRR = 1u << 28; /* PB12 initially LOW, external source LED off. */
    GPIOB->CRH = (GPIOB->CRH & ~(0xFu << 16)) | (0x2u << 16);
    /* PB12: general push-pull, 2MHz output speed grade. */
    RCC->APB2ENR |= 1u << 4; /* GPIOC clock. */
    (void)RCC->APB2ENR;
    GPIOC->BSRR = 1u << 0; /* ODR0=1 selects input pull-up. */
    GPIOC->CRL = (GPIOC->CRL & ~0xFu) | 0x8u;
    /* PC0 switch to GND: released1, pressed0. */
    raw_level = GPIOC->IDR & 1u;
    candidate_level = raw_level;
    stable_level = raw_level; /* Boot level is baseline, NOT a press event. */
    NVIC->ICER[0] = 1u << 6; /* EXTI0 IRQ6. */
    RCC->APB2ENR |= 1u << 0; /* AFIO clock. */
    (void)RCC->APB2ENR;
    EXTI->IMR &= ~(1u << 0);
    AFIO->EXTICR[0] = (AFIO->EXTICR[0] & ~0xFu) | 0x2u; /* PC0 -> EXTI0. */
    EXTI->RTSR |= 1u << 0;
    EXTI->FTSR |= 1u << 0; /* Observe both edges of the same switch. */
    EXTI->PR = 1u << 0;
    NVIC->ICPR[0] = 1u << 6;
    EXTI->IMR |= 1u << 0;
    NVIC->ISER[0] = 1u << 6;

    NVIC->ICER[0] = 1u << 28; /* TIM2 IRQ28. */
    /* Supplied SystemInit: HSI nominal8MHz, APB1 /1 -> TIM2 clock8MHz. */
    RCC->APB1ENR |= 1u << 0;
    (void)RCC->APB1ENR;
    RCC->APB1RSTR |= 1u << 0;
    RCC->APB1RSTR &= ~(1u << 0);
    TIM2->CR1 = 0u;
    TIM2->DIER = 0u;
    TIM2->PSC = 799u; /* 8MHz/(799+1)=10kHz. */
    TIM2->ARR = 9u;   /* Count0..9: 10 counts -> update nominal1ms. */
    TIM2->CNT = 0u;
    TIM2->EGR = 1u << 0; /* UG loads prescaler and initializes time base. */
    while ((TIM2->SR & (1u << 0)) == 0u) { } /* Setup only. */
    TIM2->SR = 0u; /* Discard this setup update before enabling UIE. */
    NVIC->ICPR[0] = 1u << 28;
    TIM2->DIER = 1u << 0;
    NVIC->ISER[0] = 1u << 28;
    TIM2->CR1 |= 1u << 0;

    while (1) {
        now = ms_tick;
        if (now != last_sample) { /* At most one observation at the current tick. */
            raw_level = GPIOC->IDR & 1u;
            gap = (uint32_t)(now - last_sample);
            last_sample = now;
            if ((gap != 1u) || (raw_level != candidate_level)) {
                if (gap != 1u) {
                    sample_gaps++; /* Main missed observations while ticks advanced. */
                }
                candidate_level = raw_level;
                candidate_since = now; /* Restart evidence; NEVER invent old samples. */
            } else if ((candidate_level != stable_level)
                       && ((uint32_t)(now - candidate_since) >= 20u)) {
                stable_level = candidate_level;
                if (stable_level == 0u) { /* Only the confirmed1->0 transition. */
                    press_count++;
                    led_state = 1u - led_state;
                }
            }
        }
        if (led_state != 0u) {
            GPIOB->BSRR = 1u << 12; /* External LED on. */
        } else {
            GPIOB->BSRR = 1u << 28; /* External LED off. */
        }
        /* Hold does not retrigger. Confirm release before the next press. */
    }
}
