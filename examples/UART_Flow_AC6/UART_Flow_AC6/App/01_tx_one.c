#include "stm32f10x.h"

volatile uint32_t tx_submitted = 0u;
volatile uint32_t tx_complete = 0u;

int main(void)
{
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

    while ((USART1->SR & USART_SR_TXE) == 0u) {
        /* Wait until DR can accept the byte. */
    }
    USART1->DR = 'A';
    tx_submitted = 1u;
    while ((USART1->SR & USART_SR_TC) == 0u) {
        /* Wait until the final stop bit is sent. */
    }
    tx_complete = 1u;

    while (1) {
        /* One byte per reset. No repeated messages or software delay. */
    }
}
