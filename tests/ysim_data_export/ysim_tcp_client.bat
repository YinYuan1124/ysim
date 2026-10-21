@echo off
rem *****************************************************************
rem [ext-01] TCP streaming - one-click quick verify
rem Flow: launch mission.exe (ysim_smoke_2v2 scenario, real-time 20s, TCP 31600)
rem       -> connect 31600, receive JSON lines until sim_complete
rem       -> kill the simulation window this bat spawned
rem Output: stats in this window, detail -> temp\ysim_client_result.txt
rem *****************************************************************
chcp 65001 >nul
set PYTHONUTF8=1
set "MISSION=e:\afsim\ysim\build\Release\mission.exe"
set "DEMO=e:\afsim\ysim\build\demos\air_to_air"
set "SCEN=e:\afsim\ysim\tests\ysim_data_export\scenarios\ysim_smoke_2v2.txt"
set "MOUT=e:\afsim\ysim\temp\bat_tcp_mission.out"
set "WTITLE=ysim-bat-tcp"
cd /d %~dp0
rem Port override: set YSIM_PORT=32700 before running (applies to both mission and client)
set "MARGS=-rt"
if defined YSIM_PORT set "MARGS=--ysim-port %YSIM_PORT% -rt"
rem Kill leftover simulation windows with the same title from a previous crashed run
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
echo [bat] launching simulation (minimized window): ysim_smoke_2v2.txt
start "%WTITLE%" /min /d "%DEMO%" cmd /c ""%MISSION%" %MARGS% "%SCEN%" > "%MOUT%" 2>&1"
python ysim_tcp_client.py %*
echo [bat] client finished, cleaning up simulation...
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
pause
