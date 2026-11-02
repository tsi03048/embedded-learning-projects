#include "stm32f10x.h"

volatile uint32_t phase = 0U, request = 0U, app_error = 0U;
volatile uint32_t init_budget = 0U, init_sr = 0U;
volatile uint32_t prepared_cr1 = 0U, prepared_cnt = 0U;
volatile uint32_t prepared_psc = 0U, prepared_arr = 0U;
volatile uint32_t gate_off_enr = 0U;
volatile uint32_t reenabled_enr = 0U;
volatile uint32_t reenabled_cr1 = 0U, reenabled_cnt = 0U;
volatile uint32_t reenabled_psc = 0U, reenabled_arr = 0U;
volatile uint32_t reset_asserted_rstr = 0U, reset_held_enr = 0U;
volatile uint32_t reset_released_rstr = 0U, reset_released_enr = 0U;
volatile uint32_t reset_cr1 = 0U, reset_cnt = 0U;
volatile uint32_t reset_psc = 0U, reset_arr = 0U;

int main(void)
{
    if (((RCC->CR & RCC_CR_HSIRDY) == 0U) ||
        ((RCC->CFGR & (RCC_CFGR_SW | RCC_CFGR_SWS |
                       RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) != 0U))
    {
        app_error = 1U;
        while (1) { }
    }

    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;
    TIM2->CR1 = 0U;            /* Keep CEN=0 so CNT is a stable comparison marker. */
    TIM2->DIER = 0U;
    TIM2->SR = 0U;
    TIM2->PSC = 7999U;
    TIM2->ARR = 9999U;
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
    TIM2->SR = 0U;
    TIM2->CNT = 1234U;         /* Set AFTER UG because UG reinitializes the counter. */
    prepared_cr1 = TIM2->CR1;
    prepared_cnt = TIM2->CNT;
    prepared_psc = TIM2->PSC;
    prepared_arr = TIM2->ARR;
    phase = 1U;

    while (1)
    {
        if ((phase == 1U) && (request == 1U))
        {
            RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN;
            gate_off_enr = RCC->APB1ENR;
            phase = 2U;
        }
        else if ((phase == 2U) && (request == 2U))
        {
            RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
            reenabled_enr = RCC->APB1ENR;
            reenabled_cr1 = TIM2->CR1;
            reenabled_cnt = TIM2->CNT;
            reenabled_psc = TIM2->PSC;
            reenabled_arr = TIM2->ARR;
            phase = 3U;
        }
        else if ((phase == 3U) && (request == 3U))
        {
            RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;
            reset_asserted_rstr = RCC->APB1RSTR;
            reset_held_enr = RCC->APB1ENR;
            phase = 4U;       /* Do not access TIM2 while reset is held. */
        }
        else if ((phase == 4U) && (request == 4U))
        {
            RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;
            reset_released_rstr = RCC->APB1RSTR;
            reset_released_enr = RCC->APB1ENR;
            reset_cr1 = TIM2->CR1;
            reset_cnt = TIM2->CNT;
            reset_psc = TIM2->PSC;
            reset_arr = TIM2->ARR;
            phase = 5U;
        }
        /* No continuous TIM2 reads: phase2 is gated, phase4 is held in reset. */
    }
}
