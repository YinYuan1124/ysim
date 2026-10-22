@echo off
rem *****************************************************************
rem [ext-02 B2] DIS engagement capture - one-click demo
rem Flow: launch mission.exe in a VISIBLE console (dis_b2_iface + dis_b2_scenario,
rem       real-time 60s, CWD auto-switched to source tree dis\warfare; AFSIM logs
rem       stream live in that window)
rem       -> this window: live-print Fire/Detonation/Emission/Signal events
rem          as they arrive, 2s heartbeat, RESULT json
rem       -> kill the simulation window this bat spawned
rem Note: iface IP in the dis block and LOCAL_IF in dis_capture.py must match this machine
rem       Formal assertion runs: python dis_b2_verify.py
rem *****************************************************************
chcp 65001 >nul
set PYTHONUTF8=1
set "MISSION=e:\Project\ysim\build\Release\mission.exe"
set "WARFARE=e:\Project\ysim\src\core\wsf_mil\test_mission\dis\warfare"
set "IBLK=e:\Project\ysim\tests\ysim_dis_gateway\scenarios\dis_b2_iface.txt"
set "SCEN=e:\Project\ysim\tests\ysim_dis_gateway\scenarios\dis_b2_scenario.txt"
set "WTITLE=ysim-bat-dis2"
cd /d %~dp0
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
echo [bat] launching simulation (visible console): dis_b2_iface + dis_b2_scenario
start "%WTITLE%" /d "%WARFARE%" cmd /c ""%MISSION%" -rt "%IBLK%" "%SCEN%""
python dis_capture.py
echo [bat] capture finished, cleaning up simulation...
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
pause
