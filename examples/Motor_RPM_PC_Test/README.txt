Motor speed count-window exercise / Windows x64 / PC only.
Run in PowerShell: .\rpm_test.exe
To rebuild in x64 Native Tools Command Prompt for Visual Studio:
cl /nologo /std:c11 /W4 /WX rpm_test.c /Fe:rpm_test.exe

The examples use hypothetical edge counts, counts per measured shaft revolution,
and elapsed window. Fractional RPM is truncated. Zero observed edges does not
prove the motor is stopped or the sensor is working. The model does not count
real pulses, infer direction, compensate gear ratios, or validate a sensor.
