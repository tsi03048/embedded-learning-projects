STM32F103VET6 TIM2 button debounce / Arm Compiler 6.22
Open TIM2_Button_AC6.uvproj; select 01_TIM2_Button.
PC0: button to GND; internal pull-up. Confirm board pin and shared circuitry.
Nominal HSI 8 MHz, APB1 divide 1, TIM2 PSC=7 ARR=999, 1 ms updates.
Watch timer_services, raw_high, stable_high, press_count.
20 consecutive differing samples confirm a state change. No EXTI or LEDs.
C files compile on AC6.22. Author environment armasm fails with a Windows DLL
error: full link and hardware timing/behavior remain unverified.
