@echo off
rem [ext-02 phase A] XIO platform-level command set E2E (two rounds, real-time 45s + regression; add --regression to run regression only)
chcp 65001 >nul
set PYTHONUTF8=1
cd /d %~dp0
python xio_e2e_verify.py %*
pause
