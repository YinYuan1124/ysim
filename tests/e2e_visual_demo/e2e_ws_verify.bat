@echo off
rem [e2e demo] Headless WebSocket streaming verify (handshake / snapshot / state stream / heartbeat / fields)
chcp 65001 >nul
set PYTHONUTF8=1
cd /d %~dp0
python e2e_ws_verify.py %*
pause
