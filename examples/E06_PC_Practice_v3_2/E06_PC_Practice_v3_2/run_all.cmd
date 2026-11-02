@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
  echo Open a Visual Studio x64 Developer Command Prompt, then run this file.
  exit /b 1
)
pushd "%~dp0"
if not exist build mkdir build
call :run "02_fresh_samples" "fresh_samples"
if errorlevel 1 goto failed
call :run "04_periodic_watch" "watch"
if errorlevel 1 goto failed
call :run "06_counter_wrap" "counter_wrap"
if errorlevel 1 goto failed
call :run "08_counter_100" "counter_100"
if errorlevel 1 goto failed
call :run "10_extended_counter" "extended_counter"
if errorlevel 1 goto failed
call :run "12_timer_read_race" "timer_read_race"
if errorlevel 1 goto failed
call :run "15_sensorless_review" "review"
if errorlevel 1 goto failed
call :run "17_step_identity" "step_identity"
if errorlevel 1 goto failed
call :run "19_sample_origin" "sample_origin"
if errorlevel 1 goto failed
call :run "21_request_busy" "request_busy"
if errorlevel 1 goto failed
echo All 10 PC exercises passed. Expected total: 98 checks.
popd
exit /b 0
:failed
popd
exit /b 1
:run
cl /nologo /W4 /WX /TC "%~1\%~2.c" "/Fe:build\%~2.exe" "/Fo:build\%~2.obj"
if errorlevel 1 exit /b 1
"build\%~2.exe"
exit /b %errorlevel%
