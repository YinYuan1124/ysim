# -*- coding: utf-8 -*-
# 基准客户端：连 31600 收流即丢（只统计行数），为帧率基准提供最严广播负载
import socket
import sys
import time

lines = 0
s = socket.create_connection(("127.0.0.1", int(sys.argv[1]) if len(sys.argv) > 1 else 31600), timeout=5)
s.settimeout(1.0)
buf = b""
deadline = time.time() + float(sys.argv[2] if len(sys.argv) > 2 else 300)
while time.time() < deadline:
    try:
        data = s.recv(65536)
    except socket.timeout:
        continue
    if not data:
        break
    buf += data
    lines += data.count(b"\n")
    buf = buf[-256:]
print("received lines:", lines)
