#include "stm32f10x.h"
#include <stdint.h>

enum { ADC_WAITING = 0, ADC_OK = 1, ADC_CLOCK_ERROR = 2,
       ADC_STABILIZE_ERROR = 3, ADC_RESET_CAL_TIMEOUT = 4,
       ADC_CAL_TIMEOUT = 5, ADC_CONVERSION_TIMEOUT = 6, ADC_DELAY_ERROR = 7 };
volatile uint32_t g_status = ADC_WAITING;
volatile uint16_t g_adc_raw = 0xFFFFu;
volatile uint32_t g_millivolts = 0u;
volatile uint32_t g_sample_count = 0u;
#define VREF_MV 3300u /* example assumption; verify actual reference */
#define WAIT_LIMIT 2000000u /* iteration bound, not milliseconds */

/* SysTick is used exclusively here for a one-millisecond startup wait. */
static int wait_startup(void)
{
    uint32_t n;
    SystemCoreClockUpdate();
    if (SystemCoreClock < 1000u || (SystemCoreClock / 1000u) > 0x01000000u) return 0;
    SysTick->LOAD = (SystemCoreClock / 1000u) - 1u;
    SysTick->VAL = 0u;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    for (n = 0u; n < WAIT_LIMIT; ++n) {
        if ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0u) {
            SysTick->CTRL = 0u;
            return 1;
        }
    }
    SysTick->CTRL = 0u;
    return 0;
}

static int wait_clear(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t n;
    for (n = 0u; n < WAIT_LIMIT; ++n) if ((*reg & mask) == 0u) return 1;
    return 0;
}

int main(void)
{
    uint32_t n;
    /* ADC clock = PCLK2/8 (9 MHz if PCLK2 is 72 MHz). */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_ADCPRE) | RCC_CFGR_ADCPRE_DIV8;
    GPIOA->CRL &= ~((uint32_t)0xFu); /* PA0 analog input */
    RCC->APB2RSTR |= RCC_APB2RSTR_ADC1RST;
    RCC->APB2RSTR &= ~RCC_APB2RSTR_ADC1RST;
    ADC1->CR1 = 0u;
    ADC1->CR2 = 0u; /* single conversion, right aligned */
    ADC1->SQR1 = 0u; /* sequence length 1 */
    ADC1->SQR3 = 0u; /* channel 0 */
    ADC1->SMPR2 = ADC_SMPR2_SMP0; /* 239.5 ADC cycles */
    ADC1->CR2 = ADC_CR2_EXTSEL | ADC_CR2_EXTTRIG | ADC_CR2_ADON;
    if (!wait_startup()) { g_status = ADC_STABILIZE_ERROR; for (;;) {} }
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    if (!wait_clear(&ADC1->CR2, ADC_CR2_RSTCAL)) { g_status = ADC_RESET_CAL_TIMEOUT; for (;;) {} }
    ADC1->CR2 |= ADC_CR2_CAL;
    if (!wait_clear(&ADC1->CR2, ADC_CR2_CAL)) { g_status = ADC_CAL_TIMEOUT; for (;;) {} }
    for (;;) {
        uint32_t elapsed_ms;
        g_status = ADC_WAITING;
        ADC1->SR = 0u;
        ADC1->CR2 |= ADC_CR2_SWSTART;
        for (n = 0u; n < WAIT_LIMIT; ++n) {
            if ((ADC1->SR & ADC_SR_EOC) != 0u) break;
        }
        if (n == WAIT_LIMIT) { g_status = ADC_CONVERSION_TIMEOUT; for (;;) {} }
        g_adc_raw = (uint16_t)ADC1->DR;
        g_millivolts = ((uint32_t)g_adc_raw * VREF_MV) / 4096u;
        ++g_sample_count;
        g_status = ADC_OK;
        /* Approximate pacing; actual time also includes processing. */
        for (elapsed_ms = 0u; elapsed_ms < 100u; ++elapsed_ms) {
            if (!wait_startup()) { g_status = ADC_DELAY_ERROR; for (;;) {} }
        }
    }
}
