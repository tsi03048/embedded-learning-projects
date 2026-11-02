ADC snapshot demonstration - PC Windows x64 only
Run in PowerShell: .\snapshot.exe
To rebuild in x64 Native Tools Command Prompt for Visual Studio:
cl /nologo /std:c11 /W4 /WX snapshot.c /Fe:snapshot.exe

The program manually inserts a writer between separate reads to demonstrate a
mixed value and timestamp. Its serialized copy is single-threaded; it is NOT
an interrupt-safe implementation, lock, or concurrency test. No STM32 ADC,
interrupt, hardware timing, or firmware behavior is tested.
