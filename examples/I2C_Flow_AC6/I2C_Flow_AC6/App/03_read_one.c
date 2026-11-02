#include "stm32f10x.h"

volatile uint32_t i2c_ready = 0u;
volatile uint32_t transfer_done = 0u;
volatile uint32_t received = 0x100u; /* NOT a valid byte. */
volatile uint32_t result_valid = 0u;
volatile uint32_t error_flags = 0u;

int main(void)
{
    uint32_t data;
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

    /* Conditional example: AT24C02D, A2/A1/A0=000, normal ready state.
       Run only after address probe ACK. Unknown stored byte at word address12.
       Normal path shown; no timeout, retry or unexpected-error recovery. */
    while ((I2C1->SR2 & (1u << 1)) != 0u) { }
    I2C1->CR1 |= 1u << 8;
    while ((I2C1->SR1 & (1u << 0)) == 0u) { }
    I2C1->DR = 0xA0u; /* Device50, Write direction. */
    while ((I2C1->SR1 & (1u << 1)) == 0u) { }
    (void)I2C1->SR1;
    (void)I2C1->SR2;
    while ((I2C1->SR1 & (1u << 7)) == 0u) { } /* TxE. */
    I2C1->DR = 0x12u; /* EEPROM word address, NOT payload to program. */
    while ((I2C1->SR1 & (1u << 2)) == 0u) { } /* BTF: pointer byte+ACK done. */
    I2C1->CR1 |= 1u << 8; /* Repeated START, no intervening STOP. */
    while ((I2C1->SR1 & (1u << 0)) == 0u) { }
    I2C1->DR = 0xA1u; /* Device50, Read direction. */
    while ((I2C1->SR1 & (1u << 1)) == 0u) { }

    /* ONE-byte receive, RM0008 Figure277 EV6_3.
       ES0340 Rev17 2.9.1 workaround3: hold SCL low across ADDR clear/STOP.
       ES0340 says SR3, a documentation typo; F103 has SR2, as RM0008 states. */
    I2C1->CR1 &= ~(1u << 10); /* ACK=0 BEFORE clearing ADDR: final NACK. */
    GPIOB->BRR = 1u << 6; /* Prepare GPIO ODR6=0 while AF controls pin. */
    GPIOB->CRL = (GPIOB->CRL & ~(0xFu << 24)) | (0x6u << 24);
    /* PB6 now GPIO open-drain LOW, holding SCL, not push-pull HIGH. */
    (void)I2C1->SR1;
    (void)I2C1->SR2; /* ADDR clear. */
    I2C1->CR1 |= 1u << 9; /* Schedule STOP after final received byte. */
    GPIOB->CRL = (GPIOB->CRL & ~(0xFu << 24)) | (0xEu << 24);
    /* Restore PB6 AF open-drain: I2C completes byte/NACK/STOP. */
    while ((I2C1->SR1 & (1u << 6)) == 0u) { } /* RxNE. */
    data = I2C1->DR & 0xFFu;
    while ((I2C1->CR1 & (1u << 9)) != 0u) { }
    while ((I2C1->SR2 & (1u << 1)) != 0u) { }
    error_flags = I2C1->SR1 & 0x0F00u; /* BERR, ARLO, AF, OVR. */
    if (error_flags == 0u) {
        received = data;
        result_valid = 1u;
        transfer_done = 1u;
    }
    while (1) {
        /* One read only. No EEPROM data-byte write/erase. */
    }
}
