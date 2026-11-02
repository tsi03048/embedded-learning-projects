STM32F103VET6 TIM4 PWM PB6 / Arm Compiler 6.22
Open PWM_AC6.uvproj and select 01_PWM_LED. TIM4 default mapping CH1 PB6.
Nominal HSI 8 MHz, APB1 divide 1, PSC=7 ARR=999 CCR1=300: 1 kHz, 30%.
Change CCR1 to 250, 500, 750 and rebuild for 25%, 50%, 75%.
Before wiring verify PB6 connector, remap and shared circuits (including I2C1).
Example assumes exclusive TIM4 ownership and resets the entire TIM4 peripheral.
C files compiled with AC6.22; armasm launch failed due to Windows DLL error.
Full link, PWM waveform, voltage/current, LED and board operation unverified.
