#include "gpio.h"
#include "stm32f10x.h"

#define LED_PIN     (1u << 12)
#define BUTTON_PIN  (1u << 0)

void gpio_led_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    (void)RCC->APB2ENR;
    GPIOB->BSRR = LED_PIN << 16; /* prepare LOW before output mode */
    /* PB12: general-purpose push-pull output, maximum speed 2 MHz. */
    GPIOB->CRH = (GPIOB->CRH & ~(0xFu << 16)) | (0x2u << 16);
}

void gpio_led_on(void)
{
    GPIOB->BSRR = LED_PIN;
}

void gpio_led_off(void)
{
    GPIOB->BSRR = LED_PIN << 16;
}

void gpio_button_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    (void)RCC->APB2ENR;
    GPIOC->BSRR = BUTTON_PIN; /* F1 input pull-up: ODR0 = 1 */
    GPIOC->CRL = (GPIOC->CRL & ~0xFu) | 0x8u;
}

uint32_t gpio_button_read(void)
{
    if ((GPIOC->IDR & BUTTON_PIN) == 0u) {
        return 0u;
    }
    return 1u;
}
