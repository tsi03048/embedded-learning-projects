STM32F103VET6 UART IRQ ring + LF resynchronization / Arm Compiler 6.22
Open UART_Resync_AC6.uvproj; select 01_UART_Resync.
PCLK2 nominal 8 MHz, BRR=0x341; terminal 9600 8N1, LF or CRLF, flow none.
3.3V-compatible USB-UART TX -> PA10 RX, common GND; check power and pin sharing.
After reset send blank line first, then LED ON / LED OFF. Watch led_request,
command_count, sync_count, recovery_count and fault counters. No LED or reply.
Fault handling discards buffered/partial input until a later clean LF; an extra
valid line may be lost. No timeout or guarantee of detecting all corruption.
C files compile under AC6.22; author environment cannot run armasm due to Windows
DLL error. Full link, IRQ race behavior and hardware communication unverified.
