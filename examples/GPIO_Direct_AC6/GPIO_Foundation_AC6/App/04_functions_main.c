#include "gpio.h"

volatile uint32_t raw_button;

int main(void)
{
    gpio_led_init();
    gpio_button_init();

    while (1) {
        raw_button = gpio_button_read();

        if (raw_button == 0u) {
            gpio_led_on();
        } else {
            gpio_led_off();
        }
    }
}
