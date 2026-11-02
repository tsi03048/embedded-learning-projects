#include "stm32f10x.h"

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

    /* 4. Set PB12 HIGH: LED ON. For OFF change 12 to 28 here only. */
    GPIOB->BSRR = 1u << 12;

    while (1) {
        /* The output state is held by GPIO hardware. */
    }
}
