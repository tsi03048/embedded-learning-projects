ADC freshness teaching model: PC only, Windows x64.
Run in PowerShell: .\freshness.exe
To rebuild in x64 Native Tools Command Prompt for Visual Studio:
cl /nologo /std:c11 /W4 /WX freshness.c /Fe:freshness.exe

The time ticks and ADC events are injected by the test; no ADC or timer is used.
The time threshold 500 is a teaching example, not a recommended timeout.
A newly observed sample wins over timeout at the exact deadline by design.
No STM32 firmware, physical timing, input wiring or hardware is tested.
