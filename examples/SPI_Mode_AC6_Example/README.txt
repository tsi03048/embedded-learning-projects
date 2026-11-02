STM32F103VET6 SPI mode 0 / mode 3 loopback / Arm Compiler 6.22
Open SPI_Mode_AC6.uvproj; select 01_SPI_Mode. Default SPI_MODE=0 and TEST_BYTE
0xA6. Change SPI_MODE to 3 in App/main.c, rebuild and download to try mode 3.
Each program sends one byte after reset; inspect spi_state, spi_rx, spi_match.
Nominal 8 MHz PCLK2 / 256 = 31.25 kHz. PA7 MOSI -> PA6 MISO external jumper.
Turn off power and verify pins, shared loads and conflicts before connecting.
Loopback cannot prove bit order or real peripheral mode compatibility. Observe
PA5 SCK and PA7 MOSI with suitable analyzer for actual idle and sampled bits.
Mode 0 and mode 3 C sources compile on AC6.22; author environment cannot start
armasm due to Windows DLL error. Full link and actual signals unverified.
