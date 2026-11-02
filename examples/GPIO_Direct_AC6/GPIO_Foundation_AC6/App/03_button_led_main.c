#include "stm32f10x.h"

volatile uint32_t raw_button;

int main(void)
{
    /* 1. Enable GPIOB operation. */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    (void)RCC->APB2ENR;

    /* 2. Prepare PB12 LOW before selecting output mode. */
    GPIOB->BSRR = 1u << 28;

    /* 3. PB12 configuration: CNF12=00, MODE12=10 (2 MHz output). */
    GPIOB->CRH &= ~(0xFu << 16);
    GPIOB->CRH |= 0x2u << 16;

    /* 1. Enable GPIOC operation. */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    (void)RCC->APB2ENR;

    /* 2. F1 pull selection: ODR0=1 chooses pull-up. */
    GPIOC->BSRR = 1u << 0;

    /* 3. PC0 configuration: CNF0=10, MODE0=00 (input with pull). */
    GPIOC->CRL &= ~(0xFu << 0);
    GPIOC->CRL |= 0x8u << 0;

    while (1) {
        raw_button = GPIOC->IDR & (1u << 0);

        if (raw_button == 0u) {
            GPIOB->BSRR = 1u << 12; /* PB12 HIGH: LED ON */
        } else {
            GPIOB->BSRR = 1u << 28; /* PB12 LOW: LED OFF */
        }
    }
}
