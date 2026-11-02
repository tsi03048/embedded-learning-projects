#include "stm32f10x.h"
#include <stdint.h>
#include <string.h>

/* Standalone: USART1 PCLK2 nominal 8 MHz, PA10 default RX,
   RXNE interrupt, one ISR producer, one main-loop consumer. */
#define RX_SIZE 16u
#define RX_MASK 15u

static volatile uint8_t rx_buffer[RX_SIZE];
static volatile uint8_t rx_head = 0u; /* next ISR write */
static volatile uint8_t rx_tail = 0u; /* next main read */
static volatile uint8_t rx_fault = 0u;

volatile uint32_t stored_count = 0u;
volatile uint32_t full_count = 0u;
volatile uint32_t fault_drop_count = 0u;
volatile uint32_t uart_error_count = 0u;
volatile uint32_t recovery_count = 0u;
volatile uint32_t processed_count = 0u;
volatile uint8_t last_processed = 0u;

static void usart1_rx_irq_init(void)
{
    NVIC_DisableIRQ(USART1_IRQn);

    RCC->APB2ENR |= (1u << 2) | (1u << 14);
    RCC->APB2RSTR |=  (1u << 14);
    RCC->APB2RSTR &= ~(1u << 14);
    GPIOA->CRH = (GPIOA->CRH & ~(15u << 8)) | (4u << 8);

    USART1->CR1 = 0u;
    USART1->CR2 = 0u;
    USART1->CR3 = 0u;
    USART1->BRR = 0x0341u; /* nominal 8 MHz / 9600 baud */

    NVIC_ClearPendingIRQ(USART1_IRQn);
    NVIC_EnableIRQ(USART1_IRQn);
    USART1->CR1 = (1u << 13) | (1u << 2) | (1u << 5);
    /* UE, RE, RXNEIE */
}

void USART1_IRQHandler(void)
{
    uint32_t status;
    uint8_t data;
    uint8_t next;

    status = USART1->SR;
    if ((status & 0x2Fu) != 0u) { /* RXNE or PE/FE/NE/ORE */
        data = (uint8_t)USART1->DR; /* F103: SR, then DR */

        if ((status & 0x0Fu) != 0u) {
            uart_error_count++;
            rx_fault = 1u;
        } else if ((status & (1u << 5)) != 0u) {
            if (rx_fault != 0u) {
                fault_drop_count++;
            } else {
                next = (uint8_t)((rx_head + 1u) & RX_MASK);
                if (next == rx_tail) {
                    full_count++;
                    rx_fault = 1u; /* New byte is discarded. */
                } else {
                    rx_buffer[rx_head] = data;
                    rx_head = next; /* Publish after storing data. */
                    stored_count++;
                }
            }
        }
    }
}

/* Return 0: empty, 1: byte, 2: fault observed and queue flushed.
   Call only from the single main-loop consumer. */
static uint8_t ring_take(uint8_t *data)
{
    uint32_t saved_mask;
    uint8_t result = 0u;

    saved_mask = __get_PRIMASK();
    __disable_irq();

    if (rx_fault != 0u) {
        rx_tail = rx_head; /* Discard buffered bytes. */
        rx_fault = 0u;
        result = 2u;
    } else if (rx_tail != rx_head) {
        *data = rx_buffer[rx_tail];
        rx_tail = (uint8_t)((rx_tail + 1u) & RX_MASK);
        result = 1u;
    }

    __set_PRIMASK(saved_mask);
    return result;
}


/* Parser is owned by main only. At startup discard until a clean LF. */
#define LINE_SIZE 16u
static char line_buffer[LINE_SIZE];
static uint8_t line_len = 0u;
static uint8_t line_discard = 1u;
static uint8_t cr_seen = 0u;

volatile uint8_t led_request = 0u; /* Watch only: no GPIO action. */
volatile uint32_t command_count = 0u;
volatile uint32_t unknown_count = 0u;
volatile uint32_t line_reject_count = 0u;
volatile uint32_t sync_count = 0u;
volatile uint32_t blocked_commit_count = 0u;

static void parser_discard_to_lf(void)
{
    line_len = 0u;
    cr_seen = 0u;
    line_discard = 1u;
}

/* Called only after a complete LF-terminated line. */
static void parser_finish(void)
{
    uint8_t candidate = 0u; /* 1=ON, 2=OFF, 3=unknown */
    uint32_t saved_mask;

    line_buffer[line_len] = 0;
    if (line_len != 0u) {
        if (strcmp(line_buffer, "LED ON") == 0) {
            candidate = 1u;
        } else if (strcmp(line_buffer, "LED OFF") == 0) {
            candidate = 2u;
        } else {
            candidate = 3u;
        }
    }

    /* Serialize the observed-fault check and Watch-variable update
       against USART1_IRQHandler. This is NOT a hardware integrity check. */
    saved_mask = __get_PRIMASK();
    __disable_irq();
    if (rx_fault != 0u) {
        blocked_commit_count++;
        parser_discard_to_lf(); /* ring_take will flush the fault next */
    } else {
        if (candidate == 1u) {
            led_request = 1u;
            command_count++;
        } else if (candidate == 2u) {
            led_request = 0u;
            command_count++;
        } else if (candidate == 3u) {
            unknown_count++;
        }
        line_len = 0u;
        cr_seen = 0u;
    }
    __set_PRIMASK(saved_mask);
}

static void parser_accept(uint8_t data)
{
    if (line_discard != 0u) {
        if (data == 0x0Au) {
            line_discard = 0u;
            line_len = 0u;
            cr_seen = 0u;
            sync_count++;
        }
        return;
    }

    if (data == 0x0Au) {
        parser_finish();
        return;
    }

    if (data == 0x0Du) {
        if (cr_seen != 0u) {
            line_reject_count++;
            parser_discard_to_lf();
        } else {
            cr_seen = 1u;
        }
        return;
    }

    if ((cr_seen != 0u) || (data < 0x20u) || (data > 0x7Eu)) {
        line_reject_count++;
        parser_discard_to_lf();
    } else if (line_len < (LINE_SIZE - 1u)) {
        line_buffer[line_len] = (char)data;
        line_len++;
    } else {
        line_reject_count++;
        parser_discard_to_lf();
    }
}

int main(void)
{
    uint8_t data;
    uint8_t result;

    usart1_rx_irq_init();

    while (1) {
        result = ring_take(&data);
        if (result == 1u) {
            last_processed = data;
            processed_count++;
            parser_accept(data);
        } else if (result == 2u) {
            recovery_count++;
            parser_discard_to_lf(); /* Discard until a later clean LF. */
        }
        /* Keep other tasks short; do not treat data as commands here. */
    }
}
