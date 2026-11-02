#include "stm32f10x.h"
#include <stdint.h>

/* Assumptions: USART1 PCLK2 nominal 8 MHz, PA9 default TX.
   Standalone example: USART1 and PA9 are reserved. */

/* 0: wait for TXE; 1: wait for TC; 2: finished. */
volatile uint8_t tx_stage = 0u;

static void usart1_tx_init(void)
{
    /* Enable GPIOA and USART1 clocks on APB2. */
    RCC->APB2ENR |= (1u << 2) | (1u << 14);

    /* Reset the whole USART1 peripheral. */
    RCC->APB2RSTR |=  (1u << 14);
    RCC->APB2RSTR &= ~(1u << 14);

    /* PA9 CRH[7:4] = 1001:
       alternate-function push-pull, 10 MHz output. */
    GPIOA->CRH = (GPIOA->CRH & ~(15u << 4)) | (9u << 4);

    /* 8 data, no parity, 1 stop; asynchronous, no flow control. */
    USART1->CR1 = 0u;
    USART1->CR2 = 0u;
    USART1->CR3 = 0u;

    /* 8 MHz / 9600 = 833.33, rounded to 833 = 0x341. */
    USART1->BRR = 0x0341u;

    USART1->CR1 = (1u << 13) | (1u << 3); /* UE, TE */
}

int main(void)
{
    usart1_tx_init();

    while (1) {
        if (tx_stage == 0u) {
            if ((USART1->SR & (1u << 7)) != 0u) { /* TXE */
                USART1->DR = 0x41u; /* ASCII 'A' */
                tx_stage = 1u;
            }
        } else if (tx_stage == 1u) {
            if ((USART1->SR & (1u << 6)) != 0u) { /* TC */
                tx_stage = 2u;
            }
        }
        /* Other short, non-blocking work may run here. */
    }
}
