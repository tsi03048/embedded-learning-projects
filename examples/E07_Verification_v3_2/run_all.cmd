@echo off
setlocal
cd /d "%~dp0"
where cl >nul 2>nul
if errorlevel 1 (
  echo Open the x64 Developer Command Prompt for Visual Studio first.
  exit /b 2
)
if not exist build mkdir build
if not exist evidence mkdir evidence
cl /W4 /WX /Od /TC "src\request_busy.c" /Fe:"build\request_busy.exe" /Fo:"build\request_busy.obj" > "evidence\request_busy_build.txt" 2>&1
if errorlevel 1 (
  type "evidence\request_busy_build.txt"
  exit /b 1
)
type "evidence\request_busy_build.txt"
"build\request_busy.exe" > "evidence\request_busy_run.txt"
if errorlevel 1 (
  type "evidence\request_busy_run.txt"
  exit /b 1
)
type "evidence\request_busy_run.txt"
cl /W4 /WX /Od /TC "src\sample_origin.c" /Fe:"build\sample_origin.exe" /Fo:"build\sample_origin.obj" > "evidence\sample_origin_build.txt" 2>&1
if errorlevel 1 (
  type "evidence\sample_origin_build.txt"
  exit /b 1
)
type "evidence\sample_origin_build.txt"
"build\sample_origin.exe" > "evidence\sample_origin_run.txt"
if errorlevel 1 (
  type "evidence\sample_origin_run.txt"
  exit /b 1
)
type "evidence\sample_origin_run.txt"
echo E07 reference programs completed.
exit /b 0
