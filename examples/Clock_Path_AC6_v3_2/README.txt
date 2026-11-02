B02 Clock Path v3.2 - STM32F103VET6 / LQFP100

Open Clock_Path.uvproj, choose AC6.22 and one target, Rebuild. Options -std=c99, -O0; no --c99.
All targets start separately from supplied nominalHSI8MHz SystemInit. Download and reset for each target.
01_HSI_APB_Divide: inspect baseline, set apply_request RAM to1; only APB1 divider changes1->2. SYSCLK/HCLK/PCLK2 stay nominal8MHz, PCLK1 becomes4MHz, APB1 timer path remains8MHz.
02_HSE_Switch: requires a confirmed8MHz crystal circuit, HSEBYP=0. HSEON -> HSERDY -> SW_HSE -> SWS_HSE. Frequency nominal8MHz stays the same while source changes.
03_PLL72_Switch: same crystal, FLASH2WS/prefetch first, APB1/2, ADC/6, HSE/1 x9, then switch afterPLLRDY and confirmSWS.
03 final assumed frequencies: SYSCLK/HCLK72MHz, PCLK136MHz, PCLK272MHz, ADC12MHz, active APB timer inputs72MHz. Timer/ADC peripherals are NOT enabled by this app.

Ready loops use a finite poll count1,000,000, NOT milliseconds. Debug pauses and compiler/code changes affect elapsed time.
clock_error1: startup assumptions not met;2: HSE readiness not observed in budget;3: PLL readiness not observed;4: switch status not confirmed;5: FLASH preparation readback mismatch.
On errors no further application work is performed. No automatic rollback. Sources remain enabled; SWS may change later after a pending switch.
Before SW is requested an HSE/PLL-ready error leaves the initially selectedHSI. After SW request an error does not proveHSI is still selected.
final_* are a one-time record; live_* are later sequential snapshots. Registers/fields are not read simultaneously.
SystemCoreClock is software bookkeeping based on the declared source. Neither this variable nor ready/status flags measure actualHz.

GPIO/MCO, timers, UART, ADC, SysTick, CSS/NMI and RTOS are not initialized by these application sources.
MCO/physical frequency observations are a separate board plan; do not output72MHz SYSCLK toMCO (maximum50MHz IO), use a supported output such asPLL/2 after separatePA8 configuration.
Check actual crystal label, boardRevision and sharedPA8 ONE-WIRE/USART1CK before optional measurements. Do not drive oscillator pins with another source while a crystal circuit is connected.
These standalone examples are not hot-clock patches for E05/P01. Existing baud/timer/deadline bookkeeping must be redesigned for clock changes in an integrated application.

Compiler/linker checks are separate from real board/clock frequency/startup/failure measurements. Real board validation is not performed here.
ST CMSIS and original startup notices are retained. Existing learning projects and ZIPs are untouched.
