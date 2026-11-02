Periodic sample watchdog PC exercise
Open a Visual Studio Developer Command Prompt in this folder and run:
    cl /W4 /WX /TC watch.c /Fe:watch.exe
    watch.exe
Expected: 13/13 passed. Times and events are synthetic.
No hardware timer, ADC, motor control, or safety output is implemented.
See the Notion lesson for explanation and assumptions.
