E07 Verification v3.2 / 2026-11-01 Asia/Seoul

E06-21 request_busy.c and E06-19 sample_origin.c are unchanged PC sources.
Open x64 Developer Command Prompt for Visual Studio, cd to this folder and run run_all.cmd.
Requires MSVC C++ Build Tools; no STM32 board, Python, or Arm Compiler required.
Options: /W4 /WX /Od /TC. Local build artifacts go to build; your fresh logs go to evidence.
Reference logs in evidence/reference remain untouched. Use a new run folder or save your logs before rerunning.
Source SHA256 is recorded in source_manifest.json. Expected totals: 10/10 and 8/8 checks, exit code0 each.

templates/requirements.csv: four bounded PC requirements.
templates/test_plan.csv: prewritten expected results, your actual/status fields remain unexecuted.
templates/test_report.md: run/environment/evidence template.
templates/defect_example.md: deliberate bad-label scenario and requirement failure despite a passing demonstration check.
templates/board_plan.md: unexecuted E05 firmware board plan, separate from these PC sources.
templates/team_record.md: roles, handoff, individual contribution.

W/V are observation labels; +/-3 are virtual signed values, not raw ADC voltages or real motor signals.
Single-thread models do not establish interrupt/DMA synchronization or hardware ADC acceptance.
No C source changes, firmware builds, real board measurements, or post-fix regression are claimed.
