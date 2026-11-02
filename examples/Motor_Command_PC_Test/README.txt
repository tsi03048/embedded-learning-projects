Motor command PC exercise / Windows x64.
Run: .\motor_test.exe
Rebuild in x64 Native Tools Command Prompt for Visual Studio:
cl /nologo /std:c11 /W4 /WX motor_test.c /Fe:motor_test.exe

Logical request model only: it does not control any motor, GPIO, PWM or driver.
STOP is an abstract request, not guaranteed physical braking or stopping.
No transition timing, safety interlock, fault latch or motor dynamics are modeled.
