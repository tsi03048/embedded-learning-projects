#include "stm32f10x.h"

volatile uint32_t phase = 0U, request = 0U, app_error = 0U;
volatile uint32_t start_cfgr = 0U, divided_cfgr = 0U;
volatile uint32_t enr_before = 0U, enr_enabled = 0U;
volatile uint32_t init_budget = 0U, init_sr = 0U;
volatile uint32_t configured_cr1 = 0U, configured_psc = 0U, configured_arr = 0U;
volatile uint32_t divided_cr1 = 0U, divided_psc = 0U, divided_arr = 0U;
volatile uint32_t live_cnt = 0U;

int main(void)
{
    start_cfgr = RCC->CFGR;
    if (((RCC->CR & RCC_CR_HSIRDY) == 0U) ||
        ((start_cfgr & (RCC_CFGR_SW | RCC_CFGR_SWS |
                        RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) != 0U))
    {
        app_error = 1U;
        while (1) { }
    }

    enr_before = RCC->APB1ENR;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    enr_enabled = RCC->APB1ENR; /* Read back after enabling the interface clock. */

    TIM2->CR1 = 0U;
    TIM2->DIER = 0U;           /* No timer interrupt or DMA requests. */
    TIM2->SR = 0U;
    TIM2->PSC = 7999U;
    TIM2->ARR = 9999U;
    TIM2->CNT = 0U;
    TIM2->EGR = TIM_EGR_UG;
    init_budget = 10000U;
    while (((TIM2->SR & TIM_SR_UIF) == 0U) && (init_budget != 0U))
    {
        init_budget--;
    }
    init_sr = TIM2->SR;
    if ((init_sr & TIM_SR_UIF) == 0U)
    {
        app_error = 2U;
        while (1) { }
    }
    TIM2->SR = 0U;             /* Clear the initialization update flag. */
    configured_cr1 = TIM2->CR1;
    configured_psc = TIM2->PSC;
    configured_arr = TIM2->ARR;
    phase = 1U;               /* Gate enabled, counter still disabled. */

    while (1)
    {
        if ((phase == 1U) && (request == 1U))
        {
            TIM2->CR1 |= TIM_CR1_CEN;
            phase = 2U;
        }
        else if ((phase == 2U) && (request == 2U))
        {
            /* HCLK stays8MHz. APB1 /4 -> PCLK1=2MHz -> TIM2 clock=4MHz. */
            RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_PPRE1) | RCC_CFGR_PPRE1_DIV4;
            divided_cfgr = RCC->CFGR;
            divided_cr1 = TIM2->CR1;
            divided_psc = TIM2->PSC;
            divided_arr = TIM2->ARR;
            phase = 3U;
        }
        live_cnt = TIM2->CNT;
    }
}
