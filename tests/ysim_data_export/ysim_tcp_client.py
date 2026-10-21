# -*- coding: utf-8 -*-
# ****************************************************************************
# 二次开发（ext-01）：ysim_data_export TCP 直播演示客户端
# 行为：循环重试连接 → 控制台实时滚动打印消息流 → 收到 sim_complete 后汇总
# 输出：控制台直播（demo 观感）+ 统计明细写 temp/ysim_client_result.txt
# ****************************************************************************
import json
import os
import socket
import sys
import time

HOST = "127.0.0.1"
PORT = int(os.environ.get("YSIM_PORT", "31600"))
RESULT_FILE = r"E:\afsim\ysim\temp\ysim_client_result.txt"


def tstamp(sim_time):
    return "t+%5.1fs" % sim_time


def main():
    print("=" * 62)
    print(" ysim_data_export 插件 - TCP 数据流直播演示")
    print(" 目标: %s:%d  (连接失败自动重试, 等待 mission 启动...)" % (HOST, PORT))
    print("=" * 62)

    lines = []
    sock = None
    t0 = time.time()
    deadline = t0 + 60  # 总超时：1 分钟（含等待 mission 启动）
    connected = False

    # 循环重试连接：mission 初始化期间端口才开放（20ms 高频轮询抢在首帧前连上）
    while time.time() < deadline:
        try:
            sock = socket.create_connection((HOST, PORT), timeout=1)
            connected = True
            break
        except OSError:
            time.sleep(0.02)

    if sock is None:
        print("[FAIL] 60 秒内未能连接数据出口 %s:%d" % (HOST, PORT))
        with open(RESULT_FILE, "w", encoding="utf-8") as f:
            f.write("FAIL: 60 秒内未能连接数据出口 %s:%d\n" % (HOST, PORT))
        sys.exit(1)
    print("[conn] 已连接 (%.1fs), 开始接收实时数据流\n" % (time.time() - t0))

    stats = {}          # 类型 -> 计数
    state_shown = 0     # 已逐条展示的 state 条数
    last_report = time.time()

    def feed(text, live=True):
        """解析一行 JSON：记账 + 按策略在控制台滚动展示"""
        nonlocal state_shown
        lines.append(text)
        try:
            obj = json.loads(text)
            t = obj.get("type", "?")
        except Exception:
            t = "_bad"
            obj = {}
        stats[t] = stats.get(t, 0) + 1
        if not live or t == "_bad":
            return
        ts = tstamp(obj.get("time", 0.0))
        if t == "platform_added":
            print("[%s] +平台 加入  %-10s 型别 %-8s 阵营 %s" %
                  (ts, obj.get("name", "?"), obj.get("platform_type", "?"), obj.get("side", "?")))
        elif t == "platform_removed":
            print("[%s] -平台 移除  %s" % (ts, obj.get("name", "?")))
        elif t == "sim_start":
            print("[%s] ---- 仿真开始 ----" % ts)
        elif t == "sim_complete":
            print("[%s] ---- 仿真结束 ----" % ts)
        elif t == "state":
            # 前 6 条逐条展示，其后每 25 条抽样一条，避免刷屏
            state_shown += 1
            if state_shown <= 6 or state_shown % 25 == 0:
                print("[%s] 状态  %-10s 高度 %6.0f m  速度 %5.1f m/s  航向 %5.1f°  (lat %.4f, lon %.4f)" %
                      (ts, obj.get("name", "?"), obj.get("alt_m", 0.0), obj.get("speed_mps", 0.0),
                       obj.get("heading_deg", 0.0), obj.get("lat", 0.0), obj.get("lon", 0.0)))
        else:
            # weapon_fired 等事件类
            print("[%s] 事件  %s" % (ts, t))

    sock.settimeout(1.0)
    buf = b""
    try:
        while time.time() < deadline:
            try:
                data = sock.recv(4096)
            except socket.timeout:
                # 每 5 秒一条进度心跳，让窗口始终有反馈
                if time.time() - last_report >= 5.0:
                    print("[....] 心跳  已收 %d 行  %r" % (len(lines), stats))
                    last_report = time.time()
                continue
            if not data:
                break  # 服务端关闭连接
            buf += data
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                if line:
                    feed(line.decode("utf-8", "replace"))
            # 收到仿真结束帧：短暂排空后正常退出
            if lines and '"sim_complete"' in lines[-1]:
                time.sleep(0.3)
                try:
                    while True:
                        data = sock.recv(4096)
                        if not data:
                            break
                        buf += data
                        while b"\n" in buf:
                            line, buf = buf.split(b"\n", 1)
                            if line:
                                feed(line.decode("utf-8", "replace"), live=False)
                except Exception:
                    pass
                break
    finally:
        sock.close()

    # 汇总统计
    complete = any('"sim_complete"' in l for l in lines)
    with open(RESULT_FILE, "w", encoding="utf-8") as f:
        f.write("收到总行数: %d\n" % len(lines))
        f.write("类型统计: %r\n" % stats)
        f.write("\n--- 前 8 行 ---\n")
        f.write("\n".join(lines[:8]))
        f.write("\n\n--- 后 5 行 ---\n")
        f.write("\n".join(lines[-5:]))
        f.write("\n")

    print("\n" + "-" * 62)
    print(" 收流统计: 共 %d 行" % len(lines))
    for k, v in sorted(stats.items(), key=lambda kv: -kv[1]):
        print("   %-18s %5d" % (k, v))
    print(" 明细已写入: %s" % RESULT_FILE)
    print(" 结论: %s" % ("PASS - 收到 sim_complete, 数据流完整" if complete
                        else "FAIL - 未收到 sim_complete (中断/超时)"))
    print("-" * 62)
    sys.exit(0 if complete else 1)


if __name__ == "__main__":
    main()
