@echo off
rem *****************************************************************
rem [ext-02 B1] DIS seven-segment live listen - one-click demo
rem Flow: launch mission.exe in a VISIBLE console (dis_iface_block + dis_b1_scenario,
rem       real-time 20s; AFSIM logs + plugin banner stream live in that window)
rem       -> this window: join multicast 224.2.25.54:3225, live-print entity
rem          seven-segment codes as they appear, 2s heartbeat, RESULT json
rem       -> kill the simulation window this bat spawned
rem Note: iface IP in the dis block and IFACE in dis_listen.py must match this machine (192.168.5.3)
rem       Formal assertion runs: python dis_b1_verify.py
rem *****************************************************************
chcp 65001 >nul
set PYTHONUTF8=1
set "MISSION=e:\Project\ysim\build\Release\mission.exe"
set "DEMO=e:\Project\ysim\build\demos\air_to_air"
set "IBLK=e:\Project\ysim\tests\ysim_dis_gateway\scenarios\dis_iface_block.txt"
set "SCEN=e:\Project\ysim\tests\ysim_dis_gateway\scenarios\dis_b1_scenario.txt"
set "WTITLE=ysim-bat-dis1"
cd /d %~dp0
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
echo [bat] launching simulation (visible console): dis_iface_block + dis_b1_scenario
start "%WTITLE%" /d "%DEMO%" cmd /c ""%MISSION%" -rt "%IBLK%" "%SCEN%""
python dis_listen.py
echo [bat] listener finished, cleaning up simulation...
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
pause
