#include "stm32f10x.h"

volatile uint32_t ms_tick = 0u; /* Serviced TIM2 update count, nominal1ms each. */
volatile uint32_t led_state = 0u;
volatile uint32_t toggle_count = 0u;

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
    uint32_t last_toggle = 0u;
    RCC->APB2ENR |= 1u << 3; /* GPIOB clock. */
    (void)RCC->APB2ENR;
    GPIOB->BSRR = 1u << 28; /* PB12 initially LOW, external source LED off. */
    GPIOB->CRH = (GPIOB->CRH & ~(0xFu << 16)) | (0x2u << 16);
    /* PB12: general push-pull, 2MHz output speed grade. */
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
        now = ms_tick; /* One snapshot for this decision. */
        if ((uint32_t)(now - last_toggle) >= 500u) {
            last_toggle = now;
            led_state = 1u - led_state; /*0 and1 exchange. */
            toggle_count++;
        }
        if (led_state != 0u) {
            GPIOB->BSRR = 1u << 12; /* External LED on. */
        } else {
            GPIOB->BSRR = 1u << 28; /* External LED off. */
        }
        /* No delay loop. Late main service toggles once, no catch-up flashes. */
    }
}
