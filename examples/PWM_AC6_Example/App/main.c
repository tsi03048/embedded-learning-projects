#include "stm32f10x.h"

/* Standalone example.
   TIM4 clock nominal 8 MHz: HSI, APB1 divide by 1.
   TIM4 default mapping: CH1 -> PB6.
   TIM4 and PB6 are reserved for this example. */

static void pwm_init(void)
{
    /* Enable GPIOB and TIM4 peripheral clocks. */
    RCC->APB2ENR |= (1u << 3);
    RCC->APB1ENR |= (1u << 2);

    /* Reset entire TIM4: destroys all TIM4 channel settings. */
    RCC->APB1RSTR |=  (1u << 2);
    RCC->APB1RSTR &= ~(1u << 2);

    /* PB6 CRL[27:24] = 1001:
       alternate-function push-pull, 10 MHz output mode. */
    GPIOB->CRL = (GPIOB->CRL & ~(15u << 24)) | (9u << 24);

    TIM4->PSC = 7u;        /* 8 MHz / 8 = 1 MHz */
    TIM4->ARR = 999u;      /* 1000 counter ticks per period */
    TIM4->CCR1 = 300u;     /* 300 ticks high: 30% */

    /* CH1 output, PWM mode 1 (110), CCR1 preload enabled. */
    TIM4->CCMR1 = (6u << 4) | (1u << 3);
    TIM4->CR1 = (1u << 7); /* ARPE=1, up-count, CEN=0 */

    TIM4->EGR = 1u;        /* UG: load PSC / preload values */
    TIM4->SR = 0u;         /* clear flags while initializing */
    TIM4->CCER = 1u;       /* CC1E=1, CC1P=0 active high */
    TIM4->CR1 |= 1u;       /* CEN=1 */
}

int main(void)
{
    pwm_init();

    while (1) {
        /* Timer hardware continues to generate PWM. */
    }
}
