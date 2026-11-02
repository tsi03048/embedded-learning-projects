Motor feedback freshness exercise / Windows x64 / PC only.
Run in PowerShell: .\feedback_test.exe
Rebuild in x64 Native Tools Command Prompt for Visual Studio:
cl /nologo /std:c11 /W4 /WX feedback_test.c /Fe:feedback_test.exe

All ticks, edges and the separately reported fault are simulated.
NO_RECENT_EDGE proves neither motor stop nor sensor failure.
The 1000-tick timeout and reset are teaching conventions only.
No motor, sensor, driver, timing safety, or actual STM32 behavior is tested.
