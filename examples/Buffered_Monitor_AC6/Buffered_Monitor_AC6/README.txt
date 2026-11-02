Buffered monitoring teaching branch / STM32F103VET6 LQFP100 / Arm Compiler6.22.
Open Buffered_Monitor.uvproj; choose01/02/03. No target uses Compiler5 --c99.
Same PA4pin29 ADC1_IN4 pot (GND..verified VREF+3.3V only), PB12pin51 external
680ohm redLED source circuit, PA9pin68 USART1 TX ->3.3V TTL adapterRX; commonGND.
No adapterVCC/TX connection. Check board shared touch/SPI/DAC/onboardLED first.
HSI nominal8MHz; TIM2 PSC799 ARR9 ->nominal1ms update, serviced IRQ count only.
ADC4MHz,239.5cycles,ch4,single independent, main polling EOC, no ADC IRQ/DMA.
USART1 BRR0x341 ->nominal9603.84, PC9600/8N1/no flow; TX only, no IRQ/DMA.
Acquisition each100tick independent of TX state. Main visits ADC then TX.
Two pending snapshots: q_raw[2]/q_alarm[2]/q_id[2]; q_count0..2 with read/write0..1.
Separate active_raw/active_alarm/active_id and21byte report. Never rebuild
an active line from the changing live raw/alarm. Counters are32bit and wrap.
If full, drop NEW REPORT only; current ADC result/LED still update. FIFO of
accepted reports; backlog can display older states than the current LED.
q_count excludes active frame. queued_total counts admissions, queue_dropped
counts refused new reports, reports_confirmed counts local finalTC not PC ACK.
sample_count==queued_total+queue_dropped (mod2^32). admitted equals pending+
active+confirmed before count wrap; frozen error diagnostics retain active/queue.
01 direct main, no user helper. 02 differs ONLY demo_byte_gap_ticks20 vs0.
Gap gates each DR submission; realbaud unchanged. It is NOT a physical fault.
TXE50tick budget begins AFTER each demo gap. FinalTC50tick after lastDR.
ADC EOC10tick. Ready observed at boundary is accepted; later observation rejected.
Fatal startup/ADC/TXE/TC error latches both paths, no newDR/ADC/dequeue/auto retry.
Raw/LED/queue retained for diagnosis; already-started hardware can finish.
No manual recovery in this isolated queue branch; S09 recovery ZIP preserved.
Combining recovery with queued data requires a fresh pending-data validity policy.
Format D0,raw=0000,state=N CRLF exactly21bytes, no NUL. D0 is fixed branch tag;
sample id is RAM-only, not in wire. Old S09 strict line parser can read normal
complete lines; a board reset midline still needs boundary resynchronization.
No CRC, timestamps, ACK, or cross-reset unique IDs. A terminal alone cannot
infer which sample numbers were dropped. Use queue_dropped/Watch for this lab.
03 OPTIONAL same as01 except six formatting statements move to report_format.c.
report_format.h declaration, C body, array parameter and main call are in lesson.
Only ms_tick is ISR-written. Queue/report owned by main; volatile for Watch is
NOT a lock. Do not move producer/consumer into ISR/RTOS without synchronization.
100000 UG polls are not milliseconds. Tick/main must advance for time budgets;
masked IRQs/debug halts can lose elapsed time. No watchdog/real-time guarantee.
PC/observe_buffer.py uses Python standardlib only;22 rule tests and comparison.
Injected EOC nexttick, TXEready, TC nexttick; NOT ARM/NVIC/ADC/baud/electrical sim.
python PC/observe_buffer.py (from this folder). Actual board validation pending.
