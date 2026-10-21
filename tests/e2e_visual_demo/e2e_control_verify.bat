@echo off
rem [e2e control panel] Headless full-chain verify (startup freeze / run / pause / 4x speed / reset / cleanup)
chcp 65001 >nul
set PYTHONUTF8=1
cd /d %~dp0
python e2e_control_verify.py %*
pause
