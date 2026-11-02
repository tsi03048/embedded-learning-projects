#include "stm32f10x.h"
#include <stdint.h>

/* Standalone: USART1 PCLK2 nominal 8 MHz.
   PA9 TX / PA10 RX: default USART1 mapping. */

volatile uint32_t rx_count = 0u;
volatile uint32_t tx_count = 0u;
volatile uint32_t drop_count = 0u;
volatile uint32_t error_count = 0u;
volatile uint8_t last_rx = 0u;

static uint8_t pending_byte = 0u;
static uint8_t pending_valid = 0u;

static void usart1_init(void)
{
    RCC->APB2ENR |= (1u << 2) | (1u << 14);
    RCC->APB2RSTR |=  (1u << 14);
    RCC->APB2RSTR &= ~(1u << 14);

    /* PA9 = 1001: AF push-pull, 10 MHz output.
       PA10 = 0100: input floating. */
    GPIOA->CRH = (GPIOA->CRH & ~(0xFFu << 4))
               | (0x49u << 4);

    USART1->CR1 = 0u;
    USART1->CR2 = 0u;
    USART1->CR3 = 0u;
    USART1->BRR = 0x0341u; /* PCLK2 nominal 8 MHz, 9600 baud */

    USART1->CR1 = (1u << 13) | (1u << 3) | (1u << 2);
    /* UE=1, TE=1, RE=1 */
}

static void echo_task(void)
{
    uint32_t status;
    uint8_t data;

    /* One SR snapshot, then DR if RXNE or an error is set. */
    status = USART1->SR;

    if ((status & 0x2Fu) != 0u) { /* RXNE or PE/FE/NE/ORE */
        data = (uint8_t)USART1->DR;

        if ((status & 0x0Fu) != 0u) {
            /* Observed receive error: discard this read. */
            error_count++;
        } else if ((status & (1u << 5)) != 0u) { /* RXNE */
            rx_count++;
            last_rx = data;

            if (pending_valid == 0u) {
                pending_byte = data;
                pending_valid = 1u;
            } else {
                drop_count++;
            }
        }
    }

    /* Do not wait here: return to the main loop if TXE=0. */
    if (pending_valid != 0u) {
        if ((USART1->SR & (1u << 7)) != 0u) { /* TXE */
            USART1->DR = pending_byte;
            pending_valid = 0u;
            tx_count++; /* Submitted to DR, not final stop-bit completion. */
        }
    }
}

int main(void)
{
    usart1_init();

    while (1) {
        echo_task();
        /* Other work here must also finish quickly. */
    }
}
