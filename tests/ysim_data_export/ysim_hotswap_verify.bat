@echo off
rem [ext-01] Adapter hot-swap acceptance: self-launches mission (~40s), asserts list / add_udp / remove / add_tcp sequence
chcp 65001 >nul
set PYTHONUTF8=1
cd /d %~dp0
python ysim_hotswap_verify.py %*
pause
