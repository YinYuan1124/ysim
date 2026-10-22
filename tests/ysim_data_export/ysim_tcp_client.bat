@echo off
rem *****************************************************************
rem [ext-01] TCP streaming demo - one-click
rem Flow: launch mission.exe in a VISIBLE console (ysim_smoke_2v2, real-time 20s,
rem       TCP 31600; AFSIM logs + plugin banner stream live in that window)
rem       -> this window: connect 31600, live-print the JSON event stream
rem       -> kill the simulation window this bat spawned
rem Output: live stream here, detail -> temp\ysim_client_result.txt
rem *****************************************************************
chcp 65001 >nul
set PYTHONUTF8=1
set "MISSION=e:\Project\ysim\build\Release\mission.exe"
set "DEMO=e:\Project\ysim\build\demos\air_to_air"
set "SCEN=e:\Project\ysim\tests\ysim_data_export\scenarios\ysim_smoke_2v2.txt"
set "WTITLE=ysim-bat-tcp"
cd /d %~dp0
rem Port override: set YSIM_PORT=32700 before running (applies to both mission and client)
set "MARGS=-rt"
if defined YSIM_PORT set "MARGS=--ysim-port %YSIM_PORT% -rt"
rem Kill leftover simulation windows with the same title from a previous crashed run
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
echo [bat] launching simulation (visible console): ysim_smoke_2v2.txt
start "%WTITLE%" /d "%DEMO%" cmd /c ""%MISSION%" %MARGS% "%SCEN%""
python ysim_tcp_client.py %*
echo [bat] client finished, cleaning up simulation...
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
pause
