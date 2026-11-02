#include "stm32f10x.h"

volatile uint32_t before_cr = 0U, before_cfgr = 0U;
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

    /* This target requires the confirmed 8MHz crystal circuit, not bypass input. */
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

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSE;
    clock_stage = 40U;
    poll_budget = 1000000U;
    while (((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE) && (poll_budget != 0U))
    {
        poll_budget--;
    }
    if ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE)
    {
        clock_error = 4U;
        goto observe;
    }
    SystemCoreClock = 8000000U; /* Software bookkeeping for the declared source. */
    clock_stage = 50U;

observe:
    final_cr = RCC->CR;
    final_cfgr = RCC->CFGR;
    /* Keep both enabled sources on. A pending switch could finish after a budget expires. */
    while (1)
    {
        live_cr = RCC->CR;
        live_cfgr = RCC->CFGR;
    }
}
