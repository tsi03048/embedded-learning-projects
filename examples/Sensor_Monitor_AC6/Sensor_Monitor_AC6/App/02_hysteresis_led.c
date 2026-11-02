#include "stm32f10x.h"

volatile uint32_t adc_ready = 0u;
volatile uint32_t sample_valid = 0u;
volatile uint32_t adc_raw = 0xFFFFu; /* Not a valid 12-bit result. */
volatile uint32_t voltage_mv = 0u; /* Estimate; assumed VREF+=3300mV, VREF-=0. */
volatile uint32_t alarm_state = 0u; /* 0=N(normal), 1=W(warning), valid after sample. */
volatile uint32_t sample_count = 0u;

int main(void)
{
    RCC->APB2ENR |= 1u << 3; /* GPIOB clock. */
    (void)RCC->APB2ENR;
    GPIOB->BSRR = 1u << 28; /* PB12 initially LOW: external LED off. */
    GPIOB->CRH = (GPIOB->CRH & ~(0xFu << 16)) | (0x2u << 16);
    /* PB12: general push-pull, 2MHz speed grade, NOT blink frequency. */
    volatile uint32_t settle;

    /* Supplied SystemInit: HSI nominal 8 MHz, AHB /1, APB2 /1. */
    RCC->APB2ENR |= (1u << 2) | (1u << 9); /* IOPAEN, ADC1EN */
    (void)RCC->APB2ENR;
    RCC->APB2RSTR |= 1u << 9;  /* Reset ADC1 only. */
    RCC->APB2RSTR &= ~(1u << 9);
    RCC->CFGR &= ~(3u << 14); /* ADCPRE=00: PCLK2 /2 -> 4 MHz. */
    GPIOA->CRL &= ~(0xFu << 16); /* PA4: MODE4=00, CNF4=00, analog. */

    ADC1->CR1 = 0u;  /* Independent, SCAN=0, all ADC IRQ disabled. */
    ADC1->CR2 = 0u;  /* Off while configuring; CONT=0, ALIGN=0, DMA=0. */
    ADC1->SMPR2 = 7u << 12; /* SMP4=111: sample for 239.5 ADC cycles. */
    ADC1->SQR1 = 0u; /* L=0000: ONE regular conversion, not zero. */
    ADC1->SQR2 = 0u;
    ADC1->SQR3 = 4u; /* SQ1=00100: first rank is ADC channel 4 (PA4). */
    ADC1->CR2 = (7u << 17) | (1u << 20) | (1u << 0);
    /* EXTSEL=111 selects SWSTART; EXTTRIG=1 enables that trigger; ADON=1. */

    /* Power ON before calibration: RM0008 Rev21, 11.4.
       1024 NOPs alone take nominal >=128 us at 8 MHz, plus loop overhead.
       This exceeds tSTAB max 1 us and two 4 MHz ADC cycles (0.5 us).
       It is a conservative startup wait, NOT a sampling-period timer. */
    for (settle = 0u; settle < 1024u; ++settle) {
        __NOP();
    }
    ADC1->CR2 |= 1u << 3; /* RSTCAL */
    while ((ADC1->CR2 & (1u << 3)) != 0u) { }
    ADC1->CR2 |= 1u << 2; /* CAL */
    while ((ADC1->CR2 & (1u << 2)) != 0u) { }
    (void)ADC1->DR; /* Calibration code is NOT an input voltage sample. */
    ADC1->SR = 0u; /* Clear flags; this target owns ADC1, no injected/AWD. */
    adc_ready = 1u;

    while (1) {
        ADC1->CR2 |= 1u << 22; /* SWSTART: request ONE conversion. */
        while ((ADC1->SR & (1u << 1)) == 0u) { } /* Wait EOC. */
        adc_raw = ADC1->DR & 0x0FFFu; /* Read result; also clears EOC. */
        voltage_mv = (adc_raw * 3300u) / 4096u;
        if (adc_raw >= 2600u) {
            alarm_state = 1u; /* Upper threshold: enter warning. */
        } else if (adc_raw <= 2400u) {
            alarm_state = 0u; /* Lower threshold: leave warning. */
        }
        /* 2401..2599: keep previous state. Boot policy starts from0. */
        if (alarm_state != 0u) {
            GPIOB->BSRR = 1u << 12;
        } else {
            GPIOB->BSRR = 1u << 28;
        }
        sample_count++;
        sample_valid = 1u;
    }
}
