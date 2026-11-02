#include "gpio.h"

int main(void)
{
    gpio_led_init();
    gpio_led_on();

    while (1) {
        /* The output hardware keeps the LED on. */
    }
}
