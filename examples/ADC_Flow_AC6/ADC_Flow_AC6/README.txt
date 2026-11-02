STM32F103VET6 / ADC1 channel4 PA4 / Arm Compiler 6.22.
Open ADC_Flow.uvproj, select ONE target, build/download/full MCU reset.
Supplied CMSIS SystemInit: HSI nominal8MHz, PCLK2=8MHz, ADCPRE/2 => ADC4MHz.
PA4 analog; regular sequence one rank: channel4; independent, right aligned.
SMP4=239.5 cycles; conversion=239.5+12.5=252 cycles=nominal63us.
Not a software loop period guarantee. CONT=0 in all three targets.
After ADON=1, inline1024 NOP loop exceeds 1us tSTAB and two ADC clocks.
RSTCAL then CAL; wait hardware clear. Discard calibration DR before measuring.
EXTSEL=111, EXTTRIG=1, SWSTART=1 starts each conversion, EOC marks complete.
Read DR clears EOC. adc_raw=0xFFFF initially is NOT a valid result.
01_Single_Poll: known GND input first; one result/count1 per reset then idle.
02_Repeat_Poll: same PA4 with pot; raw varies, voltage_mv=raw*reference_mv/4096.
reference_mv=3300 is assumed; VREF-=0; integer estimate not measured accuracy.
03_Single_IRQ: one software trigger; ADC1 EOCIE -> ADC1_2 IRQ18 -> NVIC[0]bit18.
ISR reads DR/count++; main only main_loops; ADC2 stays reset/off.
No timer, SysTick, DMA, LED, UART, scan, injected group, user init/task helpers.
Blocking waits intentionally show hardware sequence; stuck wait is NOT success.
Production timeout/recovery is later work, not implemented here.
Wiring: verify VET6 LQFP100 board VREF+/VDDA and common analog ground.
Input must stay within GND..VREF+, never 5V. Remove other PA4 output drivers.
10k pot ends: verified same3.3V supply as analog reference and GND; wiperPA4.
Begin with PA4 tied GND, not an uncontrolled/floating source.
Watch RAM adc_ready/adc_raw/sample_count, NOT ADC_DR (debug reads clear EOC).
Breakpoints disturb continuous observation. No actual board verification done.
ST CMSIS/startup notices retained. User will verify actual reference, pins,
voltage readings, acquisition settling and real interrupt entry/return later.
