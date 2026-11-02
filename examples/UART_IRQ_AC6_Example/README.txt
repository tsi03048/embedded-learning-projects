STM32F103VET6 USART1 RX interrupt one-byte handoff / Arm Compiler 6.22
Open UART_IRQ_AC6.uvproj; select 01_UART_IRQ.
PCLK2 nominal 8 MHz, BRR 0x341, terminal 9600 8N1 no flow control.
3.3V compatible USB-UART TX -> PA10 RX; common GND. Check pin mapping,
power and off-state input conditions before wiring. This example never transmits.
Watch rx_count, processed_count, last_processed, slot_drop_count, uart_error_count.
One-byte slot; fast data can be lost. Counters are not complete loss detection.
C files compile with AC6.22. Author environment cannot launch armasm due to
Windows DLL error; full link and actual UART/NVIC behavior remain unverified.
