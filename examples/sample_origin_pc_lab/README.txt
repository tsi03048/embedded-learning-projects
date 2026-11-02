Synthetic measurement origin PC exercise
Open a Visual Studio Developer Command Prompt in this folder and run:
    cl /W4 /WX /TC sample_origin.c /Fe:sample_origin.exe
    sample_origin.exe
Expected: 8/8 checks passed. One virtual request at a time, no hardware.
This is not STM32 ADC synchronization or motor-control firmware.
See the Notion lesson for Korean explanation and limitations.
