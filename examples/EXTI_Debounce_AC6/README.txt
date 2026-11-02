STM32F103VET6 / Arm Compiler 6.22: interrupt and 20-sample debounce
Open EXTI_AC6.uvproj, target 01_EXTI_Debounce. PC0 button to GND, internal pull-up.
EXTI tracks IRQ handler services; SysTick is polled independently for debounce.
Use debugger Watch: exti_services, confirmed_presses, raw_level, stable_level.
Main and debounce C files compiled with armclang 6.22. Full IDE build, link,
startup and actual board operation have not been verified. COUNTFLAG loses ticks
when execution is halted or delayed. No guarantee of exact press/edge counting.
