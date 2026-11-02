#include "stm32f10x.h"

volatile uint32_t before_cr = 0U, before_cfgr = 0U;
volatile uint32_t after_cfgr = 0U;
volatile uint32_t apply_request = 0U;
volatile uint32_t clock_stage = 0U, clock_error = 0U;

int main(void)
{
    before_cr = RCC->CR;
    before_cfgr = RCC->CFGR;
    if (((before_cr & RCC_CR_HSIRDY) == 0U) ||
        ((before_cfgr & (RCC_CFGR_SW | RCC_CFGR_SWS |
                         RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) != 0U))
    {
        clock_error = 1U;
    }
    else
    {
        clock_stage = 1U;
    }

    while (1)
    {
        if ((apply_request == 1U) && (clock_stage == 1U))
        {
            /* Keep SYSCLK/HCLK/PCLK2 at nominal8MHz; only PCLK1 becomes4MHz. */
            RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_PPRE1) | RCC_CFGR_PPRE1_DIV2;
            after_cfgr = RCC->CFGR;
            clock_stage = 2U;
        }
    }
}
