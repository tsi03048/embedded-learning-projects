#include "stm32f10x.h"
#include <stdint.h>
#include <string.h>

/* USART1 PCLK2 nominal 8 MHz, default PA10 RX.
   Standalone, single-main-loop polling example. */

#define LINE_SIZE 16u

volatile uint8_t led_request = 0u;
volatile uint32_t command_count = 0u;
volatile uint32_t unknown_count = 0u;
volatile uint32_t overflow_count = 0u;
volatile uint32_t invalid_count = 0u;
volatile uint32_t uart_error_count = 0u;

/* 0: no complete result, 1: ON, 2: OFF,
   3: unknown, 4: discarded, 5: response busy. */
volatile uint8_t last_result = 0u;

static char line_buffer[LINE_SIZE];
static uint8_t line_length = 0u;
static uint8_t discard_line = 0u;
static uint8_t cr_seen = 0u;

static void usart1_rx_init(void)
{
    RCC->APB2ENR |= (1u << 2) | (1u << 14);
    RCC->APB2RSTR |=  (1u << 14);
    RCC->APB2RSTR &= ~(1u << 14);

    /* PA9 = AF push-pull, PA10 = floating input. */
    GPIOA->CRH = (GPIOA->CRH & ~(0xFFu << 4)) | (0x49u << 4);

    USART1->CR1 = 0u;
    USART1->CR2 = 0u; /* 1 stop bit */
    USART1->CR3 = 0u; /* no hardware flow control */
    USART1->BRR = 0x0341u; /* PCLK2 nominal 8 MHz, 9600 baud */
    USART1->CR1 = (1u << 13) | (1u << 3) | (1u << 2); /* UE, TE, RE */
}

static void discard_until_lf(void)
{
    line_length = 0u;
    cr_seen = 0u;
    discard_line = 1u;
    last_result = 4u;
}


volatile uint8_t tx_busy = 0u; /* 1 until the last response byte is queued */
volatile uint32_t busy_reject_count = 0u;
static const char *tx_text;
static uint8_t tx_index = 0u;

static void start_response(const char *text)
{
    tx_text = text;
    tx_index = 0u;
    tx_busy = 1u;
}

static void response_task(void)
{
    if (tx_busy != 0u) {
        if ((USART1->SR & (1u << 7)) != 0u) { /* TXE */
            USART1->DR = (uint8_t)tx_text[tx_index];
            tx_index++;
            if (tx_text[tx_index] == 0) {
                tx_busy = 0u; /* All bytes queued; TC may still be 0. */
            }
        }
    }
}

static void process_line(void)
{
    line_buffer[line_length] = 0;

    if (line_length != 0u) {
        if (tx_busy != 0u) {
            busy_reject_count++;
            last_result = 5u; /* Rejected, no action and no reply. */
        } else if (strcmp(line_buffer, "LED ON") == 0) {
            led_request = 1u;
            command_count++;
            last_result = 1u;
            start_response("OK ON\r\n");
        } else if (strcmp(line_buffer, "LED OFF") == 0) {
            led_request = 0u;
            command_count++;
            last_result = 2u;
            start_response("OK OFF\r\n");
        } else {
            unknown_count++;
            last_result = 3u;
            start_response("ERR CMD\r\n");
        }
    }

    line_length = 0u;
    cr_seen = 0u;
}

static void accept_byte(uint8_t data)
{
    if (discard_line != 0u) {
        if (data == 0x0Au) {
            discard_line = 0u;
        }
        return;
    }

    if (data == 0x0Au) {
        process_line();
        return;
    }

    if (data == 0x0Du) {
        if (cr_seen != 0u) {
            invalid_count++;
            discard_until_lf();
        } else {
            cr_seen = 1u;
        }
        return;
    }

    /* CR must be followed by LF; only printable ASCII accepted. */
    if ((cr_seen != 0u) || (data < 0x20u) || (data > 0x7Eu)) {
        invalid_count++;
        discard_until_lf();
        return;
    }

    if (line_length < (LINE_SIZE - 1u)) {
        line_buffer[line_length] = (char)data;
        line_length++;
    } else {
        overflow_count++;
        discard_until_lf();
    }
}

static void uart_poll(void)
{
    uint32_t status;
    uint8_t data;

    status = USART1->SR;

    if ((status & 0x2Fu) != 0u) { /* RXNE or PE/FE/NE/ORE */
        data = (uint8_t)USART1->DR; /* SR read, then DR read */

        if ((status & 0x0Fu) != 0u) {
            uart_error_count++;
            discard_until_lf();
        } else if ((status & (1u << 5)) != 0u) {
            accept_byte(data);
        }
    }
}

int main(void)
{
    usart1_rx_init();

    while (1) {
        uart_poll();
        response_task();
        /* Keep other work short enough for the input rate. */
    }
}
