STM32F103VET6 TIM4 PWM PB6 -> TIM3 Capture PA6 / Arm Compiler 6.22
Open PWM_Capture_AC6.uvproj; target 01_PWM_Capture.
Assumes default TIM3 and TIM4 mapping; both timers reset for exclusive use.
Nominal HSI 8 MHz, APB1 divide 1. Both PSC=7, output ARR=999 CCR1=300.
Before power verify pin mapping, shared circuits and no conflicting drivers.
PB6 to PA6 jumper only with power off; same-board GND is common.
Watch period_us, high_us, duty_percent, measurement_valid, discard_count.
No timeout: valid and last data may remain after signal stops. True period must
be below 65536 capture ticks. Common clock cannot prove absolute frequency.
C source compiles with AC6.22. The author environment fails to start armasm due
to Windows DLL error; full link and hardware behavior remain unverified.
