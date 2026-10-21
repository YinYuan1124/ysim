@echo off
rem *****************************************************************
rem [ext-02 B1] DIS seven-segment listen - one-click quick verify
rem Flow: launch mission.exe (dis_iface_block + dis_b1_scenario, real-time 20s)
rem       -> join multicast 224.2.25.54:3225, aggregate EntityState seven-seg codes
rem       -> print RESULT json, then kill the simulation window this bat spawned
rem Note: iface IP in the dis block and IFACE in dis_listen.py must match this machine (192.168.5.3)
rem *****************************************************************
chcp 65001 >nul
set PYTHONUTF8=1
set "MISSION=e:\afsim\ysim\build\Release\mission.exe"
set "DEMO=e:\afsim\ysim\build\demos\air_to_air"
set "IBLK=e:\afsim\ysim\tests\ysim_dis_gateway\scenarios\dis_iface_block.txt"
set "SCEN=e:\afsim\ysim\tests\ysim_dis_gateway\scenarios\dis_b1_scenario.txt"
set "MOUT=e:\afsim\ysim\temp\bat_dis_b1_mission.out"
set "WTITLE=ysim-bat-dis1"
cd /d %~dp0
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
echo [bat] launching simulation (minimized window): dis_iface_block + dis_b1_scenario
start "%WTITLE%" /min /d "%DEMO%" cmd /c ""%MISSION%" -rt "%IBLK%" "%SCEN%" > "%MOUT%" 2>&1"
python dis_listen.py
echo [bat] listener finished, cleaning up simulation...
taskkill /F /T /FI "WINDOWTITLE eq %WTITLE%" >nul 2>&1
pause
