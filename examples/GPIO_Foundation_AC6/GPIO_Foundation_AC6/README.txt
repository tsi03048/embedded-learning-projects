GPIO Foundation / STM32F103VET6 / Arm Compiler 6.22

Open GPIO_Foundation.uvproj in Keil uVision. Select one target:
01_LED_On: PB12 LED stays ON. Change gpio_led_on() to gpio_led_off(), rebuild
           and download again to test OFF. No delay loop is used.
02_Button_Read: Watch raw_button; released=1, pressed=0. No LED is needed.
03_Button_LED: LED ON while pressed, OFF while released.

External wiring, power OFF while connecting:
PB12 -> 680 ohm resistor -> red LED anode; cathode -> board GND.
PC0 -> normally-open switch -> board GND. Internal pull-up is enabled.
MCU uses the board's 3.3V supply. Never connect 5V to these pins.
Verify connector pin labels, shared peripherals and board jumpers first.
This package does not assume the built-in LED polarity or built-in key pin.

App/gpio.h: function declarations (the names the main files can call).
App/gpio.c: STM32F1 register configuration (the actual function bodies).
App/01...03...main.c: one application per target. Do not add all three mains
to one target. Existing CMSIS/startup support retains ST copyright notices.

No SysTick initialization, TIM configuration, interrupt enable, debounce,
software delay, press counter or press-to-toggle logic is used by App/.
RCC enables GPIO clocks; this is required GPIO preparation, not a timer lab.
CMSIS/system_stm32f10x.c retains the internal HSI 8MHz setup.

Button contact bounce is intentionally unfiltered. A raw read is not a
debounced press event. Watch is not a waveform recorder; do not infer that
brief changes do not exist just because the debugger does not show them.

See VALIDATION.txt for actual build/verification status. Hardware operation
remains to be checked on the user's board.
