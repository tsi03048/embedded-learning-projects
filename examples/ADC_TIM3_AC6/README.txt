TIM3-triggered ADC1 single conversions / STM32F103VE / ARM Compiler 6.22
Open ADC_TIM3.uvproj, select ADC_TIM3, Rebuild with ARM Compiler 6.
The bundled clock code uses nominal 8 MHz HSI; HCLK=8 MHz and APB1/2 /1
are required. Code reports status 2 and stops if the assumed setting differs.
TIM3 PSC=7999 ARR=99, update TRGO to ADC1 regular channel 0, nominal 10 Hz.
ADC1 input PA0 may be shared with a joystick; check board wiring and voltages.
Watch g_status, g_valid, g_adc_raw, g_sample_count. Status 1 means armed,
not that samples are arriving. Count is serviced EOC events, not trigger count.
This simple polling model does not detect loss, stalled timer, or stale data.
SysTick is exclusively used during startup; timeouts are loop counts.
C compile succeeded. Full Keil build failed because armasm.exe had Windows DLL
error. Link, timer period and board behavior are not verified.
