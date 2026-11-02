Extended virtual counter PC exercise
Open a Visual Studio Developer Command Prompt in this folder and run:
    cl /W4 /WX /TC extended_counter.c /Fe:extended_counter.exe
    extended_counter.exe
Expected: 8/8 checks passed. Snapshots are fictional and assumed coherent.
No actual timer, interrupt, overflow tracking, or atomic read is implemented.
See the Notion lesson for Korean explanation and limitations.
