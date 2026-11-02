#include "stm32f10x.h"

volatile uint32_t cnt_snapshot = 0u;
volatile uint32_t tim2_services = 0u;
volatile uint32_t main_loops = 0u;

void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & (1u << 0)) != 0u) {
        TIM2->SR = 0u;       /* Clear status: TIM2 is used for update only. */
        tim2_services++;
    }
}

int main(void)
{
    NVIC->ICER[0] = 1u << 28; /* TIM2 is IRQ28 on STM32F103. */
    /* HSI nominal 8 MHz; HCLK=8 MHz, APB1 /1 -> TIM2=8 MHz.
       The supplied system_stm32f10x.c keeps this reset clock path. */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;
    /* This project owns TIM2; reset all its configuration. */
    RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;

    TIM2->CR1 = 0u;          /* Stopped, upcount, edge-aligned, ARPE=0. */
    TIM2->DIER = 0u;         /* No interrupt or DMA request yet. */
    TIM2->PSC = 7999u;       /* 8 MHz / (7999+1) = 1000 counts/s. */
    TIM2->ARR = 999u;   /* Count 0 through ARR, inclusive. */
    TIM2->CNT = 0u;

    TIM2->EGR = 1u << 0;     /* UG: load PSC and initialize the time base. */
    while ((TIM2->SR & (1u << 0)) == 0u) {
        /* Setup only: wait for this generated update to be recorded. */
    }
    TIM2->SR = 0u;           /* Clear setup flags; this timer uses update only. */

    NVIC->ICPR[0] = 1u << 28;
    TIM2->DIER |= 1u << 0;    /* UIE: update interrupt request enable. */
    NVIC->ISER[0] = 1u << 28;
    TIM2->CR1 |= 1u << 0;

    while (1) {
        cnt_snapshot = TIM2->CNT;
        main_loops++;
    }
}
