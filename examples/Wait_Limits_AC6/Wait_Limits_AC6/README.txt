STM32F103VET6 / same PA4 pot, PB12 external680ohm LED, PA9 UART TX.
Arm Compiler6.22; HSI nominal8MHz, APB1/APB2 /1. ADC4MHz/ch4/239.5cycles.
TIM2 PSC799/ARR9: servicedIRQ tick, nominal1ms, started BEFORE ADC calibration.
TimerUG startup has bounded100000 poll iterations (NOT a millisecond budget).
RSTCAL/CAL startup waits use10ticks; runtime ADC waits return to main.
Runtime phase0 IDLE,1 WAIT_ADC,2 SEND,3 WAIT_TC,4 ERROR.
Error0 none,1 RSTCAL,2 CAL,3 ADC_EOC,4 UART_TXE,5 UART_TC,6 TIMER_UG.
Observation policy: ready and age<=limit accepted; ready exactly at limit wins.
Notready at age>=limit faults. First ready observation AFTER limit is rejected.
This bounds software observation; it does NOT timestamp when HW completed.
ADC deadline10ticks; UART inactivity50ticks PER submission, finalTC50ticks.
Not a50tick whole-report deadline. Main samples on100tick software slots.
Only one pipeline runs: acquire->LED->optionalreport; not parallel acquisition.
sample_valid0 at each newrequest; accepted DR result sets1. Error ADC leaves
last raw/LED unchanged and invalid. UART error retains valid acquired sample.
report_index counts bytes SUBMITTED toDR; reports_confirmed counts finalTC,
not PC receipt/ack. A completed line on PC does not prove software saw TC.
ERROR latches; mainloops/timer continue. No new ADC request/DR access inERROR.
Already-started ADC/UART HW may finish. No auto-retry/flush/lateDR read.
Recovery in this lesson is full MCU reset after diagnosis, not a software retry.
01_ADC_Bounded: ADC+LED runtime wait and deadline only, noUART.
02_UART_Bounded: same plus one-byte-per-main-pass TXE, finalTC confirmation.
03_ADC_Observation_Fault: hide observedEOC with test_hide_eoc1.
04_TXE_Observation_Fault: hide TXE after4 bytes submitted, test_txe_stall_after4.
05_TC_Observation_Fault: hide observedTC with test_hide_tc1.
Fault targets mask SOFTWARE observations; peripherals/physical flags are not
deliberately broken. These are branch exercises, NOT real device fault tests.
Defaults force one named fault after reset. Reset each target; don't disable
NVIC/timebase or disconnect signal wiring to pretend a status timeout.
Timer/IRQ masking/debug halt can coalesce updates: tick deadlines are conditional
on progressing software time and main revisits. They are NOT hard wall-clock
limits, clock-failure protection or watchdogs. Frozen tick can freeze timeout.
Use same VET6 board wiring checks as P01. PC9600/8N1/no flow control.
Watch RAM, not ADC_DR (debug reads clearEOC). Board tests remain for user.
PC rule model is separate from compiled ARM C/peripheral/electrical execution.
