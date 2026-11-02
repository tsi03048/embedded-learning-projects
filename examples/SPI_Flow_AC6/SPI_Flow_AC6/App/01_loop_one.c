#include "stm32f10x.h"

volatile uint32_t spi_ready = 0u;
volatile uint32_t transfer_done = 0u;
volatile uint32_t received = 0x100u; /* Not a valid 8-bit result. */
volatile uint32_t rx_count = 0u;
volatile uint32_t matches = 0u;

int main(void)
{
    /* Supplied SystemInit: HSI nominal8MHz, HCLK/APB2 /1. Full MCU reset. */
    RCC->APB2ENR |= (1u << 2) | (1u << 12); /* GPIOA, SPI1 clocks. */
    (void)RCC->APB2ENR;
    RCC->APB2RSTR |= 1u << 12;
    RCC->APB2RSTR &= ~(1u << 12); /* Reset SPI1 only. */
    /* Default SPI1_REMAP=0 after full reset: PA5 SCK, PA6 MISO, PA7 MOSI. */
    GPIOA->CRL = (GPIOA->CRL & ~((0xFu << 20) | (0xFu << 24) | (0xFu << 28)))
               | (0xAu << 20) | (0x4u << 24) | (0xAu << 28);
    /* SCK/MOSI: AF push-pull 2MHz speed grade; MISO: floating input. */
    SPI1->CR2 = 0u; /* All IRQ, DMA and hardware NSS output disabled. */
    SPI1->CR1 = (1u << 9) | (1u << 8) | (7u << 3) | (1u << 2);
    /* SSM=1, SSI=1, BR=111(/256), MSTR=1; SPE initially0.
       BIDIMODE=0, RXONLY=0: 2-line full duplex; DFF=0: 8 bits.
       LSBFIRST=0: MSB first; CPOL=0/CPHA=0: mode0; CRC disabled. */
    SPI1->CR1 |= 1u << 6; /* SPE enable. SCK nominal31.25kHz. */
    spi_ready = 1u;

    while ((SPI1->SR & (1u << 1)) == 0u) { } /* TXE: Tx buffer free. */
    SPI1->DR = 0x96u; /* Write Tx buffer -> shift -> PA7 MOSI. */
    while ((SPI1->SR & (1u << 0)) == 0u) { } /* RXNE: result ready. */
    received = SPI1->DR & 0xFFu; /* Read Rx buffer; clears RXNE. */
    rx_count++;
    while ((SPI1->SR & (1u << 1)) == 0u) { } /* Final TXE. */
    while ((SPI1->SR & (1u << 7)) != 0u) { } /* Final BSY=0. */
    transfer_done = 1u;
    if (received == 0x96u) {
        matches = 1u;
    }
    while (1) {
        /* One exchange per reset. External jumper PA7 -> PA6 is required. */
    }
}
