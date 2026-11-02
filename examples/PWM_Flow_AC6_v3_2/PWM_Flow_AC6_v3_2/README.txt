STM32F103VET6 / TIM2 CH3 -> PA2 (default no-remap) / Arm Compiler 6.22
Open PWM_Flow.uvproj, select ONE target, build/download/reset it separately.
Supplied startup/SystemInit keeps HSI nominal 8 MHz, AHB/APB1 /1.
01_Count_Compare: PSC7999 ARR999 CCR3=250, 1 s period, no GPIO/CC3E/IRQ.
expected_active is a CPU-computed prediction from the sampled CNT, not OC3REF.
02_Slow_Output: same timer, PA2 AF push-pull, nominal High250ms/Low750ms.
03_Fast_Duty: PSC7 ARR999 CCR3=250, nominal 1kHz/25%. Rebuild for other CCR3.
04_Preload: slow 1Hz PWM; edit duty_request in Watch while running if supported.
CPU clamps request to 0..1000 and writes CCR3 preload; normal UEV applies it.
duty_written means last written, NOT physical application acknowledgement.
05_Compare_IRQ: same slow output, fixed CCR3=250; CC3IE -> TIM2 IRQ28.
IRQ records compare match at CNT=250; PWM generation does NOT require IRQ.
UIE and DMA remain disabled; only TIM2_IRQHandler in the fifth target.
TIM2 is exclusively owned. SR=0 clears ALL status, not just one flag.
Preloads OC3PE and ARPE enabled; initial UG applied and setup UIF cleared.
Default AFIO TIM2_REMAP=00 assumes a full MCU reset; no MAPR writes needed.
PA2 -> 680 ohm -> red LED anode -> cathode -> GND, external demonstration.
Disconnect PA2 USART2/module connections, check actual board routing first.
Do not reuse previous PB12 LED or PA0 joystick pin as if they were PA2.
First enabling transient is not a measured complete PWM period; observe steady cycles.
Changing 0%/100% creates steady Low/High; CCR3=999 is 99.9%, 1000 is 100%.
This works because ARR+1=1000 fits the 16-bit CCR; ARR65535 cannot store 65536.
Debugger halt may leave TIM2 running. Watch samples do not measure pulse widths.
Physical board tests/clock accuracy/real output remain for the user later.
Original ST headers/startup sources and notices are preserved.

v3.2 addition (2026-11-01):
06_Controlled_Preload is the timing-observation target for A04-04.
Existing targets 01..05 and their App/CMSIS source files are unchanged.
TIM2 CH3 PA2, HSI nominal8MHz, PSC7999 ARR999, OC3PE=ARPE=1.
Starts CCR3=750; after seeing a normal UEV, one foreground write requests250
while CNT is in [500,550). No runtime UG, no IRQ/DMA/SysTick/new peripheral.
write_cnt_before/after bracket the CCR write. Run continuously without debug
halts; accept a timing trial only if 500<=before<=after<550 and write_done=1.
Expect current cycle High750ms, next cycle High250ms, nominal period1s both.
Reading CCR3=250 or write_done=1 does not acknowledge active-value transfer.
Capture the waveform continuously; RAM records alone do not verify PA2.
Only the new target is rebuilt for this addition. Original five-target build
records are retained separately in VALIDATION_original.txt.
Physical-board timing/output and the effect of actual debugger writes are
not verified; the user will perform these checks later.
