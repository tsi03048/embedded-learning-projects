STM32F103VET6 USART1 IRQ ring buffer / Arm Compiler 6.22
Open UART_Ring_AC6.uvproj; select 01_UART_Ring.
PCLK2 nominal 8 MHz, BRR=0x341, terminal 9600 8N1 no flow control.
3.3V-compatible USB-UART TX -> PA10 RX; common GND; verify power and pin sharing.
Ring has 16 positions, usable capacity 15. ISR writes, main loop reads.
On full or observed UART error: fault set, buffered bytes flushed by main.
This does not restore command framing or count all lost bytes. No transmit.
C files compile under AC6.22. Author environment cannot launch armasm due to
Windows DLL error; full link and actual hardware communication unverified.
