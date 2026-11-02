#include "stm32f10x.h"
#include <stdint.h>

/* Standalone example: USART1 PCLK2 nominal 8 MHz,
   default PA10 RX, 9600 8N1, single main-loop consumer. */

volatile uint8_t rx_slot = 0u;
volatile uint8_t slot_valid = 0u;
volatile uint32_t rx_count = 0u;
volatile uint32_t slot_drop_count = 0u;
volatile uint32_t uart_error_count = 0u;
volatile uint8_t last_processed = 0u;
volatile uint32_t processed_count = 0u;

static void usart1_rx_irq_init(void)
{
    NVIC_DisableIRQ(USART1_IRQn);

    RCC->APB2ENR |= (1u << 2) | (1u << 14);
    RCC->APB2RSTR |=  (1u << 14);
    RCC->APB2RSTR &= ~(1u << 14);

    /* PA10: input floating. */
    GPIOA->CRH = (GPIOA->CRH & ~(15u << 8)) | (4u << 8);

    USART1->CR1 = 0u;
    USART1->CR2 = 0u;
    USART1->CR3 = 0u;
    USART1->BRR = 0x0341u; /* nominal 8 MHz / 9600 baud */

    NVIC_ClearPendingIRQ(USART1_IRQn);
    NVIC_EnableIRQ(USART1_IRQn);

    /* UE=1, RE=1, RXNEIE=1. */
    USART1->CR1 = (1u << 13) | (1u << 2) | (1u << 5);
}

void USART1_IRQHandler(void)
{
    uint32_t status;
    uint8_t data;

    status = USART1->SR;

    if ((status & 0x2Fu) != 0u) { /* RXNE or PE/FE/NE/ORE */
        data = (uint8_t)USART1->DR; /* SR then DR */

        if ((status & 0x0Fu) != 0u) {
            uart_error_count++; /* Discard this read. */
        } else if ((status & (1u << 5)) != 0u) {
            rx_count++;

            if (slot_valid == 0u) {
                rx_slot = data;
                slot_valid = 1u; /* Publish after writing data. */
            } else {
                slot_drop_count++; /* Preserve the older byte. */
            }
        }
    }
}

static uint8_t take_rx_byte(uint8_t *data)
{
    uint32_t saved_mask;
    uint8_t have_data = 0u;

    saved_mask = __get_PRIMASK();
    __disable_irq();

    if (slot_valid != 0u) {
        *data = rx_slot;
        slot_valid = 0u;
        have_data = 1u;
    }

    __set_PRIMASK(saved_mask);
    return have_data;
}

int main(void)
{
    uint8_t data;

    usart1_rx_irq_init();

    while (1) {
        if (take_rx_byte(&data) != 0u) {
            last_processed = data;
            processed_count++;
        }
        /* Other short, non-blocking tasks may run here. */
    }
}
