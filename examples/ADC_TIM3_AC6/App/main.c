#include "stm32f10x.h"
#include <stdint.h>

enum { STARTING=0, RUNNING=1, CLOCK_ERROR=2, STABILIZE_ERROR=3,
       RESET_CAL_TIMEOUT=4, CAL_TIMEOUT=5 };
volatile uint32_t g_status = STARTING;
volatile uint32_t g_sample_count = 0u;
volatile uint32_t g_valid = 0u;
volatile uint16_t g_adc_raw = 0xFFFFu;
#define WAIT_LIMIT 2000000u /* loop count, not calibrated time */

static int wait_one_ms(void)
{
    uint32_t i;
    SysTick->LOAD = (SystemCoreClock / 1000u) - 1u;
    SysTick->VAL = 0u;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    for (i=0u; i<WAIT_LIMIT; ++i) {
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
    uint32_t i;
    for (i=0u; i<WAIT_LIMIT; ++i) if ((*reg & mask) == 0u) return 1;
    return 0;
}

int main(void)
{
    SystemCoreClockUpdate();
    /* This example supports only HCLK=8MHz and APB1/APB2 divisors of 1. */
    if (SystemCoreClock != 8000000u ||
        (RCC->CFGR & (RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) != 0u) {
        g_status = CLOCK_ERROR;
        for (;;) {}
    }
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_ADCPRE) | RCC_CFGR_ADCPRE_DIV8;
    GPIOA->CRL &= ~((uint32_t)0xFu); /* PA0 analog / ADC1 channel 0 */
    RCC->APB2RSTR |= RCC_APB2RSTR_ADC1RST;
    RCC->APB2RSTR &= ~RCC_APB2RSTR_ADC1RST;
    RCC->APB1RSTR |= RCC_APB1RSTR_TIM3RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM3RST;

    /* Load prescaler before ADC triggering is enabled. */
    TIM3->CR1 = 0u;
    TIM3->PSC = 7999u;
    TIM3->ARR = 99u; /* 8 MHz / 8000 / 100 = 10 Hz */
    TIM3->EGR = TIM_EGR_UG;
    TIM3->SR = 0u;
    TIM3->CNT = 0u;
    TIM3->CR2 = TIM_CR2_MMS_1; /* update event -> TRGO */

    ADC1->CR1 = 0u;
    ADC1->CR2 = 0u; /* single conversion; external trigger still disabled */
    ADC1->SQR1 = 0u; /* one regular conversion */
    ADC1->SQR3 = 0u; /* channel 0 */
    ADC1->SMPR2 = ADC_SMPR2_SMP0; /* 239.5 ADC cycles */
    ADC1->CR2 = ADC_CR2_ADON;
    if (!wait_one_ms()) { g_status = STABILIZE_ERROR; for (;;) {} }
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    if (!wait_clear(&ADC1->CR2, ADC_CR2_RSTCAL)) { g_status = RESET_CAL_TIMEOUT; for (;;) {} }
    ADC1->CR2 |= ADC_CR2_CAL;
    if (!wait_clear(&ADC1->CR2, ADC_CR2_CAL)) { g_status = CAL_TIMEOUT; for (;;) {} }
    ADC1->SR = 0u;
    /* ADC1 regular EXTSEL=100 selects TIM3 TRGO. */
    ADC1->CR2 |= ADC_CR2_EXTSEL_2 | ADC_CR2_EXTTRIG;
    g_status = RUNNING; /* armed; does not indicate a completed conversion */
    TIM3->CR1 |= TIM_CR1_CEN;
    for (;;) {
        if ((ADC1->SR & ADC_SR_EOC) != 0u) {
            g_adc_raw = (uint16_t)ADC1->DR; /* reading DR clears EOC */
            ++g_sample_count;
            g_valid = 1u;
        }
    }
}
