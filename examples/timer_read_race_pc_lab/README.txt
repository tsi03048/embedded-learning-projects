Deterministic timer-read race PC exercise
Open a Visual Studio Developer Command Prompt in this folder and run:
    cl /W4 /WX /TC timer_read_race.c /Fe:timer_read_race.exe
    timer_read_race.exe
Expected: 5/5 checks passed. This is a synthetic event-order model.
It is not an STM32 timer, ISR, synchronization algorithm, or safe snapshot.
See the Notion lesson for the Korean explanation and limitations.
