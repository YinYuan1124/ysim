# -*- coding: utf-8 -*-
# 基准客户端（直播版）：连数据出口收流即丢（只统计行数），为帧率基准提供最严广播负载
# 每 5 秒在控制台打印累计行数与平均速率，结束时输出汇总
import socket
import sys
import time

HOST = "127.0.0.1"
port = int(sys.argv[1]) if len(sys.argv) > 1 else 31600
duration = float(sys.argv[2]) if len(sys.argv) > 2 else 300

print("=" * 62)
print(" ysim_data_export 插件 - 吞吐基准演示")
print(" 目标: %s:%d  时长: %.0f s" % (HOST, port, duration))
print("=" * 62)

# 循环重试连接（等 mission 起来，最长 30s）
sock, t0 = None, time.time()
while time.time() - t0 < 30:
    try:
        sock = socket.create_connection((HOST, port), timeout=1)
        break
    except OSError:
        time.sleep(0.1)
if sock is None:
    print("[FAIL] 30 秒内未能连接 %s:%d" % (HOST, port))
    sys.exit(1)
print("[conn] 已连接 (%.1fs), 收流计行中...\n" % (time.time() - t0))

sock.settimeout(1.0)
buf = b""
lines = 0
deadline = time.time() + duration
last_tick = time.time()
while time.time() < deadline:
    try:
        data = sock.recv(65536)
    except socket.timeout:
        continue
    if not data:
        break
    buf += data
    lines += data.count(b"\n")
    buf = buf[-256:]
    if time.time() - last_tick >= 5.0:
        elapsed = time.time() - t0
        print("[t+%5.1fs] 已收 %d 行 | 平均 %.0f 行/s" % (elapsed, lines, lines / max(elapsed, 0.001)))
        last_tick = time.time()

elapsed = time.time() - t0
print("\n" + "-" * 62)
print(" 基准结果: 共 %d 行 | 平均 %.0f 行/s (历时 %.1fs)" % (lines, lines / max(elapsed, 0.001), elapsed))
print("-" * 62)
