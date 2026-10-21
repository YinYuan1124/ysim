@echo off
rem *****************************************************************
rem [ext-01] Multi-channel concurrency - one-click quick verify
rem Flow: launch mission.exe (ysim_smoke_full scenario, real-time 20s, TCP/WS/UDP all on)
rem       -> concurrent receive on TCP 31600 + WS 31601 + UDP 239.16.16.16:31602
rem       -> kill the simulation window this bat spawned
rem Output: tcp/ws/udp PASS/FAIL in this window, detail -> temp\ysim_multi_result.txt
rem *****************************************************************
chcp 65001 >nul
set PYTHONUTF8=1
set "MISSION=e:\afsim\ysim\build\Release\mission.exe"
set "DEMO=e:\afsim\ysim\build\demos\air_to_air"
set "SCEN=e:\afsim\ysim\tests\ysim_data_export\scenarios\ysim_smoke_full.txt"
set "MOUT=e:\afsim\ysim\temp\bat_multi_mission.out"
set "WTITLE=ysim-bat-multi"
cd /d %~dp0
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
echo [bat] launching simulation (minimized window): ysim_smoke_full.txt
start "%WTITLE%" /min /d "%DEMO%" cmd /c ""%MISSION%" -rt "%SCEN%" > "%MOUT%" 2>&1"
python ysim_multi_client.py %*
echo [bat] client finished, cleaning up simulation...
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
pause
