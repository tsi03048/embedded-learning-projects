STM32F103VET6 UART line receiver / Arm Compiler 6.22
Open UART_Line_AC6.uvproj; select 01_UART_Line.
PCLK2 nominal 8 MHz, BRR=0x341, terminal 9600 8N1 flow none, LF or CRLF.
USB-UART TX -> PA10 RX; common GND; verify 3.3 V signals and power first.
Send LED ON / LED OFF, inspect led_request, command_count and last_result.
No echo or response; GPIO LED is not controlled. Max 15 printable ASCII bytes.
Overflow, invalid input or observed UART error discards through next valid LF.
No timeout; RX polling may miss data. Not a safety or command integrity guarantee.
C files compiled under AC6.22. Author environment armasm launch failed due to
Windows DLL error; full link and actual hardware communication unverified.
