STM32F103VET6 UART integrated RX interrupt, ring, parser, reply / AC6.22
Open UART_Integrated_AC6.uvproj; select 01_UART_Integrated.
Nominal PCLK2 8 MHz BRR=0x341, 9600 8N1 flow none; LF or CRLF.
3.3V compatible USB-UART TX -> PA10 RX, PA9 TX -> adapter RX, common GND.
After reset send blank LF then LED ON -> OK ON; LED OFF -> OK OFF.
One response slot. Wait for full response before the next command. No timeout;
busy/error/invalid input can produce no response. No actual LED control.
Observed RX fault triggers flush and discard until a later clean LF. An extra
valid line may be discarded; unobserved corruption is not excluded.
C files compile under AC6.22. Author environment cannot run armasm due to Windows
DLL error; full link, hardware communication and concurrency are unverified.
