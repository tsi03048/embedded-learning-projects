GPIO: understand register operations FIRST, functions AFTER.
STM32F103VET6 / Arm Compiler 6.22 / GPIO_Foundation.uvproj

01_LED_On: all GPIO initialization and output writes are visible in main.
To test OFF, change ONLY the final GPIOB->BSRR = 1u << 12 to 1u << 28.
02_Button_Read: all PC0 setup and IDR reads are visible in main.
03_Button_LED: combine the same setup/read/write lines in main.
04_Functions_After: OPTIONAL after understanding targets 1-3. The exact
hardware operations are grouped into functions in gpio.c, declared in gpio.h.
Targets 1-3 do not build gpio.c or include the custom gpio.h header.

External red LED: PB12 -> 680 ohm -> LED anode; LED cathode -> board GND.
Button: PC0 -> normally-open switch -> same GND. Internal pull-up enabled.
Power OFF while wiring. Confirm real connector mapping/shared jumpers.
Use 3.3 V MCU power. Do not connect 5 V to these GPIO pins.
No timer, delay, interrupt enable, debounce or press counter is used.
Watch raw_button only after the IDR assignment executes; released=1/pressed=0.
This package retains ST CMSIS headers and startup provenance. Preserve the
included project settings and read VALIDATION.txt for actual build status.
Actual board wiring/download/operation remain to be verified by the user.
