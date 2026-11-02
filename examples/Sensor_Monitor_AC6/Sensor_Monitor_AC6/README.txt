STM32F103VET6 / first voltage monitor / Arm Compiler6.22.
Open Sensor_Monitor.uvproj. Choose one target, Rebuild, download, full reset.
Provided CMSIS SystemInit uses HSI nominal8MHz, AHB/APB1/APB2 /1.
PA4 analog ADC1_IN4, LQFP100 pin29. Pot10k ends: verified VREF+ supply3.3V
and GND, wiperPA4. VREF-=GND. Check actual VREF/VDDA/connector/sharing.
PB12 pin51 ->680ohm ->redLED A->K->GND, HIGH=ON, same prior externalLED.
Remove conflicting PA4 DAC/SPI NSS and PB12 LED/touch/SPI2 output drivers.
Target04 adds PA9 pin68 ->3.3V TTL USB-UART RX, GND common; no adapterTX
or VCC wire needed. PC9600/8N1/no flow control. NOT RS232/5V logic.
ADC PCLK2/2=4MHz, SMP4=239.5cycles, conversion total252cycles=63us nominal.
ADC startup/calibration sequence reused from validated ADC lesson.
01: repeat SW single conversions, raw>=2600 warningLED, otherwise normal.
02: upper>=2600 enters warning, lower<=2400 leaves, band holds priorstate.
Initial alarm0; first valid band sample chooses normal by explicit policy.
03: add TIM2 PSC799/ARR9 nominal1ms servicedIRQ count. Main services100tick
software slots and requests ONE present-time ADC conversion. No TRGO or
ADC hardware timer trigger, no fabricated catchup samples. Missed software
slots increment skipped_periods, while nominal software phase is retained.
04: same03 plus18 ASCII bytes raw=0000,state=N\r\n after every sample.
No terminator transmitted. Baud8MHz/833=9603.84;180bits=18.7425ms nominal.
Polling TX blocks main. TimerIRQ remains enabled. This budget is a simple
nominal example, NOT a measured scheduling or worst-case response guarantee.
ms_tick counts serviced IRQs. Masking/debug halt may coalesce updates;
skipped_periods cannot recover physical elapsed time lost in one UIF flag.
adc_ready means configured/calibrated, sample_valid means a result exists.
Before first sample LEDoff is unvalidated startup, NOT proof of normal input.
voltage_mv=raw*3300/4096 is integer estimate with assumed reference, not
precision calibration. Thresholds are learning choices, not a sensor spec.
No button, extra sensor, ADCIRQ, USARTIRQ/RX, DMA, filter, CRC, storage, RTOS.
Inline main register setup/read/judgment/write. No user init/task wrapper.
Normal-path ADC/UART waits have no timeout/recovery; follow-up work required.
Watch RAM values, not ADC_DR: debugger DR reads can clear EOC.
PC Python rules model is separate; it does not execute ARM C or hardware.
Real board validation has NOT been performed. User will validate later.
