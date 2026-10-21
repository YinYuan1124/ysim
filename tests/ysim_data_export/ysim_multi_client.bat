@echo off
rem *****************************************************************
rem [ext-01] Multi-channel concurrency demo - one-click
rem Flow: launch mission.exe in a VISIBLE console (ysim_smoke_full, real-time 20s,
rem       TCP/WS/UDP all on; AFSIM logs + plugin banner stream live in that window)
rem       -> this window: concurrent live receive on TCP 31600 + WS 31601
rem          + UDP multicast 239.16.16.16:31602, with per-channel progress
rem       -> kill the simulation window this bat spawned
rem Output: tcp/ws/udp live progress + PASS/FAIL here, detail -> temp\ysim_multi_result.txt
rem *****************************************************************
chcp 65001 >nul
set PYTHONUTF8=1
set "MISSION=e:\afsim\ysim\build\Release\mission.exe"
set "DEMO=e:\afsim\ysim\build\demos\air_to_air"
set "SCEN=e:\afsim\ysim\tests\ysim_data_export\scenarios\ysim_smoke_full.txt"
set "WTITLE=ysim-bat-multi"
cd /d %~dp0
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
echo [bat] launching simulation (visible console): ysim_smoke_full.txt
start "%WTITLE%" /d "%DEMO%" cmd /c ""%MISSION%" -rt "%SCEN%""
python ysim_multi_client.py %*
echo [bat] client finished, cleaning up simulation...
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
pause
