@echo off
rem *****************************************************************
rem [ext-02 XIO] XIO gateway live demo - one-click, three windows
rem Flow (no python needed - pure console passthrough, live output):
rem   win1 (this)  : controller - env check, step-by-step launch log,
rem                  5s alive-status monitor, visible cleanup result
rem   win2 (client): xio_gateway_client.exe joins multicast 235.7.11.27:3002,
rem                  sends the 10-command script (ping/add_platform/set_route/
rem                  set_speed/pause/resume/get_status/delete_platform x2/
rem                  negative) and prints acks live
rem   win3 (mission): mission.exe runs air_movers scenario + xio_interface block
rem                  (RT, end_time 45s, pauses until client connects); the
rem                  ysim_xio_gateway plugin prints received/executed command
rem                  lines live in that window
rem End: controller detects simulation exit (5s poll, 75s cap), then kills
rem      both spawned windows and prints a summary.
rem Note: iface prefix "192.168." must match this machine's NIC (see block).
rem       Formal assertion runs: python xio_e2e_verify.py
rem *****************************************************************
setlocal EnableDelayedExpansion
chcp 65001 >nul
set "CLIENT=e:\afsim\ysim\build\Release\xio_gateway_client.exe"
set "MISSION=e:\afsim\ysim\build\Release\mission.exe"
set "PLUGIN=e:\afsim\ysim\build\Release\mission_plugins\ysim_xio_gateway.dll"
set "DEMO=e:\afsim\ysim\build\demos\air_to_air"
set "IBLK=e:\afsim\ysim\tests\ysim_xio_gateway\scenarios\xio_iface_block.txt"
set "SCEN=e:\afsim\ysim\tests\ysim_xio_gateway\scenarios\xio_rt2_scenario.txt"
set "CTITLE=ysim-bat-xio-client"
set "MTITLE=ysim-bat-xio"
cd /d %~dp0

echo ==================================================================
echo  XIO gateway live demo (ext-02) - controller window
echo  plugin : ysim_xio_gateway.dll   client : xio_gateway_client.exe
echo ==================================================================

echo [bat] step 0/5: environment check ...
if exist "%CLIENT%" (echo          [FOUND ] client   : %CLIENT%) else (echo         [MISSING] client   : %CLIENT%& goto :fail)
if exist "%MISSION%" (echo         [FOUND ] mission  : %MISSION%) else (echo         [MISSING] mission  : %MISSION%& goto :fail)
if exist "%PLUGIN%" (echo          [FOUND ] plugin   : %PLUGIN%) else (echo         [MISSING] plugin   : %PLUGIN%& goto :fail)
if exist "%IBLK%" (echo           [FOUND ] ifblock  : %IBLK%) else (echo         [MISSING] ifblock  : %IBLK%& goto :fail)
if exist "%SCEN%" (echo           [FOUND ] scenario : %SCEN%) else (echo         [MISSING] scenario : %SCEN%& goto :fail)
taskkill /F /T /FI "WINDOWTITLE eq %CTITLE%" >nul 2>&1
taskkill /F /T /FI "WINDOWTITLE eq %MTITLE%" >nul 2>&1
echo          leftover windows from a previous run: cleaned (if any)

echo [bat] step 1/5: starting XIO client window (title %CTITLE%) ...
start "%CTITLE%" /d "e:\afsim\ysim\build\Release" cmd /c ""%CLIENT%" 235.7.11.27 192.168. 3002 180 5"
echo          client joining multicast 235.7.11.27:3002, will run a
echo          10-command script and print every ack in its own window

echo [bat] step 2/5: waiting 3s for the client to join the multicast group ...
ping -n 4 127.0.0.1 >nul

echo [bat] step 3/5: starting simulation window (title %MTITLE%) ...
start "%MTITLE%" /d "%DEMO%" cmd /c ""%MISSION%" -rt "%IBLK%" "%SCEN%""
echo          scenario : xio_rt2_scenario (RT, end_time 45s, pause-for-connection)
echo          watch that window for plugin lines like:
echo            received command 'set_speed' ... / executed command ...

echo [bat] step 4/5: monitoring both processes (5s ticks, cap 75s) ...
echo          client window shows the command script + ack replies live.
echo          the simulation ends by itself at 45s sim time.
set /a TICK=0
:monitor
set /a TICK+=5
if !TICK! GEQ 75 goto :monitor_done
rem wait-first then check: tick label matches wall time; ping is
rem stdin-independent (timeout.exe errors out under redirected stdin)
ping -n 6 127.0.0.1 >nul
tasklist /FI "IMAGENAME eq mission.exe" 2>nul | find /I "mission.exe" >nul && (set "MST=running") || (set "MST=exited ")
tasklist /FI "IMAGENAME eq xio_gateway_client.exe" 2>nul | find /I "xio_gateway_client.exe" >nul && (set "CST=running") || (set "CST=exited ")
echo [t+!TICK!s] simulation: !MST!   client: !CST!
if "!MST!"=="exited " goto :monitor_done
goto :monitor
:monitor_done
if !TICK! GEQ 75 (echo          monitor cap 75s reached) else (echo          simulation exited by itself - demo complete)

echo [bat] step 5/5: cleanup - closing spawned windows ...
taskkill /F /T /FI "WINDOWTITLE eq %CTITLE%"
taskkill /F /T /FI "WINDOWTITLE eq %MTITLE%"
tasklist /FI "IMAGENAME eq mission.exe" 2>nul | find /I "mission.exe" >nul && (echo         [WARN] mission.exe still alive) || (echo         [OK] no mission.exe left)
tasklist /FI "IMAGENAME eq xio_gateway_client.exe" 2>nul | find /I "xio_gateway_client.exe" >nul && (echo         [WARN] client still alive) || (echo         [OK] no xio_gateway_client.exe left)

echo ==================================================================
echo  demo finished (total !TICK!s of run time)
echo  - client window showed the 10-command script + ack replies
echo  - mission window showed plugin received/executed command lines
echo  for formal assertions run: python xio_e2e_verify.py
echo ==================================================================
pause
exit /b 0

:fail
echo [bat] ABORT: required file missing - fix the [MISSING] item(s) above.
pause
exit /b 1
