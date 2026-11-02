#include "stm32f10x.h"

enum { PH_IDLE=0u, PH_WAIT_ADC=1u, PH_SEND=2u, PH_WAIT_TC=3u, PH_ERROR=4u,
       PH_ADC_CONFIG=10u, PH_RSTCAL=11u, PH_CAL=12u,
       PH_DRAIN=20u, PH_SYNC_SEND=21u, PH_SYNC_TC=22u };
enum { ERR_NONE=0u, ERR_RSTCAL=1u, ERR_CAL=2u, ERR_ADC_EOC=3u,
       ERR_UART_TXE=4u, ERR_UART_TC=5u, ERR_TIMER_UG=6u,
       ERR_DRAIN=7u, ERR_SYNC_TXE=8u, ERR_SYNC_TC=9u };
volatile uint32_t phase = PH_ADC_CONFIG;
volatile uint32_t error_stage = ERR_NONE;
volatile uint32_t error_tick = 0u;
volatile uint32_t last_error_stage = ERR_NONE;
volatile uint32_t last_error_tick = 0u;
volatile uint32_t retry_request = 0u; /* Set1 in RAM to REQUEST, not force success. */
volatile uint32_t retry_attempts = 0u; /* At most1 per full MCU boot. */
volatile uint32_t retry_denied = 0u;
volatile uint32_t recovery_active = 0u;
volatile uint32_t reinit_completed = 0u; /* Ready again, NOT a fresh sample yet. */
volatile uint32_t generation = 0u; /* 0 initial,1 after one successful reinit. */
volatile uint32_t sample_generation = 0xFFFFFFFFu;
volatile uint32_t ms_tick = 0u;
volatile uint32_t adc_ready = 0u;
volatile uint32_t sample_valid = 0u;
volatile uint32_t adc_raw = 0xFFFFu; /* Last accepted sample; keep for diagnosis. */
volatile uint32_t alarm_state = 0u;
volatile uint32_t sample_count = 0u;
volatile uint32_t skipped_periods = 0u;
volatile uint32_t main_loops = 0u;
volatile uint32_t report_index = 0u;
volatile uint32_t reports_confirmed = 0u;
volatile uint32_t sync_index = 0u;

volatile uint32_t demo_first_stall_after = 4u;
volatile uint32_t demo_hide_drain_tc = 1u;

void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & (1u << 0)) != 0u) {
        TIM2->SR = 0u; /* Update-only timer. Clear UIF by0. */
        ms_tick++;
    }
}

