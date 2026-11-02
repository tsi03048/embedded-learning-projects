B01 Power/Reset v3.2 - STM32F103VET6 LQFP100

Open Power_Reset.uvproj in Keil uVision, choose Arm Compiler6.22 and one target, Rebuild.
01_Reset_Flags -> App/01_reset_flags.c: snapshot RCC_CSR; set clear_request RAM to1 only after recording.
02_PVD_Poll -> App/02_pvd_poll.c: PWREN, PLS111, PVDE, pollPVDO. No PVD interrupt, timer, UART or watchdog enabled by the app.
Target Device: STM32F103VE / STM32F10X_HD; Flash512KB, SRAM64KB; copied CMSIS uses nominal HSI8MHz.
Compiler option: -std=c99 (not --c99); optimization setting1 in this project is compiler -O0.

Use normal board power for the first run. No new GPIO wiring is required for RAM observation.
First reset source is affected by download/debugger behavior. After recording, clear viaRMVF and establish a fresh baseline before a physicalNRST test.
Reset flags can accumulate; keep the fullCSR snapshot, do not infer a unique latest cause from one flag.
All application RAM globals are reinitialized by C runtime on restart. Old values in boot_csr are not nonvolatile logs.
Only reset-flag mask0xFC000000 is expected0 after clear, not the entireCSR (LSI bits are separate).

PVD valid only afterPVDE is enabled. PLS111 names nominal2.9V; DS5792Rev13Table12 has falling2.66/2.78/2.90V and rising2.76/2.88/3.00V.
PVDO reads0 when above,1 when below the actual threshold, with hysteresis. PVDO is status, not reset or a saved interrupt event.
Supply variation is a later board plan: confirm actual MCU VDD/VDDA, common ground, valid operating conditions and all alternate power paths first.
Do not tie an adjustable source to an already-powered rail. ST-LINK/USB-UART and modules can back-power pins. Board-level power changes are not verified here.

Verification separates compiler/linker logs from real hardware. No actual board, PVD threshold, NRST waveform or power-ramp timing has been measured.
Existing learning packages and original ST notices/startup are preserved. The app uses no user initialization wrappers.
