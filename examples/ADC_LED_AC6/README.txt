ADC + LED threshold example, STM32F103VE / ARM Compiler 6.22
Open ADC_LED.uvproj, select ADC_LED, rebuild with ARM Compiler 6.
PA0/ADC1 channel 0 input. PB12 drives an EXTERNAL active-high LED with
series current-limiting resistor to ground. Verify voltage, resistor, LED
polarity, pin conflicts, joystick and onboard LED jumpers before wiring.
This is not configured for an assumed onboard active-low LED.
8 non-overlapping samples per group, around 100 ms apart plus processing.
Average >= 2200: LED ON; <= 1900: LED OFF; between: hold state.
LED initializes OFF; on ADC conversion or pacing error it is turned OFF.
Watch g_status, g_adc_raw, g_average_raw, g_average_valid, g_led_on.
C source compiles. Full Keil build could not complete because armasm.exe had
Windows DLL error. Link, timing, electrical conditions and board unverified.
