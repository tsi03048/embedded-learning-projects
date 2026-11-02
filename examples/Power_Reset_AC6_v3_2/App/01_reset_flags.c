#include "stm32f10x.h"

volatile uint32_t boot_csr = 0U;
volatile uint32_t boot_reset_flags = 0U;
volatile uint32_t boot_pin = 0U, boot_por = 0U, boot_software = 0U;
volatile uint32_t boot_iwdg = 0U, boot_wwdg = 0U, boot_low_power = 0U;
volatile uint32_t capture_done = 0U;
volatile uint32_t clear_request = 0U;
volatile uint32_t clear_done = 0U;
volatile uint32_t after_clear_csr = 0U;
volatile uint32_t after_clear_flags = 0U;

int main(void)
{
    boot_csr = RCC->CSR;
    boot_reset_flags = boot_csr & 0xFC000000U;
    boot_pin = (boot_csr & RCC_CSR_PINRSTF) != 0U;
    boot_por = (boot_csr & RCC_CSR_PORRSTF) != 0U;
    boot_software = (boot_csr & RCC_CSR_SFTRSTF) != 0U;
    boot_iwdg = (boot_csr & RCC_CSR_IWDGRSTF) != 0U;
    boot_wwdg = (boot_csr & RCC_CSR_WWDGRSTF) != 0U;
    boot_low_power = (boot_csr & RCC_CSR_LPWRRSTF) != 0U;
    capture_done = 1U;

    while (1)
    {
        /* Set this RAM request to 1 in the debugger after recording boot_csr. */
        if ((clear_request == 1U) && (clear_done == 0U))
        {
            RCC->CSR |= RCC_CSR_RMVF;
            after_clear_csr = RCC->CSR;
            after_clear_flags = after_clear_csr & 0xFC000000U;
            clear_done = 1U;
        }
    }
}
