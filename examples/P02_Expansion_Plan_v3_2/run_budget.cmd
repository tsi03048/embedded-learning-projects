@echo off
setlocal
cd /d "%~dp0"
where cl >nul 2>nul
if errorlevel 1 exit /b 2
if not exist build mkdir build
if not exist evidence mkdir evidence
cl /W4 /WX /Od /TC report_budget.c /Fe:build\report_budget.exe /Fo:build\report_budget.obj > evidence\build.txt 2>&1
if errorlevel 1 (
  type evidence\build.txt
  exit /b 1
)
type evidence\build.txt
build\report_budget.exe > evidence\run.txt
if errorlevel 1 exit /b 1
type evidence\run.txt
exit /b 0
