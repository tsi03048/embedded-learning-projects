STM32F103VET6 / PC0 switch / Arm Compiler 6.22
Open EXTI_Flow.uvproj. Select 01_Falling, then 02_Rising separately.
PC0 -> normally-open switch -> same board GND. Internal pull-up enabled.
The GPIO lesson LED wiring may stay; this project never configures PB12.
No timer, time delay, periodic sampling or debounce algorithm is used.
All initialization is visible in main. The actual EXTI0 handler is visible.
Watch exti0_services (serviced EXTI0 requests) and main_loops (main progress).
main_loops is an unsigned wraparound counter, not a time measurement.
Start each experiment from reset, with the switch released while setup runs.
CPU PRIMASK/BASEPRI/FAULTMASK remain at their reset values in this project.
Initial pending state is intentionally discarded before enabling EXTI0.
One physical press may cause multiple services; pending is not an event queue.
Never describe this as a count of debounced button presses.
Startup's vector EXTI0 slot is 0x58 from __Vectors; its IRQ number is 6.
AC6 startup uses integrated assembler-with-cpp; retain the file settings.
Original ST CMSIS/startup notices are retained.
Build, PC checks and physical board validation are recorded separately.
