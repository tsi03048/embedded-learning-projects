#include "stm32f10x.h"

volatile uint32_t i2c_ready = 0u;
volatile uint32_t transfer_done = 0u;
volatile uint32_t address_acked = 0u;
volatile uint32_t address_nacked = 0u;
volatile uint32_t error_flags = 0u;

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

    while ((I2C1->SR2 & (1u << 1)) != 0u) { } /* BUSY=0 first. */
    I2C1->CR1 |= 1u << 8; /* START. */
    while ((I2C1->SR1 & (1u << 0)) == 0u) { } /* SB. */
    I2C1->DR = 0x50u << 1; /* 7-bit address50 + W0 -> A0 on the bus. */
    while ((I2C1->SR1 & ((1u << 1) | (1u << 10))) == 0u) { }
    if ((I2C1->SR1 & (1u << 1)) != 0u) { /* ADDR: target ACK. */
        I2C1->CR1 |= 1u << 9; /* Address-only probe: request STOP BEFORE clear. */
        (void)I2C1->SR1;
        (void)I2C1->SR2; /* Clear ADDR, allowing the pending STOP. */
        address_acked = 1u;
    } else { /* AF: target did not acknowledge. */
        I2C1->CR1 |= 1u << 9;
        I2C1->SR1 &= ~(1u << 10); /* Clear AF (write0), not ADDR clear. */
        address_nacked = 1u;
    }
    while ((I2C1->CR1 & (1u << 9)) != 0u) { } /* STOP request completed. */
    while ((I2C1->SR2 & (1u << 1)) != 0u) { } /* Bus released. */
    error_flags = I2C1->SR1 & ((1u << 8) | (1u << 9) | (1u << 11));
    if (error_flags == 0u) {
        transfer_done = 1u; /* Completed probe, ACK and NACK kept distinct. */
    }
    while (1) { }
}
