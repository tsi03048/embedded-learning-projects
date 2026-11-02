#include "stm32f10x.h"
#include <stdint.h>

#define TEST_BYTE 0xA6u
#define SPI_MODE 0u /* Change to 3u for mode 3, then rebuild. */
#if SPI_MODE != 0u && SPI_MODE != 3u
#error Only mode 0 or 3 supported in this lesson
#endif
#define WAIT_MS 100u
#define SR_RXNE (1u << 0)
#define SR_TXE  (1u << 1)
#define SR_MODF (1u << 5)
#define SR_OVR  (1u << 6)
#define SR_BSY  (1u << 7)

enum { WAIT_TXE = 1, WAIT_RXNE, WAIT_END, DONE, FAILED };
enum { ERR_NONE = 0, ERR_TICK, ERR_REMAP, ERR_TIMEOUT, ERR_SPI };

volatile uint32_t ms_tick;
volatile uint32_t spi_state;
volatile uint32_t spi_error;
volatile uint32_t spi_last_sr;
volatile uint8_t spi_rx;
volatile uint8_t spi_match;
static uint32_t stage_start;

void SysTick_Handler(void) { ms_tick++; }

static void next_state(uint32_t state)
{
    stage_start = ms_tick;
    spi_state = state;
}

static void spi_init(void)
{
    RCC->APB2ENR |= (1u << 0) | (1u << 2) | (1u << 12);
    RCC->APB2RSTR |= (1u << 12);
    RCC->APB2RSTR &= ~(1u << 12);

    /* Standalone project: SPI1_REMAP must be zero. */
    if ((AFIO->MAPR & 1u) != 0u) {
        spi_error = ERR_REMAP;
        spi_state = FAILED;
        return;
    }

    /* PA5 and PA7 AF push-pull 2 MHz; PA6 floating input. */
    GPIOA->CRL = (GPIOA->CRL & ~((15u << 20) | (15u << 24) | (15u << 28)))
               | (10u << 20) | (4u << 24) | (10u << 28);

    SPI1->CR2 = 0u;
    /* 2-line full duplex, 8 bit, MSB first, mode selected below, /256,
       software NSS high, master. SPI is enabled last. */
    SPI1->CR1 = (1u << 2) | (7u << 3) | (1u << 8) | (1u << 9);
    SPI1->CR1 |= SPI_MODE; /* CR1 CPOL bit 1, CPHA bit 0 */
    SPI1->CR1 |= (1u << 6);
    next_state(WAIT_TXE);
}

static void spi_task(void)
{
    uint32_t sr;
    if ((spi_state == DONE) || (spi_state == FAILED)) return;

    sr = SPI1->SR;
    spi_last_sr = sr;
    if ((sr & (SR_MODF | SR_OVR)) != 0u) {
        spi_error = ERR_SPI;
        spi_state = FAILED;
        return;
    }
    if ((uint32_t)(ms_tick - stage_start) >= WAIT_MS) {
        spi_error = ERR_TIMEOUT;
        spi_state = FAILED;
        return;
    }
    if (spi_state == WAIT_TXE) {
        if ((sr & SR_TXE) != 0u) {
            SPI1->DR = TEST_BYTE;
            next_state(WAIT_RXNE);
        }
    } else if (spi_state == WAIT_RXNE) {
        if ((sr & SR_RXNE) != 0u) {
            spi_rx = (uint8_t)SPI1->DR;
            next_state(WAIT_END);
        }
    } else if (spi_state == WAIT_END) {
        if (((sr & SR_TXE) != 0u) && ((sr & SR_BSY) == 0u)) {
            spi_match = (spi_rx == TEST_BYTE) ? 1u : 0u;
            spi_state = DONE;
        }
    }
}

int main(void)
{
    SystemCoreClockUpdate();
    if (SysTick_Config(SystemCoreClock / 1000u) != 0u) {
        spi_error = ERR_TICK;
        spi_state = FAILED;
    } else {
        spi_init();
    }
    while (1) {
        spi_task();
        /* Other short, non-blocking work goes here. */
    }
}


