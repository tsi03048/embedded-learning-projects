/* GPIO lesson for STM32F103VET6, Arm Compiler 5, CMSIS V3.5.0.
 * PB12/PB13: external active-HIGH LEDs, each through 680 ohm to GND.
 * PC0: button to GND, external 10k to 3.3V recommended; internal PU on.
 * Open conflicting LED/SPI2/Touch jumpers; no motor connected.
 */
#include "stm32f10x.h"
#include "debounce.h"
#ifndef LESSON_EXAMPLE
#define LESSON_EXAMPLE 3
#endif
#define LED_READY (1u << 12)
#define LED_RUN   (1u << 13)
#define LED_MASK  (LED_READY | LED_RUN)
#define BUTTON   (1u << 0)
volatile uint32_t g_samples;
volatile uint32_t g_press_count;
volatile uint8_t g_raw, g_stable, g_running;

static void gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;
    (void)RCC->APB2ENR;
    GPIOB->BSRR = LED_MASK << 16; /* Latch LOW before enabling outputs. */
    GPIOB->CRH = (GPIOB->CRH & ~((0xFu << 16) | (0xFu << 20)))
               | (0x2u << 16) | (0x2u << 20); /* PB12/13, PP, 2MHz */
    GPIOC->BSRR = BUTTON; /* ODR bit=1 selects input pull-UP on F1. */
    GPIOC->CRL = (GPIOC->CRL & ~0xFu) | 0x8u; /* PC0: input PU/PD */
}
static uint8_t button_raw(void)
{
    return (GPIOC->IDR & BUTTON) != 0u ? 1u : 0u;
}
static void leds_write(uint32_t high_bits)
{
    /* A single BSRR write updates this group; other PB pins unchanged. */
    GPIOB->BSRR = (high_bits & LED_MASK)
               | ((~high_bits & LED_MASK) << 16);
}
static void tick_init(void)
{
    SystemCoreClockUpdate(); /* This package uses HSI 8MHz, no HSE/PLL. */
    SysTick->CTRL = 0u;
    SysTick->LOAD = SystemCoreClock / 1000u - 1u;
    SysTick->VAL = 0u;
    SysTick->CTRL = 5u; /* ENABLE + processor clock; TICKINT=0 (polling). */
}
static uint8_t tick_elapsed(void)
{
    return (SysTick->CTRL & (1u << 16)) != 0u ? 1u : 0u;
}
int main(void)
{
    Debounce button;
    uint16_t phase = 0u;
    uint8_t blink = 0u;
    gpio_init();
    debounce_init(&button, button_raw());
    tick_init();
    for (;;) {
        if (!tick_elapsed()) continue;
        ++g_samples;
        g_raw = button_raw();
#if LESSON_EXAMPLE == 1
        /* Example 1: PB12 toggles every 500ms; PB13 stays off. */
        if (++phase >= 500u) { phase = 0u; blink ^= 1u; }
        leds_write(blink ? LED_READY : 0u);
#elif LESSON_EXAMPLE == 2
        /* Example 2: raw PC0 LOW turns on PB12 (no debounce). */
        leds_write(g_raw == 0u ? LED_READY : 0u);
#elif LESSON_EXAMPLE == 3
        /* Example 3: each debounced press toggles READY/RUN. */
        if (debounce_step(&button, g_raw)) {
            ++g_press_count;
            g_running ^= 1u;
            phase = 0u;
            blink = 1u;
        }
        g_stable = button.stable;
        if (g_running) {
            if (++phase >= 250u) { phase = 0u; blink ^= 1u; }
            leds_write(blink ? LED_RUN : 0u);
        } else {
            phase = 0u;
            leds_write(LED_READY);
        }
#else
#error LESSON_EXAMPLE must be 1, 2 or 3
#endif
    }
}
