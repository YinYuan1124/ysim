@echo off
rem ============================================================================
rem ext-05 equipmentdb end-to-end test
rem Runs mission.exe with test_equipmentdb.txt, then cross-verifies the
rem script output against a direct sqlite3 query of DB3K_modern.db3.
rem Output file eqdb_result.txt is created in this directory and removed on exit.
rem ============================================================================
title ysim-bat-equipmentdb
setlocal

set "ROOT=%~dp0..\.."
set "MISSION=%ROOT%\build\Release\mission.exe"
set "SCENARIO=%~dp0scenarios\test_equipmentdb.txt"
set "RESULT=%~dp0eqdb_result.txt"

if not exist "%MISSION%" (
    echo [ERROR] mission.exe not found: %MISSION%
    goto :fail
)
if not exist "%SCENARIO%" (
    echo [ERROR] scenario not found: %SCENARIO%
    goto :fail
)

if exist "%RESULT%" del "%RESULT%"

echo [1/2] Running mission with test scenario...
cd /d "%~dp0"
"%MISSION%" "%SCENARIO%"
set "MRC=%ERRORLEVEL%"

if not exist "%RESULT%" (
    echo [ERROR] mission exited rc=%MRC% and no result file was produced.
    echo         Check that ysim_equipment_db.dll is in build\Release\mission_plugins.
    goto :fail
)

echo [2/2] Cross-verifying against direct sqlite3 queries...
python "%~dp0verify_equipmentdb.py" "%RESULT%"
set "VRC=%ERRORLEVEL%"

if "%VRC%"=="0" (
    echo.
    echo ============================================================
    echo  ext-05 equipmentdb test: PASS
    echo ============================================================
    del "%RESULT%"
    endlocal
    exit /b 0
)

:fail
echo.
echo ============================================================
echo  ext-05 equipmentdb test: FAIL
echo ============================================================
if exist "%RESULT%" echo Result file kept for inspection: %RESULT%
endlocal
exit /b 1
