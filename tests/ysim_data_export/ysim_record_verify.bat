@echo off
rem *****************************************************************
rem [ext-01] Structured record - one-click quick verify
rem Flow: run mission.exe synchronously (ysim_smoke_record scenario, no -rt,
rem       fast-forward to completion; push channels off / CSV+SQLite record on)
rem       -> verify latest run_* under temp\ysim_record_out\
rem       (CSV headers / full event lifecycle / SQLite 4 tables / metric consistency)
rem *****************************************************************
chcp 65001 >nul
set PYTHONUTF8=1
set "MISSION=e:\afsim\ysim\build\Release\mission.exe"
set "DEMO=e:\afsim\ysim\build\demos\air_to_air"
set "SCEN=e:\afsim\ysim\tests\ysim_data_export\scenarios\ysim_smoke_record.txt"
set "MOUT=e:\afsim\ysim\temp\bat_record_mission.out"
cd /d %~dp0
echo [bat] running record scenario (fast-forward, exits when done)...
pushd "%DEMO%"
"%MISSION%" "%SCEN%" > "%MOUT%" 2>&1
set "MRC=%errorlevel%"
popd
echo [bat] mission exit code %MRC%, verifying record output...
python ysim_record_verify.py %*
pause
