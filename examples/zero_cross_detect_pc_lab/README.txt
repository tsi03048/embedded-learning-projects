Synthetic zero crossing PC exercise
Open a Visual Studio Developer Command Prompt in this folder and run:
    cl /W4 /WX /TC crossing.c /Fe:crossing.exe
    crossing.exe
Expected: 12/12 passed. Values are synthetic signed differences, not ADC readings.
This program does not measure or control an actual motor.
See the Notion lesson for the Korean explanation and assumptions.
