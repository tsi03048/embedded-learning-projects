STM32F103VET6 / Arm Compiler6.22 / controlled single recovery per MCU boot.
Same PA4 ADC1_IN4 pot, PB12 external680ohm sourceLED, optional PA9 USART1 TX.
Check same VET6 pin/board/reference/sharing conditions as P01. PC9600/8N1.
HSI8MHz, ADC4MHz/239.5cycles, TIM2 PSC799/ARR9/serviced1msIRQ count.
Runtime waits retain <=10ADC/<=50UART ready-at-boundary observation policy.
Set RAM retry_request=1 only after PH_ERROR to REQUEST a recovery.
Accept only EOC3 / (UART variant also TXE4 or TC5), and retry_attempts==0.
Consume other requests; increment retry_denied. No auto retry, no queuing.
Keep last_error_stage/tick, last raw/LED. Clear current error ONLY when ADC
reconfiguration/calibration completes. Valid remains0 until a NEW conversion.
ADC_CONFIG first powers off ADON, then asserts/releases RCC ADC1RST, reapplies single independent ADC1,
power stability, RSTCAL, CAL; discard calibrationDR/SR, no publishing oldEOC.
RCC reset concerns ADC1 only, not fullMCU. ADC2/off, IRQ/DMA off, single owner.
Do NOT generalize this reset to shared DMA/dualADC/concurrent drivers.
UART recovery never forces UE/TE off midframe. First PH_DRAIN waits realTC
<=50ticks with NO newDR writes; if missing latchERR7 and refuse furtherretry.
Then send9bytes CR LF ! S Y N C CR LF, oneTXE-per-main-pass, finalTC required.
Only then reconfigureADC. If markerTXE/TC fails latchERR8/9, no fresh data.
Wire format CHANGED from previous18bytes: D0,raw=2600,state=W CR LF is21bytes.
Generation0 initial,1 after one successful reinit; one ASCII digit is enough
ONLY because retries capped at1. Generation is NOT unique across MCU resets.
Host strict line rule accepts only exact D[01],raw=dddd,state=[NW] CRLF,
raw0..4095. SYNC is control, not data, cannot retract already accepted records.
Delimiter/grammar repair controlled partial lines, NOT arbitrary bit corruption,
CRC protection, PC ACK or certified recovery from physical faults.
01_ADC_Recovery normalADC;02_UART_Recovery normalADC+UART.
03_ADC_First_Timeout hides firstEOC before retry;04_UART_Partial_Retry hides
TXE after4submittedbytes before retry (D0,r prefix);05 same +hide drainTC.
Observation masks exercise software branches; actual HW can finish normally.
After manual retry in03/04, mask stops at retry_attempts1 by TEST design.
Removing a test mask is not proof that a physical fault has cleared.
reinit_completed1/generation1 mean ready-again, NOT sample_valid1 yet.
sample_generation assigned only after DR accepted. Tags do not identify stale
hardware data by themselves; reset/configuration/start order is the guarantee.
Cold init RSTCAL/CAL/TIM2 faults1/2/6 are not eligible for this limited retry.
TimerUG has100000poll bound, not milliseconds. Tick/main must advance; masked
IRQ/debugging can lose elapsed time. No independent watchdog added.
No MCU RX command, new button, USART IRQ, DMA, ringbuffer or user init/task helper.
Firmware code in main is direct. Board validation remains for user.
PC model/stream parser use injected observations/bytes, NOT ARM/peripheral/baud.
