ADC1 Single Conversion, STM32F103VE, ARM Compiler 6.22

Open ADC_Single.uvproj in Keil uVision. Select ADC_Single and Rebuild.
Use Compiler 6 and keep -std=c99 (not --c99).
The example samples PA0 / ADC1 channel 0 once, then stops in an infinite loop.
In a real board debug session inspect g_status and g_adc_raw:
1 = OK; 3 = startup delay timeout; 4 = reset calibration timeout;
5 = calibration timeout; 6 = conversion timeout; 0 = still waiting.
The raw value is valid only when g_status is 1; 0xFFFF is the invalid marker.
The wait loop limits are iterations, not calibrated times.
Connect only after checking actual pin ownership, reference, voltage and wiring.
PA0 may be connected to a joystick on Open103V: inspect the board schematic.
ADC clock is PCLK2 / 8; 9 MHz when PCLK2 is 72 MHz.
The example uses SysTick exclusively for the initialization delay.
Status: source compilation succeeded; full Keil build failed in this environment
because armasm.exe could not start (Windows DLL error). Hardware not tested.
