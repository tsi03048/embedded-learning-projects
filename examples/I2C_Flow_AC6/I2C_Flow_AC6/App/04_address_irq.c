#include "stm32f10x.h"

volatile uint32_t i2c_ready = 0u;
volatile uint32_t transfer_done = 0u;
volatile uint32_t address_acked = 0u;
volatile uint32_t address_nacked = 0u;
volatile uint32_t error_flags = 0u;
volatile uint32_t main_loops = 0u;

void I2C1_EV_IRQHandler(void)
{
    uint32_t status = I2C1->SR1; /* Do NOT read SR2 in a general snapshot. */
    if ((status & 0x0F00u) != 0u) {
        I2C1->CR2 &= ~(1u << 9); /* Let the error IRQ handle the cause. */
        return;
    }
    if ((status & (1u << 0)) != 0u) { /* SB */
        I2C1->DR = 0xA0u;
    } else if ((status & (1u << 1)) != 0u) { /* ADDR */
        I2C1->CR2 &= ~((1u << 9) | (1u << 8)); /* Event/error IRQ off. */
        I2C1->CR1 |= 1u << 9; /* Probe STOP BEFORE ADDR clear. */
        (void)I2C1->SR1;
        (void)I2C1->SR2;
        address_acked = 1u;
    }
}

void I2C1_ER_IRQHandler(void)
{
    uint32_t status = I2C1->SR1;
    I2C1->CR2 &= ~((1u << 9) | (1u << 8));
    error_flags = status & ((1u << 8) | (1u << 9) | (1u << 11));
    if ((status & (1u << 9)) == 0u) { /* No STOP if arbitration lost. */
        I2C1->CR1 |= 1u << 9;
    }
    if ((status & (1u << 10)) != 0u) { /* AF = expected address NACK case. */
        I2C1->SR1 &= ~(1u << 10);
        address_nacked = 1u;
    }
    /* Other errors stay recorded; no automatic recovery/retry. */
}

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

    while ((I2C1->SR2 & (1u << 1)) != 0u) { }
    NVIC->ICER[0] = 1u << 31; /* I2C1_EV IRQ31. */
    NVIC->ICER[1] = 1u << 0;  /* I2C1_ER IRQ32. */
    NVIC->ICPR[0] = 1u << 31;
    NVIC->ICPR[1] = 1u << 0;
    I2C1->CR2 |= (1u << 9) | (1u << 8); /* ITEVTEN, ITERREN; ITBUFEN=0. */
    NVIC->ISER[0] = 1u << 31;
    NVIC->ISER[1] = 1u << 0;
    I2C1->CR1 |= 1u << 8;
    while ((address_acked == 0u) && (address_nacked == 0u) && (error_flags == 0u)) {
        main_loops++;
    }
    if (error_flags == 0u) {
        while ((I2C1->CR1 & (1u << 9)) != 0u) { }
        while ((I2C1->SR2 & (1u << 1)) != 0u) { }
        error_flags = I2C1->SR1 & ((1u << 8) | (1u << 9) | (1u << 11));
        if (error_flags == 0u) {
            transfer_done = 1u;
        }
    }
    while (1) {
        main_loops++;
    }
}
