@echo off
rem [ext-02 B2] PDU field completion E2E (two rounds, real-time 60s each; add --regression for degraded-mode run)
chcp 65001 >nul
set PYTHONUTF8=1
cd /d %~dp0
python dis_b2_verify.py %*
pause
