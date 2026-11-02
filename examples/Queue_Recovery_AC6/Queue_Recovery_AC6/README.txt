Queue plus controlled recovery / STM32F103VET6 LQFP100 / Arm Compiler6.22.
Open Queue_Recovery.uvproj, choose one of4 targets. Compiler5 is not used.
Same PA4pin29 pot ADC1_IN4: GND..verified VREF+3.3V, never5V. PB12pin51 ->
680ohm redLED A->K->GND, HIGH on. PA9pin68 ->3.3V TTL adapterRX, commonGND;
no adapterTX/VCC. Check board DAC/SPI/touch/onboardLED shared nets/jumpers first.
HSI nominal8MHz, TIM2 PSC799 ARR9 serviced1msIRQ count; ADC4MHz/239.5cycles/ch4;
USART1 BRR0x341 nominal9603.84baud, PC9600/8N1/no flow. TXonly, noUART IRQ/DMA.
Normal main: ADC100tick schedule, independent TX state,2pending snapshots plus
1active frame. Each snapshot raw/alarm/id/generation. Full queue drops NEWreport
only and updates LED. skipped_periods and queue_dropped mean different events.
Run states: ACTIVE0,HALTED1,DRAIN2,SYNC_SEND3,SYNC_TC4,ADC_CONFIG5,RSTCAL6,CAL7.
TXstatesIDLE0,SEND1,WAIT_TC2. Errors1RSTCAL,2CAL,3EOC,4TXE,5TC,6UG,
7drainTC,8syncTXE,9syncTC. Runtime error latches both paths, valid0, queue retained.
Set RAM retry_request1 afterHALTED, NOT run_state/error values. One accepted
retry per full MCUboot; onlyruntime errors3/4/5; others consumed/denied.
At request acceptance preserve cause; pending_discarded+=q_count; reset count
and indices but leave SRAM contents for diagnosis. active_abandoned counts the
unconfirmed software snapshot; preserve ID/gen/submitted count. Not PCrollback.
Already submitted bytes may finish, even an entire old valid line. Do not retract
PC history. No newDR inDRAIN. Require actualTC, send CRLF !SYNC CRLF9bytes,
require syncTC, then ADON0,ADC1RST1->0,applyconfig,settle,RSTCAL,CAL,discardcalDR.
Only successful recal increments generation0->1,ready1;valid remains0. Oldraw/LED
stay diagnostic until NEW100tick SWSTART+accepted EOC sets sample_generation1.
Copy that generation to pending slot and active snapshot. Refuse stale slot
instead of tagging oldraw with currentgen. Tags are bookkeeping, not HW detectors.
q_gen guard is normally redundant after queueclear; modelinjects an old slot to
exercise refusal. No arbitrary RAM fault injection target required on board.
Wireformat unchanged21bytes D0/D1,raw=dddd,state=N/W CRLF. snapshotID is RAMonly;
generation oneboot0/1, not unique across powercycles. No CRC, timestamps, ACK.
Any drain/sync/recal failure stops. Cleared old pending data is NOT restored even
if retry fails. Attempt remains consumed. Keep last_error and currenterror.
01 normal: all demo variables0. 02 delay80tick each byte before retry and hideTXE
after4submissions -> firstline D0,r,2pending snapshots at timeout. 03 same plus
hide drainTC. 04 same plus observe CALbusy during retry. Variables mask observed
flags only; real HW can finish. On accepted retry TXE mask and80gap cease by TEST
design, not proof a physical fault clears. Final markers are not deliberately paced.
ADC EOC/RSTCAL/CAL10tick, TXE/TC/drain/sync50tick; ready at boundary wins,
firstready afterboundary rejected. Deliberate gap precedes the TXEtimebudget.
UG100000poll bound is not milliseconds. Tick/main progress required; debug/IRQ
loss can distort elapsedtime. One main owner for ADC/queue/UART, ISR tickonly.
Volatile is for Watch/access, NOT synchronization. No watchdog/RTOS/newperipheral.
Counts modulo2^32: samples=admissions+full_dropped. Beforewrap, admissions=
confirmed+pending+active_valid+pending_discarded+active_abandoned+stale_rejected.
PCmodel starts after initial calibration; separate rule injection, NOT ARM/NVIC/
ADC/baud/reset emulation. python PC/observe_queue_recovery.py;27tests+3scenarios.
Source/electrical behavior on actual board remains pending for user.
