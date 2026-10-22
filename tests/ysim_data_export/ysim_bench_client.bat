@echo off
rem *****************************************************************
rem [ext-01] Throughput benchmark demo - one-click
rem Flow: launch mission.exe in a VISIBLE console (ysim_bench_full, real-time,
rem       end_time 600s, all channels; AFSIM logs stream live in that window)
rem       -> this window: connect TCP port, count lines with a 5s live ticker
rem       -> kill the simulation window this bat spawned
rem Usage: ysim_bench_client.bat [port] [seconds] (default 31600 300; custom port also overrides mission via --ysim-port)
rem *****************************************************************
chcp 65001 >nul
set PYTHONUTF8=1
set "MISSION=e:\Project\ysim\build\Release\mission.exe"
set "DEMO=e:\Project\ysim\build\demos\air_to_air"
set "SCEN=e:\Project\ysim\tests\ysim_data_export\scenarios\ysim_bench_full.txt"
set "WTITLE=ysim-bat-bench"
cd /d %~dp0
set "MARGS=-rt"
if not "%~1"=="" set "MARGS=--ysim-port %~1 -rt"
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
echo [bat] launching benchmark simulation (visible console): ysim_bench_full.txt
start "%WTITLE%" /d "%DEMO%" cmd /c ""%MISSION%" %MARGS% "%SCEN%""
python ysim_bench_client.py %*
echo [bat] client finished, cleaning up simulation...
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
pause
