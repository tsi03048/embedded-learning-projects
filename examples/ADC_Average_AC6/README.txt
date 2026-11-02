ADC block average, STM32F103VE, ARM Compiler 6.22
Open ADC_Average.uvproj, select ADC_Average, rebuild with Compiler 6.
8 successive conversions per non-overlapping group. Integer division truncates.
Watch g_status, g_adc_raw, g_average_raw, g_average_valid, g_average_count.
g_average_valid=0 before the first complete group and after a fatal error.
Once valid, the average remains the last completed group until the next group completes.
Genuine clock interval includes processing and debugger pauses.
PA0 may be connected to the Open103V joystick: verify schematic and voltage.
C compilation succeeded. Full build failed because armasm.exe had a Windows DLL error.
No firmware link or board measurement was verified.
