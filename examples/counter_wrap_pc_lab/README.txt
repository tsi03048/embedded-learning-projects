8-bit virtual counter wrap PC exercise
Open a Visual Studio Developer Command Prompt in this folder and run:
    cl /W4 /WX /TC counter_wrap.c /Fe:counter_wrap.exe
    counter_wrap.exe
Expected: 8/8 checks passed (including all 65536 virtual cases).
The real elapsed interval must be less than 256 ticks. No STM32 timer is used.
See the Notion lesson for the Korean explanation and assumptions.
