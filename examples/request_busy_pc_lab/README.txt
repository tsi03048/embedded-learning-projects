Virtual busy-request PC exercise
Open a Visual Studio Developer Command Prompt in this folder and run:
    cl /W4 /WX /TC request_busy.c /Fe:request_busy.exe
    request_busy.exe
Expected: 10/10 checks passed. Single-threaded synthetic events only.
No real ADC, synchronization, cancellation, timeout, or motor control is implemented.
See the Notion lesson for Korean explanation and limitations.
