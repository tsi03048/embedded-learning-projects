STM32F103VET6 / I2C1 PB6 SCL PB7 SDA / Arm Compiler6.22.
Open I2C_Flow.uvproj, select ONE target, build/download/full MCU reset.
Supplied SystemInit keeps HSI nominal8MHz/PCLK1=8MHz. I2C standard mode:
FREQ8, CCR50 -> nominal80kHz; TRISE9 -> standard rise-time max1us setting.
80kHz deliberately below88kHz per ES0340 Rev17 2.9.5 repeated-START issue.
External pull-ups to3.3V, e.g.4.7k per line for short isolated test bus;
Reset MCU only after confirmed module VCC is stable; inline1024 NOP startup
wait then exceeds nominal100us AT24C02D tPUP (not a generic delay service).
check actual equivalent resistance/capacitance/rise time, existing module pulls.
PB6/PB7 AF open-drain2MHz grade, not push-pull; pull-ups generate HIGH.
No wire short between SDA and SCL. Actual device model/module unconfirmed.
01_Idle_Lines: needs pull-ups only; expect SCL1/SDA1/BUSY0. No START.
02_Address_Poll: single address50 Write probe, no data. ACK or NACK separately
recorded. Disconnected target with pull-ups should NACK. No bus/address scan.
04_Address_IRQ: same probe through I2C1_EV IRQ31 and I2C1_ER IRQ32. Buffer IRQ
ITBUFEN=0. Expected AF/address NACK is recorded separately from other errors.
03_Read_One is CONDITIONAL on actual AT24C02D normal/ready, A2/A1/A0=000,
address probe ACK, verified package/power/wiring. SOT23 has fixed address000.
Read byte at EEPROM word12; unknown value, no automatic comparison to3C/FF.
Random-read sequence: S A0 ACK 12 ACK Sr A1 ACK data NACK P,36 clock pulses.
Pointer setup followed by repeated START does not program EEPROM data.
One-byte receive uses RM0008 Figure277 EV6_3 and ES0340 Rev17 2.9.1
workaround3: ACK0, hold PB6 low as GPIO OD, SR1->SR2 clear ADDR, STOP1,
restore PB6 AF OD, waitRxNE/readDR, STOP complete/BUSY0, then publish result.
Errata prints SR3; actual F103 register is SR2, confirmed by RM0008/header.
Do not generalize this ONE-byte receive to2/many bytes. No SW time delay/timer.
Do NOT auto-refresh SR1/SR2/DR: SR1->SR2 clearsADDR, DR read changes Rx state.
Normal-path waits have no timeout, automatic recovery or retry. If stuck,
inspect source location and error bits; no valid result is reported.
BUSY stuck with physical lines HIGH can relate to ES0340 2.9.7 analog filter.
No automatic GPIO bus-clear or filter workaround implemented by these targets.
No EEPROM program/erase, sensor format conversion, DMA, SysTick or user init
helper functions. ST CMSIS/startup notices retained. Board tests NOT done.
