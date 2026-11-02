STM32F103VET6 / USART1 / Arm Compiler 6.22
Open UART_Flow.uvproj. Select ONE target; build/download/full MCU reset.
Provided SystemInit keeps HSI nominal 8MHz / APB2 /1. USART1 PCLK2=8MHz.
No timer clock doubling for USART1. Fixed x16 oversampling on this F1.
BRR0x0341 = mantissa52, fraction1/16; nominal baud=8000000/833=9603.84.
All targets: PC terminal 9600 baud / 8 data / no parity / 1 stop / no flow control.
Local echo OFF, send one ASCII byte at a time; no automatic CR/LF for simple tests.
PA9 USART1_TX -> 3.3V-compatible adapter RX; PA10 USART1_RX <- adapter TX.
Common GND; VCC/power lines not included in this demonstration connection.
Check board revision, actual adapter signal voltage and shared drivers/jumpers.
Default no-remap after full reset. No AFIO_MAPR writes.
PA9 AF push-pull 2MHz grade; PA10 input pull-up (ODR10=1, CNF=10/MODE=00).
01_TX_One: A exactly once per reset. Open terminal before resetting.
tx_submitted means DR write; tx_complete waits TC, NOT receiver acknowledgement.
02_RX_Observe: receive only, last_rx/rx_count; terminal response remains empty.
03_Poll_Echo: elementary blocking TXE wait; not a lossless streaming driver.
04_RX_IRQ: receiver only; USART1 IRQ37 -> NVIC[1] bit5 -> USART1_IRQHandler.
ISR alone reads DR. Stores latest byte and count, NO queue/ring buffer/echo.
SR snapshot then DR read clears RXNE and observed ORE/NE/FE by documented sequence.
Observed error => discard the DR read and increment rx_error_events, not lost-byte count.
No SR=0 or new-series ICR/RDR/TDR registers; this F1 uses SR and dual-purpose DR.
Do not auto-refresh USART1 DR in debugger; reading it changes receive state.
Breakpoints can cause overrun. Watch RAM variables, not DR; type slowly.
No timers, GPIO LED control, DMA, parser, printf or user init/task wrappers.
Original ST CMSIS/startup sources and notices retained.
Build and PC logical checks do not prove physical UART or HSI accuracy.
User will verify wiring, terminal data, waveform and real IRQ entry/return later.
