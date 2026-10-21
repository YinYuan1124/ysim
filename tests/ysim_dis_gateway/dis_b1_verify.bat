@echo off
rem [ext-02 B1] Seven-segment mapping injection E2E (two rounds, real-time 20s each; add --regression for degraded-mode run)
chcp 65001 >nul
set PYTHONUTF8=1
cd /d %~dp0
python dis_b1_verify.py %*
pause
