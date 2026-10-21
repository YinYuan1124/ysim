@echo off
rem *****************************************************************
rem [ext-01] Throughput benchmark - one-click quick verify
rem Flow: launch mission.exe (ysim_bench_full scenario, real-time, end_time 600s, all channels)
rem       -> connect TCP port, count received lines (client exits on timeout or disconnect)
rem       -> kill the simulation window this bat spawned
rem Usage: ysim_bench_client.bat [port] [seconds] (default 31600 300; custom port also overrides mission via --ysim-port)
rem *****************************************************************
chcp 65001 >nul
set PYTHONUTF8=1
set "MISSION=e:\afsim\ysim\build\Release\mission.exe"
set "DEMO=e:\afsim\ysim\build\demos\air_to_air"
set "SCEN=e:\afsim\ysim\tests\ysim_data_export\scenarios\ysim_bench_full.txt"
set "MOUT=e:\afsim\ysim\temp\bat_bench_mission.out"
set "WTITLE=ysim-bat-bench"
cd /d %~dp0
set "MARGS=-rt"
if not "%~1"=="" set "MARGS=--ysim-port %~1 -rt"
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
echo [bat] launching benchmark simulation (minimized window): ysim_bench_full.txt
start "%WTITLE%" /min /d "%DEMO%" cmd /c ""%MISSION%" %MARGS% "%SCEN%" > "%MOUT%" 2>&1"
rem benchmark client has no retry: wait for the data-export port to come up
timeout /t 3 /nobreak >nul
python ysim_bench_client.py %*
echo [bat] client finished, cleaning up simulation...
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
pause
