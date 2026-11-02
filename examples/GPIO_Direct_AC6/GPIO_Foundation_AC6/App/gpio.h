#ifndef GPIO_LESSON_H
#define GPIO_LESSON_H

#include <stdint.h>

/* Output: PB12 -> 680 ohm -> red LED -> GND. */
void gpio_led_init(void);
void gpio_led_on(void);
void gpio_led_off(void);

/* Input: PC0 -> switch -> GND. Internal pull-up is enabled. */
void gpio_button_init(void);
uint32_t gpio_button_read(void); /* released = 1, pressed = 0 */

#endif
