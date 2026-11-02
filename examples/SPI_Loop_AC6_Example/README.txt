STM32F103VET6 SPI1 external loopback / Arm Compiler 6.22
Open SPI_Loop_AC6.uvproj; select 01_SPI_Loop. Nominal PCLK2 8 MHz, /256,
nominal SCK 31.25 kHz; verify SystemCoreClock and actual clock before testing.
Power off; verify PA7 MOSI and PA6 MISO connector mapping and shared circuits.
Disconnect external drivers/modules before connecting PA7 output to PA6 input.
PA5 is SCK; software NSS selected, no CS or external device used.
Watch spi_state (4 DONE, 5 FAILED), spi_error, spi_rx, spi_match.
A5 is bit-symmetric; loopback does not prove bit order or external device CS.
C files compile with AC6.22; author environment cannot launch armasm due to
Windows DLL error. Full link and actual SPI/board behavior remain unverified.
