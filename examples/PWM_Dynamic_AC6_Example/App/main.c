#include "stm32f10x.h"
#include <stdint.h>

/* TIM3 input clock = 8 MHz, default TIM3 mapping.
   TIM3 and PA6 are reserved for this standalone example. */

volatile uint16_t period_us = 0u;
volatile uint16_t high_us = 0u;
volatile uint32_t duty_percent = 0u;
volatile uint8_t measurement_valid = 0u;
volatile uint32_t discard_count = 0u;

#define CC1_FLAG  (1u << 1)
#define CC2_FLAG  (1u << 2)
#define CC1_OVER  (1u << 9)
#define CC2_OVER  (1u << 10)

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

static void capture_init(void)
{
    RCC->APB2ENR |= (1u << 2); /* GPIOA */
    RCC->APB1ENR |= (1u << 1); /* TIM3 */
    RCC->APB1RSTR |=  (1u << 1);
    RCC->APB1RSTR &= ~(1u << 1);

    /* PA6 input floating, CRL[27:24] = 0100 */
    GPIOA->CRL = (GPIOA->CRL & ~(15u << 24)) | (4u << 24);

    TIM3->PSC = 7u;        /* 1 MHz if TIM3 clock is 8 MHz */
    TIM3->ARR = 65535u;
    TIM3->EGR = 1u;        /* UG: apply PSC */

    /* CC1S=01 and CC2S=10: both observe TI1.
       Input filters off; capture every selected edge. */
    TIM3->CCMR1 = (1u << 0) | (2u << 8);

    /* CH1 rising, CH2 falling. Enable channels after clearing. */
    TIM3->CCER = (1u << 5);
    TIM3->CNT = 0u;
    TIM3->SR = 0u;
    TIM3->CCER |= (1u << 0) | (1u << 4);
    TIM3->CR1 = 1u;        /* up-count, CEN */
}

static void capture_rearm(void)
{
    /* Pause both channels, discard their pending flags. */
    TIM3->CCER &= ~((1u << 0) | (1u << 4));
    TIM3->SR = (uint16_t)~(CC1_FLAG | CC2_FLAG | CC1_OVER | CC2_OVER);
    TIM3->CCER |= (1u << 0) | (1u << 4);
}

volatile uint16_t duty_request = 300u;
volatile uint16_t duty_written = 300u;

static void pwm_request_task(void)
{
    uint16_t requested = duty_request;
    static uint16_t last_written = 300u;
    if (requested < 100u) requested = 100u;
    if (requested > 900u) requested = 900u;
    if (requested != last_written) {
        TIM4->CCR1 = requested;
        last_written = requested;
        duty_written = requested;
    }
}

int main(void)
{
    uint32_t flags;
    uint16_t stamp;
    uint16_t r1 = 0u;
    uint16_t f1 = 0u;
    uint16_t period;
    uint16_t high;
    uint8_t state = 0u; /* 0: wait R1; 1: F1; 2: R2 */

    capture_init(); /* Set PA6 capture first. */
    pwm_init();     /* Then start PB6 output. */

    while (1) {
        pwm_request_task();
        flags = TIM3->SR;

        if (((flags & (CC1_OVER | CC2_OVER)) != 0u) ||
            ((flags & (CC1_FLAG | CC2_FLAG)) ==
                     (CC1_FLAG | CC2_FLAG))) {
            capture_rearm();
            state = 0u;
            measurement_valid = 0u;
            discard_count++;
        } else if ((flags & CC1_FLAG) != 0u) {
            stamp = (uint16_t)TIM3->CCR1; /* clears CC1IF */
            if ((TIM3->SR & (CC1_OVER | CC2_OVER)) != 0u) {
                capture_rearm();
                state = 0u;
                measurement_valid = 0u;
                discard_count++;
            } else if (state == 2u) {
                period = (uint16_t)(stamp - r1);
                high = (uint16_t)(f1 - r1);

                /* Requires a true period < 65536 ticks. */
                if ((period != 0u) && (high < period)) {
                    period_us = period;
                    high_us = high;
                    duty_percent = ((uint32_t)high * 100u) / period;
                    measurement_valid = 1u;
                } else {
                    measurement_valid = 0u;
                    discard_count++;
                }
                r1 = stamp;   /* next period starts here */
                state = 1u;
            } else {
                /* First rising, or a new rising without a falling. */
                r1 = stamp;
                state = 1u;
                measurement_valid = 0u;
            }
        } else if ((flags & CC2_FLAG) != 0u) {
            stamp = (uint16_t)TIM3->CCR2; /* clears CC2IF */
            if ((TIM3->SR & (CC1_OVER | CC2_OVER)) != 0u) {
                capture_rearm();
                state = 0u;
                measurement_valid = 0u;
                discard_count++;
            } else if (state == 1u) {
                f1 = stamp;
                state = 2u;
            } else {
                /* Falling before R1, or unexpected extra falling. */
                state = 0u;
                measurement_valid = 0u;
                discard_count++;
            }
        }
    }
}

