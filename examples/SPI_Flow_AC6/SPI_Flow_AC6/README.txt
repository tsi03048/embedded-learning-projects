STM32F103VET6 / SPI1 external loopback / Arm Compiler6.22.
Open SPI_Flow.uvproj. Select ONE target, build/download/full MCU reset.
Supplied SystemInit keeps HSI nominal8MHz and PCLK2=8MHz. SPI BR=111 /256:
SCK nominal31.25kHz, bit32us, 8-bit frame256us excluding software gaps.
PA5 SCK and PA7 MOSI: AF push-pull 2MHz output speed grade.
PA6 MISO: floating digital input; PA4 is not configured as external CS.
External jumper PA7->PA6, NOT internal loopback. Disconnect other modules/
drivers, power off before wiring. Scope ground/common reference as appropriate.
Default SPI1_REMAP=0 is assumed after FULL MCU reset. No AFIO_MAPR writes.
SSM=1/SSI=1 provide internal NSS HIGH, not an external device chip-select.
Master, two-line full duplex, mode0, 8-bit, MSB first, CRC disabled.
CR1 settings are written before SPE enable. No changing mode while active.
F1 DR access via supplied uint16_t CMSIS field; DFF0 uses lower8 data bits.
01_Loop_One: send96 once, receive96, rx_count1/matches1/transfer_done1 expected.
02_Loop_Patterns: 00 FF 96 3C, receive same four; rx_count4/matches4 expected.
Read each Rx buffer before starting the next frame; final TXE then BSY0.
03_RX_IRQ: one96 frame; RXNEIE -> IRQ35 -> NVIC[1]bit3 -> SPI1_IRQHandler.
ISR alone reads DR. Main waits for RAM count, then finalTXE/BSY; main_loops grows.
TXEIE, ERRIE and DMA stay0. No error recovery or lossless stream implemented.
Blocking waits show the hardware sequence; no timer/SysTick/timeout helper.
If stuck, inspect TXE/RXNE/MODF/OVR and configuration, not 'test passed'.
Watch RAM, NOT automatic SPI_DR refresh (DR reads clear RXNE).
Loopback cannot validate an external slave, CS, command or independent bit order.
96 MSB-first is10010110; check physical waveform to confirm bit order/mode.
Existing AT45DB041E case is a DOCUMENT READING example; actual chip not confirmed.
This ZIP does not execute external Flash ID commands or write/erase memory.
ST CMSIS/startup notices retained. No real-board tests have been performed.
