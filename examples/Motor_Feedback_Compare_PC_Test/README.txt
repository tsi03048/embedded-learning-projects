Motor speed comparison practice / Windows x64 / PC only.
Run in PowerShell: .\compare_test.exe
Rebuild in x64 Native Tools Command Prompt for Visual Studio:
cl /nologo /std:c11 /W4 /WX compare_test.c /Fe:compare_test.exe

The model compares unsigned speed magnitudes using a +/-10 RPM teaching band.
It only reports an action; it does not change PWM or stop physical hardware.
NO_DECISION is not a safety action. Sensor validation, motor and driver control,
timing, fault handling, direction, and real-world stability are not implemented.
