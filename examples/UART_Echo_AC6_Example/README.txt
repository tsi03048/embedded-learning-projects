STM32F103VET6 USART1 echo / Arm Compiler 6.22
Open UART_Echo_AC6.uvproj; select 01_UART_Echo.
Nominal HSI 8 MHz, PCLK2 8 MHz, BRR 0x341; terminal 9600 8N1 no flow.
PA9 TX -> 3.3V-compatible adapter RX; adapter TX -> PA10 RX; common GND.
Check power, off-state back-powering, pin sharing and correct voltage first.
Disable terminal local echo. Watch rx_count, tx_count, drop_count, error_count.
One-byte pending storage; fast or burst traffic can lose data. TX count is DR
submissions, not final stop bit. Error count is observed errors, not lost bytes.
C files compile in AC6.22. Author environment armasm failed with Windows DLL
error; full link and board/serial behavior remain unverified.
