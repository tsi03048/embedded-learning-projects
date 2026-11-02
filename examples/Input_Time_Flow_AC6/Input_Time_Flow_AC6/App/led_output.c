#include "stm32f10x.h"
#include "led_output.h"

void led_write(uint32_t on)
{
    if (on != 0u) {
        GPIOB->BSRR = 1u << 12;
    } else {
        GPIOB->BSRR = 1u << 28;
    }
}
