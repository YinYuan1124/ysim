@echo off
rem *****************************************************************
rem [ext-02 B2] DIS enhanced capture - one-click quick verify
rem Flow: launch mission.exe (dis_b2_iface + dis_b2_scenario, real-time 60s,
rem       CWD auto-switched to source tree dis\warfare)
rem       -> capture Fire/Detonation/Emission/Signal key fields
rem       -> print RESULT json, then kill the simulation window this bat spawned
rem Note: iface IP in the dis block and LOCAL_IF in dis_capture.py must match this machine
rem *****************************************************************
chcp 65001 >nul
set PYTHONUTF8=1
set "MISSION=e:\afsim\ysim\build\Release\mission.exe"
set "WARFARE=e:\afsim\ysim\src\core\wsf_mil\test_mission\dis\warfare"
set "IBLK=e:\afsim\ysim\tests\ysim_dis_gateway\scenarios\dis_b2_iface.txt"
set "SCEN=e:\afsim\ysim\tests\ysim_dis_gateway\scenarios\dis_b2_scenario.txt"
set "MOUT=e:\afsim\ysim\temp\bat_dis_b2_mission.out"
set "WTITLE=ysim-bat-dis2"
cd /d %~dp0
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
echo [bat] launching simulation (minimized window): dis_b2_iface + dis_b2_scenario
start "%WTITLE%" /min /d "%WARFARE%" cmd /c ""%MISSION%" -rt "%IBLK%" "%SCEN%" > "%MOUT%" 2>&1"
python dis_capture.py
echo [bat] capture finished, cleaning up simulation...
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
pause
