#include "stm32f10x.h"

enum { PH_IDLE = 0u, PH_WAIT_ADC = 1u, PH_SEND = 2u,
       PH_WAIT_TC = 3u, PH_ERROR = 4u };
enum { ERR_NONE = 0u, ERR_RSTCAL = 1u, ERR_CAL = 2u,
       ERR_ADC_EOC = 3u, ERR_UART_TXE = 4u, ERR_UART_TC = 5u,
       ERR_TIMER_UG = 6u };
volatile uint32_t phase = PH_IDLE;
volatile uint32_t error_stage = ERR_NONE;
volatile uint32_t error_tick = 0u;
volatile uint32_t ms_tick = 0u; /* Serviced TIM2 IRQ count; nominal1ms each. */
volatile uint32_t adc_ready = 0u;
volatile uint32_t sample_valid = 0u; /* Current acquisition result validity. */
volatile uint32_t adc_raw = 0xFFFFu; /* Last accepted result; sentinel at boot. */
volatile uint32_t alarm_state = 0u;
volatile uint32_t sample_count = 0u;
volatile uint32_t skipped_periods = 0u;
volatile uint32_t main_loops = 0u;

volatile uint32_t test_hide_eoc = 1u;

void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & (1u << 0)) != 0u) {
        TIM2->SR = 0u; /* Dedicated update-only TIM2: clear by0. */
        ms_tick++;
    }
}

int main(void)
{
    uint32_t now = 0u;
    uint32_t age = 0u;
    uint32_t observed = 0u;
    uint32_t stage_since = 0u;
    uint32_t last_slot = 0u;
    uint32_t periods;
    uint32_t boot_spins;
    volatile uint32_t settle;
    RCC->APB2ENR |= 1u << 3; /* GPIOB clock. */
    (void)RCC->APB2ENR;
    GPIOB->BSRR = 1u << 28; /* PB12 initially LOW: external LED off. */
    GPIOB->CRH = (GPIOB->CRH & ~(0xFu << 16)) | (0x2u << 16);
    /* PB12: general push-pull, 2MHz speed grade, NOT blink frequency. */
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
    for (boot_spins = 0u; boot_spins < 100000u; ++boot_spins) {
        if ((TIM2->SR & (1u << 0)) != 0u) { break; }
    }
    /* Tick is not running yet: this is a bounded poll count, NOT milliseconds. */
    if (boot_spins == 100000u) {
        error_stage = ERR_TIMER_UG;
        phase = PH_ERROR;
    }
    if (error_stage == ERR_NONE) {
        TIM2->SR = 0u; /* Discard this setup update before enabling UIE. */
        NVIC->ICPR[0] = 1u << 28;
        TIM2->DIER = 1u << 0;
        NVIC->ISER[0] = 1u << 28;
        TIM2->CR1 |= 1u << 0;
    }
    if (error_stage == ERR_NONE) {
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
        stage_since = ms_tick;
        ADC1->CR2 |= 1u << 3; /* RSTCAL; bounded startup wait. */
        do {
            now = ms_tick;
            observed = ADC1->CR2 & (1u << 3);
            age = (uint32_t)(now - stage_since);
        } while ((observed != 0u) && (age < 10u));
        if ((observed != 0u) || (age > 10u)) {
            error_stage = ERR_RSTCAL;
            error_tick = now;
            phase = PH_ERROR;
        }
        if (error_stage == ERR_NONE) {
            stage_since = ms_tick;
            ADC1->CR2 |= 1u << 2; /* CAL; same10tick observation policy. */
            do {
                now = ms_tick;
                observed = ADC1->CR2 & (1u << 2);
                age = (uint32_t)(now - stage_since);
            } while ((observed != 0u) && (age < 10u));
            if ((observed != 0u) || (age > 10u)) {
                error_stage = ERR_CAL;
                error_tick = now;
                phase = PH_ERROR;
            }
        }
        if (error_stage == ERR_NONE) {
            (void)ADC1->DR; /* Discard calibration code, NOT an input sample. */
            ADC1->SR = 0u;
            adc_ready = 1u;
        }
    }
    last_slot = ms_tick; /* Start100tick cadence after initialization. */
    while (1) {
        main_loops++;
        now = ms_tick;
        switch (phase) {
        case PH_IDLE:
            age = (uint32_t)(now - last_slot);
            if (age >= 100u) {
                periods = age / 100u;
                last_slot += periods * 100u;
                skipped_periods += periods - 1u;
                sample_valid = 0u; /* Old raw is NOT the new acquisition. */
                stage_since = now;
                ADC1->CR2 |= 1u << 22;
                phase = PH_WAIT_ADC;
            }
            break;
        case PH_WAIT_ADC:
            age = (uint32_t)(now - stage_since);
            observed = ADC1->SR & (1u << 1); /* Observe EOC once. */
            if (test_hide_eoc != 0u) { observed = 0u; } /* TEST: hide observation only. */
            if ((observed != 0u) && (age <= 10u)) {
                adc_raw = ADC1->DR & 0x0FFFu; /* Accepted result; clears EOC. */
                if (adc_raw >= 2600u) {
                    alarm_state = 1u;
                } else if (adc_raw <= 2400u) {
                    alarm_state = 0u;
                }
                if (alarm_state != 0u) {
                    GPIOB->BSRR = 1u << 12;
                } else {
                    GPIOB->BSRR = 1u << 28;
                }
                sample_count++;
                sample_valid = 1u;
                phase = PH_IDLE;
            } else if (age >= 10u) {
                error_stage = ERR_ADC_EOC;
                error_tick = now;
                phase = PH_ERROR;
                /* Do not read DR, publish old raw, or auto-retry. */
            }
            break;
        case PH_ERROR:
        default:
            /* Latch RAM diagnostics. Keep last LED/raw; no new DR accesses.
               ADC/UART may finish already-started HW work. No auto-recovery. */
            break;
        }
        /* Main regains control after every runtime status check. */
    }
}
