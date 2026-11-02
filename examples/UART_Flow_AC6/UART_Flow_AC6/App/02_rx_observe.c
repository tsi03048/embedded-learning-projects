#include "stm32f10x.h"

volatile uint32_t rx_count = 0u;
volatile uint32_t rx_error_events = 0u;
volatile uint32_t last_rx = 0u;

int main(void)
{
    /* Supplied SystemInit: HSI nominal 8 MHz, AHB /1, APB2 /1.
       USART1 uses PCLK2 directly, NOT the APB timer x2 rule. */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;
    (void)RCC->APB2ENR;
    RCC->APB2RSTR |= RCC_APB2RSTR_USART1RST;
    RCC->APB2RSTR &= ~RCC_APB2RSTR_USART1RST;
    GPIOA->BSRR = 1u << 10; /* ODR10=1 selects input pull-up. */
    GPIOA->CRH = (GPIOA->CRH & ~(0xFu << 8)) | (0x8u << 8);
    /* PA10: input pull-up, NOT AF output. */

    USART1->CR1 = 0u;    /* UE=0 while configuring; M=0, PCE=0. */
    USART1->CR2 = 0u;    /* STOP=00: one stop bit; CLKEN=0: UART. */
    USART1->CR3 = 0u;    /* DMA, RTS/CTS, half-duplex and EIE disabled. */
    USART1->BRR = 0x0341u; /* 52 + 1/16 -> nominal 9603.84 baud. */
    USART1->CR1 = (1u << 13) | (1u << 2); /* UE, RE; all IRQ off. */

    while (1) {
        uint32_t status;
        uint8_t received;
        status = USART1->SR; /* One status snapshot BEFORE reading DR. */
        if ((status & (USART_SR_RXNE | USART_SR_ORE | USART_SR_NE | USART_SR_FE)) != 0u) {
            received = (uint8_t)USART1->DR; /* Read clears RXNE; SR->DR clears errors. */
            if ((status & (USART_SR_ORE | USART_SR_NE | USART_SR_FE)) != 0u) {
                rx_error_events++; /* Discard this read if any observed error. */
            } else if ((status & USART_SR_RXNE) != 0u) {
                last_rx = received;
                rx_count++;
            }
        }
    }
}
