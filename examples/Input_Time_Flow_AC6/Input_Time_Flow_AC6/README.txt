STM32F103VET6 / GPIO+TIM2, optional EXTI / Arm Compiler6.22.
Open Input_Time_Flow.uvproj. Select ONE target; build/download/full MCU reset.
Same prior circuit: PB12 ->680ohm -> red LED A->K->GND (HIGH=ON).
PC0 switch toGND, internal pull-up (released1/pressed0). Check real board
connector/pin sharing; isolate onboard LED/SPI2/touch drivers as appropriate.
Supplied SystemInit keeps HSI nominal8MHz, APB1 /1 -> TIM2 clock8MHz.
PSC799/ARR9 -> nominal1ms update. UG loaded then setupUIF cleared before UIE.
TIM2 IRQ28 clearsSR by0 (dedicated update-only timer) and increments ms_tick.
ms_tick is SERVICED IRQ count. Long masking/debug halt can merge UIF updates;
it is not an unconditional wall clock or a queue of missed timer events.
01_Timed_LED: startoff, main toggles every>=500 servedticks, nominal500ms.
Main does not block. Late service toggles once; no fake catch-up flashes.
02_Stable_Follow: samplePC0 once per observed newtick; changed candidate starts
age0, confirm after20tick difference with no skipped tick observations.
First and final observation span20 intervals (21 samples with1ms spacing).
20 is a learning parameter, not a silicon specification/guaranteed debounce.
03_Press_Toggle: only confirmed stable1->0 increments press_count/togglesLED.
Hold does not repeat. Confirm release before nextpress. Stable boot input is
baseline, not a synthetic newpress. In03/04/05 LED startsOFF even if held.
Main samplegap>1 restarts candidate evidence; missed history is NOT filled
with repeated copies of currentGPIO. sample_gaps cannot detect maskedIRQ
time lost when multiple timer events coalesce into oneUIF service.
04_EXTI_Compare: same03 logic + PC0 both-edge EXTI0 IRQ6. ISR only clearsPR
by1/countsservices. EXTI does NOT gate sampling or generate press events.
exti_services is not a guaranteed physical edge count because pending coalesces.
05_Function_Compare: same03 algorithm; ONLY finalLED BSRR branch wrapped in
led_write, with declaration/body/call shown. GPIO/TIM2/debounce remain visible.
No SysTick, UART, ADC, PWM, DMA, longpress/doubleclick, RTOS or motor control.
PC model uses injected tick/raw rows, not compiled C/hardware/NVIC execution.
Run Python3 PC/observe_button.py for boundary/gap/wrap tests and trace.csv.
No real-board validation yet; user will measure wiring/timing/bounce later.
