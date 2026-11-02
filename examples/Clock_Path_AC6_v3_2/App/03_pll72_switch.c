#include "stm32f10x.h"

volatile uint32_t before_cr = 0U, before_cfgr = 0U;
volatile uint32_t flash_prepared = 0U;
volatile uint32_t final_cr = 0U, final_cfgr = 0U;
volatile uint32_t live_cr = 0U, live_cfgr = 0U;
volatile uint32_t poll_budget = 0U;
volatile uint32_t clock_stage = 0U, clock_error = 0U;

int main(void)
{
    before_cr = RCC->CR;
    before_cfgr = RCC->CFGR;
    if (((before_cr & RCC_CR_HSIRDY) == 0U) ||
        ((before_cr & (RCC_CR_HSEON | RCC_CR_HSERDY | RCC_CR_PLLON | RCC_CR_PLLRDY)) != 0U) ||
        ((before_cfgr & (RCC_CFGR_SW | RCC_CFGR_SWS |
                         RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) != 0U))
    {
        clock_error = 1U;
        goto observe;
    }
    clock_stage = 10U;

    /* LATENCY[2:0]=010, HLFCYA=0, PRFTBE=1, while still on HSI8MHz and AHB/1. */
    FLASH->ACR = (FLASH->ACR & ~0x0FU) | FLASH_ACR_PRFTBE | 0x2U;
    flash_prepared = FLASH->ACR;
    if ((flash_prepared & 0x1FU) != 0x12U)
    {
        clock_error = 5U;
        goto observe;
    }

    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 |
                               RCC_CFGR_PPRE2 | RCC_CFGR_ADCPRE)) |
                RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_ADCPRE_DIV6;

    RCC->CR &= ~RCC_CR_HSEBYP;
    RCC->CR |= RCC_CR_HSEON;
    poll_budget = 1000000U;
    while (((RCC->CR & RCC_CR_HSERDY) == 0U) && (poll_budget != 0U))
    {
        poll_budget--;
    }
    if ((RCC->CR & RCC_CR_HSERDY) == 0U)
    {
        clock_error = 2U;
        goto observe;
    }
    clock_stage = 20U;

    /* PLL was checked OFF above. HSE8MHz /1 x9 = PLL72MHz. */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE |
                               RCC_CFGR_PLLMULL)) |
                RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL9;
    RCC->CR |= RCC_CR_PLLON;
    poll_budget = 1000000U;
    while (((RCC->CR & RCC_CR_PLLRDY) == 0U) && (poll_budget != 0U))
    {
        poll_budget--;
    }
    if ((RCC->CR & RCC_CR_PLLRDY) == 0U)
    {
        clock_error = 3U;
        goto observe;
    }
    clock_stage = 30U;

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    clock_stage = 40U;
    poll_budget = 1000000U;
    while (((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) && (poll_budget != 0U))
    {
        poll_budget--;
    }
    if ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
    {
        clock_error = 4U;
        goto observe;
    }
    SystemCoreClock = 72000000U; /* Declared HSE8MHz x9, AHB/1; not a measurement. */
    clock_stage = 50U;

observe:
    final_cr = RCC->CR;
    final_cfgr = RCC->CFGR;
    /* No automatic rollback. Do not disable a source while SWS is uncertain. */
    while (1)
    {
        live_cr = RCC->CR;
        live_cfgr = RCC->CFGR;
    }
}
