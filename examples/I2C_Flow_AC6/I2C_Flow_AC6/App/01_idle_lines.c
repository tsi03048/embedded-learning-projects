#include "stm32f10x.h"

volatile uint32_t i2c_ready = 0u;
volatile uint32_t transfer_done = 0u;
volatile uint32_t scl_high = 0u;
volatile uint32_t sda_high = 0u;
volatile uint32_t bus_busy = 0u;

int main(void)
{
    volatile uint32_t power_settle;

    /* Supplied SystemInit: HSI nominal8MHz, APB1 /1 -> PCLK1=8MHz.
       External pull-ups on PB6/PB7 to3.3V; single-controller isolated bus. */
    RCC->APB2ENR |= 1u << 3;  /* GPIOB clock. */
    RCC->APB1ENR |= 1u << 21; /* I2C1 clock. */
    (void)RCC->APB1ENR;
    RCC->APB1RSTR |= 1u << 21;
    RCC->APB1RSTR &= ~(1u << 21);
    /* Full MCU reset -> default I2C1_REMAP=0: PB6 SCL, PB7 SDA. */
    GPIOB->BSRR = (1u << 6) | (1u << 7); /* ODR high for GPIO release. */
    GPIOB->CRL = (GPIOB->CRL & ~((0xFu << 24) | (0xFu << 28)))
               | (0xEu << 24) | (0xEu << 28); /* AF open-drain, 2MHz grade. */
    I2C1->CR1 = 0u; /* PE=0 while configuring; I2C, no general call. */
    I2C1->CR2 = 8u; /* FREQ[5:0]=8MHz. All IRQ/DMA disabled. */
    I2C1->OAR1 = 1u << 14; /* Required bit14=1, 7-bit local address mode. */
    I2C1->OAR2 = 0u; /* Dual own address disabled. Not the target address. */
    I2C1->CCR = 50u; /* Standard mode: 8MHz/(2*50)=nominal80kHz. */
    I2C1->TRISE = 9u; /* Standard max rise1us: 8MHz*1us+1=9. */
    I2C1->CR1 = (1u << 10) | (1u << 0); /* ACK=1, POS=0, PE=1. */
    /* AT24C02D requires >=100us after its VCC is stable (DS20006100A Table4-4).
       Reset MCU after verified module power is stable. 1024 NOPs alone take
       nominal128us at8MHz, plus loop overhead. Not a bus-period timer. */
    for (power_settle = 0u; power_settle < 1024u; ++power_settle) {
        __NOP();
    }
    i2c_ready = 1u;

    while (1) {
        scl_high = (GPIOB->IDR >> 6) & 1u;
        sda_high = (GPIOB->IDR >> 7) & 1u;
        bus_busy = (I2C1->SR2 >> 1) & 1u;
        /* No START, no address, no data transfer. No SR1->SR2 sequence here. */
    }
}
