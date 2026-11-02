P02 Expansion Plan v3.2 / 2026-11-01 Asia/Seoul

This is a planning exercise, not expanded STM32 firmware.
Open x64 Developer Command Prompt for Visual Studio, cd to this folder, run run_budget.cmd.
Requires MSVC C++ Build Tools. Options /W4 /WX /Od /TC. Artifacts go to build; your logs go to evidence.
evidence/reference contains the editor reference run, separate from your run.
Preserve old run folders before rerunning. The local evidence/build.txt and run.txt are overwritten.

report_budget.c counts four fixed ASCII strings in main. 8N1,9600 baud and100ms are declared planning inputs.
Values exclude the terminating NUL on the wire. The maximum example includes only generation0/1,
32-bit decimal seq/tick, unsigned12-bit raw0..4095, one-character state andCRLF.
The53-byte array capacity is for this exact format only, not a generic parser buffer bound.
Wire time excludes deliberate gaps, sync markers, control reports, parsing and receiver scheduling.
Link occupancy is not CPU utilization or end-to-end latency.

templates/decision.md: choose one extension and list prerequisite documents.
templates/interface_and_tests.md: snapshot contract and prewritten, unexecuted integration test plan.
templates/resources_and_rtos.md: resource conflicts, time evidence and deferred RTOS decision.

Existing P01/E05 firmware, ZIPs and source were not changed. No new AC6 build or board tests.
