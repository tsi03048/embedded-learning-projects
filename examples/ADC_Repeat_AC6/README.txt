ADC1 repeated single conversions, STM32F103VE, ARM Compiler 6.22
Open ADC_Repeat.uvproj, target ADC_Repeat, rebuild using Compiler 6.
PA0 / ADC1 channel 0; PA0 may share the joystick. Verify actual board wiring.
Watch g_status, g_adc_raw, g_millivolts, and g_sample_count.
Status 1 = latest result is valid; 0 = reading; 3/4/5/6/7 = error.
g_millivolts assumes VREF-=0 V and VREF+=3300 mV. Verify or change VREF_MV.
Approximately 100 ms delay plus processing per sample; not a precise sampling clock.
SysTick is used exclusively. WAIT_LIMIT counts iterations, not milliseconds.
C compile succeeded; full build failed because armasm.exe had a Windows DLL error.
ADC, timing, wiring and hardware operation have not been verified.
