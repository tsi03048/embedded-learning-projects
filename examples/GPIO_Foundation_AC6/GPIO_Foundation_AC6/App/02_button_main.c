#include "gpio.h"

volatile uint32_t raw_button;

int main(void)
{
    gpio_button_init();

    while (1) {
        raw_button = gpio_button_read();
    }
}
