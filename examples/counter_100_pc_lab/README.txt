Virtual 0-99 counter PC exercise
Open a Visual Studio Developer Command Prompt in this folder and run:
    cl /W4 /WX /TC counter_100.c /Fe:counter_100.exe
    counter_100.exe
Expected: 8/8 checks passed including 10000 virtual pairs.
Actual elapsed time must be below 100 ticks. No STM32 timer is used.
See the Notion lesson for Korean explanation and limitations.
