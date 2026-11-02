STM32F103VET6 / TIM2 / Arm Compiler 6.22
Open Timer_Flow.uvproj. Build/download/reset each target separately.
Supplied system_stm32f10x.c keeps HSI nominal 8 MHz, AHB /1, APB1 /1.
TIM2 clock = nominal 8 MHz; no external clock source is required.
01_Counter: PSC=7999, ARR=9999 -> 1000 counts/s, 10 s wrap.
02_Update_Poll: PSC=7999, ARR=999 -> 1 s update; CPU polls UIF.
03_Update_IRQ: same 1 s update, UIE + NVIC IRQ28 -> TIM2_IRQHandler.
TIM2 on this STM32F103 is 16-bit. PSC and ARR must fit 0..65535.
UG loads PSC before counting and creates a setup update (URS=0,UDIS=0).
The code waits for and clears this initial UIF before starting observation.
TIM2 is exclusively owned and used for update only. Writing SR=0 clears
all its status flags; do not reuse that clear policy with other channels.
Snapshot variables are CPU-read copies, not the hardware counters.
UIF/pending is not an event queue. Poll/handler counts may miss updates.
Debugger halt may leave TIM2 running; stopping CPU does not prove timer stop.
No GPIO output, PWM, capture, SysTick, debounce or software timing delay.
The third target adds the timer interrupt directly after the basic two.
Original ST CMSIS/startup source and notices are preserved.
Build/PC checks and physical board measurements are separate.
