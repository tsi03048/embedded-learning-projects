#include "stm32f10x.h"

enum { ADC_IDLE=0u, ADC_WAIT=1u };
enum { TX_IDLE=0u, TX_SEND=1u, TX_WAIT_TC=2u };
enum { ERR_NONE=0u, ERR_RSTCAL=1u, ERR_CAL=2u, ERR_ADC_EOC=3u,
       ERR_UART_TXE=4u, ERR_UART_TC=5u, ERR_TIMER_UG=6u };
volatile uint32_t ms_tick=0u; /* Only this variable is written by the ISR. */
volatile uint32_t error_stage=0u, error_tick=0u;
volatile uint32_t adc_ready=0u, sample_valid=0u;
volatile uint32_t adc_raw=0xFFFFu, alarm_state=0u;
volatile uint32_t sample_count=0u, skipped_periods=0u, main_loops=0u;
volatile uint32_t adc_phase=ADC_IDLE, tx_phase=TX_IDLE;
/* Two pending snapshots, separate from the active transmission. */
volatile uint32_t q_raw[2]={0u,0u}, q_alarm[2]={0u,0u}, q_id[2]={0u,0u};
volatile uint32_t q_write=0u, q_read=0u, q_count=0u, q_peak=0u;
volatile uint32_t queued_total=0u, queue_dropped=0u;
volatile uint32_t active_raw=0u, active_alarm=0u, active_id=0u;
volatile uint32_t report_index=0u, reports_confirmed=0u, last_confirmed_id=0u;
/* Demo pacing is software gating, NOT a UART fault or baud-rate change. */
volatile uint32_t demo_byte_gap_ticks=20u;

void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & (1u << 0)) != 0u) {
        TIM2->SR=0u; /* Dedicated update-only timer: clear UIF by0. */
        ms_tick++;
    }
}

int main(void)
{
    uint32_t now=0u, age=0u, observed=0u, stage_since=0u;
    uint32_t last_slot=0u, adc_since=0u, tx_since=0u, pace_since=0u;
    uint32_t gate_open=0u, periods, boot_spins;
    volatile uint32_t settle;
    /* Same21-byte D0 format as S09; this branch does not perform retries. */
    uint8_t report[21]={'D','0',',','r','a','w','=','0','0','0','0',',',
                        's','t','a','t','e','=','N','\r','\n'};
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
        /* error_stage latches startup failure. */
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
            /* error_stage latches startup failure. */
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
                /* error_stage latches startup failure. */
            }
        }
        if (error_stage == ERR_NONE) {
            (void)ADC1->DR; /* Discard calibration code, NOT an input sample. */
            ADC1->SR = 0u;
            adc_ready = 1u;
        }
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
    last_slot=ms_tick;
    while (1) {
        main_loops++;
        now=ms_tick;
        if (error_stage != ERR_NONE) {
            /* Keep RAM/LED/queue and active frame for diagnosis.
               No new conversions, DR writes, dequeue or automatic retry.
               Already-started hardware work may still finish. */
            continue;
        }

        /* A. Acquisition: never wait for the UART to finish. */
        if (adc_phase == ADC_IDLE) {
            age=(uint32_t)(now-last_slot);
            if (age >= 100u) {
                periods=age/100u;
                last_slot+=periods*100u;
                skipped_periods+=periods-1u;
                sample_valid=0u;
                adc_since=now;
                ADC1->CR2 |= 1u << 22; /* SWSTART: one NEW conversion. */
                adc_phase=ADC_WAIT;
            }
        } else {
            age=(uint32_t)(now-adc_since);
            observed=ADC1->SR & (1u << 1);
            if ((observed != 0u) && (age <= 10u)) {
                adc_raw=ADC1->DR & 0x0FFFu;
                if (adc_raw >= 2600u) { alarm_state=1u; }
                else if (adc_raw <= 2400u) { alarm_state=0u; }
                if (alarm_state != 0u) { GPIOB->BSRR=1u << 12; }
                else { GPIOB->BSRR=1u << 28; }
                sample_count++;
                sample_valid=1u;
                /* Copy ONE consistent raw/state/id snapshot into pending RAM. */
                if (q_count < 2u) {
                    q_raw[q_write]=adc_raw;
                    q_alarm[q_write]=alarm_state;
                    q_id[q_write]=sample_count;
                    q_write++;
                    if (q_write == 2u) { q_write=0u; }
                    q_count++;
                    queued_total++;
                    if (q_count > q_peak) { q_peak=q_count; }
                } else {
                    queue_dropped++; /* Drop NEW report, keep measurement/LED. */
                }
                adc_phase=ADC_IDLE;
            } else if (age >= 10u) {
                error_stage=ERR_ADC_EOC;
                error_tick=now;
            }
        }
        /* An acquisition error stops BOTH paths before any further TX action. */
        if (error_stage != ERR_NONE) { continue; }

        /* B. Transmission: one frame, at most one DR write per main pass. */
        switch (tx_phase) {
        case TX_IDLE:
            if (q_count != 0u) {
                active_raw=q_raw[q_read];
                active_alarm=q_alarm[q_read];
                active_id=q_id[q_read];
                q_read++;
                if (q_read == 2u) { q_read=0u; }
                q_count--;
                /* The active snapshot/report is not changed by new samples. */
                report[7]=(uint8_t)('0'+active_raw/1000u);
                report[8]=(uint8_t)('0'+(active_raw/100u)%10u);
                report[9]=(uint8_t)('0'+(active_raw/10u)%10u);
                report[10]=(uint8_t)('0'+active_raw%10u);
                if (active_alarm != 0u) { report[18]='W'; }
                else { report[18]='N'; }
                report_index=0u;
                pace_since=now;
                gate_open=0u;
                tx_phase=TX_SEND;
            }
            break;
        case TX_SEND:
            if (gate_open == 0u) {
                if ((uint32_t)(now-pace_since) < demo_byte_gap_ticks) {
                    break; /* Deliberate demo gap: still run acquisition next pass. */
                }
                tx_since=now; /* TXE budget begins AFTER the deliberate gap. */
                gate_open=1u;
            }
            age=(uint32_t)(now-tx_since);
            observed=USART1->SR & (1u << 7); /* TXE; also SR-read sequence. */
            if ((observed != 0u) && (age <= 50u)) {
                USART1->DR=report[report_index];
                report_index++; /* Submitted to hardware, NOT PC acknowledgment. */
                pace_since=now;
                gate_open=0u;
                if (report_index == 21u) {
                    tx_since=now;
                    tx_phase=TX_WAIT_TC;
                }
            } else if (age >= 50u) {
                error_stage=ERR_UART_TXE;
                error_tick=now;
            }
            break;
        case TX_WAIT_TC:
            age=(uint32_t)(now-tx_since);
            observed=USART1->SR & (1u << 6);
            if ((observed != 0u) && (age <= 50u)) {
                reports_confirmed++;
                last_confirmed_id=active_id;
                tx_phase=TX_IDLE;
            } else if (age >= 50u) {
                error_stage=ERR_UART_TC;
                error_tick=now;
            }
            break;
        default:
            break;
        }
    }
}
