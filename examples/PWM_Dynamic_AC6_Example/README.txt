STM32F103VET6 PWM dynamic duty and input capture / AC6.22
Open PWM_Dynamic_AC6.uvproj; select 01_PWM_Dynamic.
TIM4 PB6 -> TIM3 PA6, verify pin sharing and connect with power off.
Nominal 8 MHz TIM3/TIM4, PSC=7, ARR=999. Debugger Watch: duty_request,
duty_written, period_us, high_us, duty_percent, measurement_valid, discard_count.
Request 300/700/500 for 30/70/50%; requests limited to 100..900.
Live debugger writes require support; halted writes do not prove real-time timing.
No signal timeout: last measurement and valid flag may be stale. Shared clock
cannot prove absolute frequency. Polling may discard samples. TIM3 and TIM4
are reset exclusively. Full link and hardware behavior remain unverified; C
sources compiled under AC6.22 but armasm could not start due to Windows DLL error.
