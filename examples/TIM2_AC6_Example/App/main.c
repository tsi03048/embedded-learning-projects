#include "stm32f10x.h"

/* Observe these variables in the debugger. */
volatile unsigned long tim2_services = 0ul;
volatile unsigned long heartbeat = 0ul;
static unsigned int divider_500ms = 0u;

void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & TIM_SR_UIF) != 0u) {
        /* TIM2 UIF is cleared by writing zero.
           Other rc_w0 status flags receive ones. */
        TIM2->SR = (unsigned short)~TIM_SR_UIF;

        tim2_services++;

        divider_500ms++;
        if (divider_500ms >= 500u) {
            divider_500ms = 0u;
            heartbeat ^= 1ul;
        }
    }
}

int main(void)
{
    /* Assumption: TIM2 input clock is nominally 8 MHz (HSI, APB1 divided by 1). */
    NVIC_DisableIRQ(TIM2_IRQn);

    /* Enable TIM2 clock; reset TIM2 to a known configuration.
       This example assumes exclusive ownership of TIM2. */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;
    RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;

    TIM2->PSC = 7u;
    TIM2->ARR = 999u;
    TIM2->CNT = 0u;

    /* Force update so the prescaler takes effect.
       Reset CR1 has URS=0 and UDIS=0. */
    TIM2->EGR = TIM_EGR_UG;
    while ((TIM2->SR & TIM_SR_UIF) == 0u) {
        /* Initialization only: wait for the generated update. */
    }
    TIM2->SR = (unsigned short)~TIM_SR_UIF;

    NVIC_ClearPendingIRQ(TIM2_IRQn);
    NVIC_SetPriority(TIM2_IRQn, 3u);
    TIM2->DIER |= TIM_DIER_UIE;
    NVIC_EnableIRQ(TIM2_IRQn);
    TIM2->CR1 |= TIM_CR1_CEN;

    while (1) {
        /* Main can perform other short, non-blocking work. */
    }
}
