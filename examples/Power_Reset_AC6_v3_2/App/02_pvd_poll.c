#include "stm32f10x.h"

volatile uint32_t pvd_cr = 0U;
volatile uint32_t pvd_csr = 0U;
volatile uint32_t pvdo = 0U;
volatile uint32_t pvd_ready = 0U;

int main(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;

    /* PLS=111: nominal level name 2.9V; actual rising/falling ranges are in DS5792. */
    PWR->CR = (PWR->CR & ~PWR_CR_PLS) | (7U << 5);
    PWR->CR |= PWR_CR_PVDE;
    pvd_cr = PWR->CR;

    while (1)
    {
        pvd_csr = PWR->CSR;
        pvdo = (pvd_csr & PWR_CSR_PVDO) != 0U;
        pvd_ready = 1U;
    }
}