int main(void)
{
    uint32_t now = 0u, age = 0u, observed = 0u;
    uint32_t stage_since = 0u, last_slot = 0u, request;
    uint32_t periods, boot_spins;
    volatile uint32_t settle;
    uint8_t report[21] = {'D','0',',','r','a','w','=','0','0','0','0',',','s','t','a','t','e','=','N','\r','\n'};
    const uint8_t sync_bytes[9] = {'\r','\n','!','S','Y','N','C','\r','\n'};
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
        recovery_active = 0u; phase = PH_ERROR;
    }
    if (error_stage == ERR_NONE) {
        TIM2->SR = 0u; /* Discard this setup update before enabling UIE. */
        NVIC->ICPR[0] = 1u << 28;
        TIM2->DIER = 1u << 0;
        NVIC->ISER[0] = 1u << 28;
        TIM2->CR1 |= 1u << 0;
    }
    if (error_stage == ERR_NONE) {
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
    
    }
    while (1) {
        main_loops++;
        now = ms_tick;
        request = retry_request;
        retry_request = 0u; /* Consume; requests during active work are not queued. */
        if (request != 0u) {
            if ((phase == PH_ERROR) && (retry_attempts == 0u)
                && ((error_stage == ERR_ADC_EOC) || (error_stage == ERR_UART_TXE) || (error_stage == ERR_UART_TC))) {
                last_error_stage = error_stage;
                last_error_tick = error_tick;
                retry_attempts++;
                recovery_active = 1u;
                sample_valid = 0u;
                adc_ready = 0u;
                stage_since = now;
                phase = PH_DRAIN;
            } else {
                retry_denied++; /* Includes boot faults and second retries. */
            }
        }
        switch (phase) {
        case PH_ADC_CONFIG:
            sample_valid = 0u;
            adc_ready = 0u;
            /* Supplied SystemInit: HSI nominal 8 MHz, AHB /1, APB2 /1. */
            RCC->APB2ENR |= (1u << 2) | (1u << 9); /* IOPAEN, ADC1EN */
            (void)RCC->APB2ENR;
            ADC1->CR2 = 0u; /* ADON0: power off old conversion path before reset. */
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
            ADC1->CR2 |= 1u << 3;
            phase = PH_RSTCAL;
            break;

        case PH_RSTCAL:
            age = (uint32_t)(now - stage_since);
            observed = ADC1->CR2 & (1u << 3);
            if ((observed == 0u) && (age <= 10u)) {
                stage_since = now;
                ADC1->CR2 |= 1u << 2;
                phase = PH_CAL;
            } else if (age >= 10u) {
                error_stage = ERR_RSTCAL; error_tick = now; recovery_active = 0u; phase = PH_ERROR;
            }
            break;

        case PH_CAL:
            age = (uint32_t)(now - stage_since);
            observed = ADC1->CR2 & (1u << 2);
            if ((observed == 0u) && (age <= 10u)) {
                (void)ADC1->DR; /* Calibration code only. Never publish it. */
                ADC1->SR = 0u;
                adc_ready = 1u;
                if (recovery_active != 0u) {
                    generation++; /* Retry cap means this becomes1 only. */
                    reinit_completed++;
                    recovery_active = 0u;
                }
                error_stage = ERR_NONE;
                last_slot = now;
                phase = PH_IDLE; /* sample_valid still0 until a NEW conversion. */
            } else if (age >= 10u) {
                error_stage = ERR_CAL; error_tick = now; recovery_active = 0u; phase = PH_ERROR;
            }
            break;
        case PH_IDLE:
            age = (uint32_t)(now - last_slot);
            if (age >= 100u) {
                periods = age / 100u;
                last_slot += periods * 100u;
                skipped_periods += periods - 1u;
                sample_valid = 0u;
                stage_since = now;
                ADC1->CR2 |= 1u << 22;
                phase = PH_WAIT_ADC;
            }
            break;

        case PH_WAIT_ADC:
            age = (uint32_t)(now - stage_since);
            observed = ADC1->SR & (1u << 1);
            if ((observed != 0u) && (age <= 10u)) {
                adc_raw = ADC1->DR & 0x0FFFu;
                sample_generation = generation;
                if (adc_raw >= 2600u) { alarm_state = 1u; }
                else if (adc_raw <= 2400u) { alarm_state = 0u; }
                if (alarm_state != 0u) { GPIOB->BSRR = 1u << 12; }
                else { GPIOB->BSRR = 1u << 28; }
                sample_count++;
                sample_valid = 1u;
                report[1] = (uint8_t)('0' + generation); /* Only0 or1 in this lesson. */
                report[7] = (uint8_t)('0' + adc_raw / 1000u);
                report[8] = (uint8_t)('0' + (adc_raw / 100u) % 10u);
                report[9] = (uint8_t)('0' + (adc_raw / 10u) % 10u);
                report[10] = (uint8_t)('0' + adc_raw % 10u);
                if (alarm_state != 0u) { report[18] = 'W'; }
                else { report[18] = 'N'; }
                report_index = 0u;
                stage_since = now;
                phase = PH_SEND;
            } else if (age >= 10u) {
                error_stage = ERR_ADC_EOC; error_tick = now; recovery_active = 0u; phase = PH_ERROR;
            }
            break;
        case PH_SEND:
            age = (uint32_t)(now - stage_since);
            observed = USART1->SR & (1u << 7);
            if ((retry_attempts == 0u) && (report_index >= demo_first_stall_after)) { observed = 0u; }
            if ((observed != 0u) && (age <= 50u)) {
                USART1->DR = report[report_index];
                report_index++;
                stage_since = now;
                if (report_index == 21u) { phase = PH_WAIT_TC; }
            } else if (age >= 50u) {
                error_stage = ERR_UART_TXE; error_tick = now; recovery_active = 0u; phase = PH_ERROR;
            }
            break;

        case PH_WAIT_TC:
            age = (uint32_t)(now - stage_since);
            observed = USART1->SR & (1u << 6);
            if ((observed != 0u) && (age <= 50u)) {
                reports_confirmed++;
                phase = PH_IDLE;
            } else if (age >= 50u) {
                error_stage = ERR_UART_TC; error_tick = now; recovery_active = 0u; phase = PH_ERROR;
            }
            break;

        case PH_DRAIN:
            age = (uint32_t)(now - stage_since);
            observed = USART1->SR & (1u << 6);
            if (demo_hide_drain_tc != 0u) { observed = 0u; }
            /* No USART_DR writes here. Finish already-submitted bytes first. */
            if ((observed != 0u) && (age <= 50u)) {
                sync_index = 0u;
                stage_since = now;
                phase = PH_SYNC_SEND;
            } else if (age >= 50u) {
                error_stage = ERR_DRAIN; error_tick = now; recovery_active = 0u; phase = PH_ERROR;
            }
            break;

        case PH_SYNC_SEND:
            age = (uint32_t)(now - stage_since);
            observed = USART1->SR & (1u << 7);
            if ((observed != 0u) && (age <= 50u)) {
                USART1->DR = sync_bytes[sync_index];
                sync_index++;
                stage_since = now;
                if (sync_index == 9u) { phase = PH_SYNC_TC; }
            } else if (age >= 50u) {
                error_stage = ERR_SYNC_TXE; error_tick = now; recovery_active = 0u; phase = PH_ERROR;
            }
            break;

        case PH_SYNC_TC:
            age = (uint32_t)(now - stage_since);
            observed = USART1->SR & (1u << 6);
            if ((observed != 0u) && (age <= 50u)) {
                phase = PH_ADC_CONFIG; /* Only after marker really finishes. */
            } else if (age >= 50u) {
                error_stage = ERR_SYNC_TC; error_tick = now; recovery_active = 0u; phase = PH_ERROR;
            }
            break;
        case PH_ERROR:
        default:
            /* Keep last raw/LED/diagnostics. No autonomous recovery or DR access. */
            break;
        }
    }
}
