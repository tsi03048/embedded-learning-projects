STM32F103VET6 GPIO - Arm Compiler 6.22
Open GPIO_AC6.uvproj with Keil uVision. For each target select Arm Compiler 6.22.
Three targets: 01_LED_Blink, 02_Button_Raw, 03_Debounce_READY_RUN.
The AC5-only --c99 option was changed to -std=c99 for all targets.
Set your own debug probe, SWD and flash programming algorithm before downloading.
External LEDs: PB12/PB13 through 680 ohms and LED to GND. Verify board jumpers,
shared loads and all connections before applying power. No motor is connected.

Validation: in MDK 5.41 / AC6.22 the three C files compiled for 01_LED_Blink.
The build stopped at startup assembly because armasm.exe failed to run in the
validation environment with a Windows DLL error. Link, all other targets and
physical hardware remain unverified. Save the full build log for any error.
The original legacy ARM-syntax startup is retained; uVision must use its
compatible assembler (auto or legacy armasm), not GNU-only assembly mode.
