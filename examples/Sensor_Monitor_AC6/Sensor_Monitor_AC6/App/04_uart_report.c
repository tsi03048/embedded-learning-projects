#include "stm32f10x.h"

volatile uint32_t adc_ready = 0u;
volatile uint32_t sample_valid = 0u;
volatile uint32_t adc_raw = 0xFFFFu; /* Not a valid 12-bit result. */
volatile uint32_t voltage_mv = 0u; /* Estimate; assumed VREF+=3300mV, VREF-=0. */
volatile uint32_t alarm_state = 0u; /* 0=N(normal), 1=W(warning), valid after sample. */
volatile uint32_t sample_count = 0u;
volatile uint32_t reports_sent = 0u;
volatile uint32_t ms_tick = 0u; /* SERVICED TIM2 updates, nominal1ms each. */
volatile uint32_t sample_tick = 0u; /* Main's request-time tick snapshot. */
volatile uint32_t skipped_periods = 0u; /* Skipped software100tick slots. */

void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & (1u << 0)) != 0u) {
        TIM2->SR = 0u; /* Dedicated update-only TIM2: clear by0. */
        ms_tick++;
    }
}

int main(void)
{
    uint32_t now;
    uint32_t elapsed;
    uint32_t periods;
    uint32_t last_slot = 0u;
    uint32_t i;
    uint8_t report[18] = {'r','a','w','=','0','0','0','0',',','s','t','a','t','e','=','N','\r','\n'};
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

    /* Supplied SystemInit: HSI nominal 8 MHz, AHB /1, APB2 /1.
       USART1 uses PCLK2 directly, NOT the APB timer x2 rule. */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;
    (void)RCC->APB2ENR;
    RCC->APB2RSTR |= RCC_APB2RSTR_USART1RST;
    RCC->APB2RSTR &= ~RCC_APB2RSTR_USART1RST;
    GPIOA->CRH = (GPIOA->CRH & ~(0xFu << 4)) | (0xAu << 4);
    /* PA9: AF push-pull, 2 MHz output speed grade. */

    USART1->CR1 = 0u;    /* UE=0 while configuring; M=0, PCE=0. */
    USART1->CR2 = 0u;    /* STOP=00: one stop bit; CLKEN=0: UART. */
    USART1->CR3 = 0u;    /* DMA, RTS/CTS, half-duplex and EIE disabled. */
    USART1->BRR = 0x0341u; /* 52 + 1/16 -> nominal 9603.84 baud. */
    USART1->CR1 = (1u << 13) | (1u << 3); /* UE, TE; all IRQ off. */

    NVIC->ICER[0] = 1u << 28; /* TIM2 IRQ28. */
    /* Supplied SystemInit: HSI nominal8MHz, APB1 /1 -> TIM2 clock8MHz. */
    RCC->APB1ENR |= 1u << 0;
    (void)RCC->APB1ENR;
    RCC->APB1RSTR |= 1u << 0;
    RCC->APB1RSTR &= ~(1u << 0);
    TIM2->CR1 = 0u;
    TIM2->DIER = 0u;
    TIM2->PSC = 799u; /* 8MHz/(799+1)=10kHz. */
    TIM2->ARR = 9u;   /* Count0..9: 10 counts -> update nominal1ms. */
    TIM2->CNT = 0u;
    TIM2->EGR = 1u << 0; /* UG loads prescaler and initializes time base. */
    while ((TIM2->SR & (1u << 0)) == 0u) { } /* Setup only. */
    TIM2->SR = 0u; /* Discard this setup update before enabling UIE. */
    NVIC->ICPR[0] = 1u << 28;
    TIM2->DIER = 1u << 0;
    NVIC->ISER[0] = 1u << 28;
    TIM2->CR1 |= 1u << 0;

    while (1) {
        now = ms_tick;
        elapsed = (uint32_t)(now - last_slot);
        if (elapsed < 100u) {
            continue; /* Keep looping; no conversion due yet. */
        }
        periods = elapsed / 100u;
        last_slot += periods * 100u; /* Keep nominal software slot phase. */
        skipped_periods += periods - 1u;
        sample_tick = now;
        /* ONE present-time sample. Never fabricate past ADC samples. */
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
        /* ASCII frame: raw=0000,state=N\r\n, exactly18 bytes, no NUL sent. */
        report[4] = (uint8_t)('0' + adc_raw / 1000u);
        report[5] = (uint8_t)('0' + (adc_raw / 100u) % 10u);
        report[6] = (uint8_t)('0' + (adc_raw / 10u) % 10u);
        report[7] = (uint8_t)('0' + adc_raw % 10u);
        report[15] = (alarm_state != 0u) ? 'W' : 'N';
        for (i = 0u; i < 18u; ++i) {
            while ((USART1->SR & (1u << 7)) == 0u) { } /* TXE */
            USART1->DR = report[i];
        }
        while ((USART1->SR & (1u << 6)) == 0u) { } /* TC: final stop sent. */
        reports_sent++;
    }
}
