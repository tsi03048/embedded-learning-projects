B03 Peripheral Clock v3.2 - STM32F103VET6 / LQFP100

Prerequisites: B02 clock path and A03 basic TIM2 counter. Fresh independent reset for each target.
Open Peripheral_Clock.uvproj, choose AC6.22 and one target, Rebuild. -std=c99, -O0, STM32F10X_HD, Flash512KB/SRAM64KB, nominalHSI8MHz.
01_Enable_Start_Divide: phase1 TIM2EN1/CEN0/CNT0; request1 startsCEN; request2 changes onlyPPRE1 to/4, phase3.
Expected nominal TIM2 input8->4MHz, PSC7999 -> counter1000->500Hz. HCLK stays8MHz; PCLK1 becomes2MHz. This is not an APB/2 example, sinceAPB/2 leaves the timer input8MHz.
02_Gate_And_Reset: phase1 CEN0,CNT1234,PSC7999,ARR9999. request1 gateoff phase2; request2 gateon and snapshot phase3; request3 holdsTIM2RST1 phase4; request4 releasesRST0 and snapshots reset defaults phase5.
Expected after gate cycle: same stopped marker/config. Expected after reset release:CEN0,CNT0,PSC0,ARR65535. TIM2EN stays1 during reset. CPU/main/RAM control state is not reset.

Gateoff phase2 and reset-held phase4 contain NO TIM2 accesses. Close automatic TIM2 peripheral-register windows and do not add TIM2 register expressions toWatch during these phases. Watch the RAM snapshots and RCC bits instead.
Do not use a zero read while gated as proof of reset. RM0008 APB1ENR note describes unavailable reads returning0.
The stopped-marker example tests configuration retention vsreset, not the elapsed-time behavior of a running gated counter.
UG loadsPSC and reinitializesCNT. Code clearsSR first, waits within a finite iteration budget for initializationUIF, clearsUIF, then sets1234 in target02. DIER0; no timer interrupt orDMA enabled.
app_error1 startup clock assumptions mismatch;2 initializationUIF not observed within10000 iterations. This budget is not milliseconds.

request is a RAM control set by the debugger in order. Stop after the phase assignment, record, editrequest andRun. A breakpoint on an assignment may stop before that assignment executes.
Counters can behave differently during debugger halt depending on debug freeze settings. Watch refresh speed is not a frequency measurement.
No UART/GPIO output, PWM, DMA, low-power mode or reset of the whole MCU is configured by these app sources.
These are standalone teaching examples, not arbitrary clock-off/reset operations for an integrated project with live transfers.
Compiler/linker status is separate from real board/register retention/rate/current/reset measurements. Actual board verification has not been performed.
Existing learning projects and ZIPs are unchanged. Original ST CMSIS and startup notices are retained.
