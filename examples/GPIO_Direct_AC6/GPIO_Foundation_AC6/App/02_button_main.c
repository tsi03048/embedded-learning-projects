#include "stm32f10x.h"

volatile uint32_t raw_button;

int main(void)
{
    /* 1. Enable GPIOC operation. */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    (void)RCC->APB2ENR;

    /* 2. F1 pull selection: ODR0=1 chooses pull-up. */
    GPIOC->BSRR = 1u << 0;

    /* 3. PC0 configuration: CNF0=10, MODE0=00 (input with pull). */
    GPIOC->CRL &= ~(0xFu << 0);
    GPIOC->CRL |= 0x8u << 0;

    while (1) {
        /* Mask PC0 only. Since PC0 is bit 0, result is 0 or 1. */
        raw_button = GPIOC->IDR & (1u << 0);
    }
}
