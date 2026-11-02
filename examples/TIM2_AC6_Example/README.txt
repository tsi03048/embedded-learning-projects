STM32F103VET6 TIM2 1 ms example / Arm Compiler 6.22
Open TIM2_AC6.uvproj and select 01_TIM2_1ms.
Project SystemInit uses nominal HSI 8 MHz and APB1 divide by 1: TIM2 clock 8 MHz.
PSC 7, ARR 999: nominal 1 ms. Changing clock requires recalculation.
Observe tim2_services and heartbeat in debugger; no LED is driven.
C files compiled with AC6.22; author environment cannot launch armasm due to
Windows DLL error. Link and hardware behavior not verified.
