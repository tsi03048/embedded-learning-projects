ADC + LED PC test (Windows x64)
Run adc_test.exe from PowerShell: .\adc_test.exe
A nonzero exit code indicates a test failure.
To rebuild, open the x64 Native Tools Command Prompt for Visual Studio in this
folder and run: cl /nologo /std:c11 /W4 /WX test_adc.c /Fe:adc_test.exe

The group averaging and LED threshold section was copied from the ADC_LED
firmware. GPIO is replaced with a PC variable and timeout is injected.
The test uses simulated raw codes, not ADC conversion or physical hardware.
It does not test STM32 peripherals, timing, firmware linking, or LED wiring.
