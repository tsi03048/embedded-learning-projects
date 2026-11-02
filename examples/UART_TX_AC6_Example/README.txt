STM32F103VET6 USART1 TX first character / Arm Compiler 6.22
Open UART_TX_AC6.uvproj; select 01_UART_TX.
Nominal HSI 8MHz PCLK2=8MHz; BRR=0x341 -> approx 9603.84 baud.
Terminal: 9600 8N1 no flow control. PA9 TX -> 3.3V USB-UART RX; common GND.
Use separate board power; verify voltage, power topology, pin mapping and loads.
After terminal opens, reset MCU to send ASCII A once. No RX, no timeout.
C files compiled with AC6.22; author environment cannot start armasm due to
Windows DLL error. Full link and actual serial communication unverified.
