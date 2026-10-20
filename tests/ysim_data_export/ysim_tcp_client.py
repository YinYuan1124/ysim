# -*- coding: utf-8 -*-
# ****************************************************************************
# 二次开发（ext-01）：ysim_data_export 最小 TCP 验证客户端
# 行为：循环重试连接 127.0.0.1:31600 → 逐行接收 JSON 消息 → 收到 sim_complete 后退出
# 输出：统计结果写入 temp/ysim_client_result.txt
# ****************************************************************************
import json
import os
import socket
import sys
import time

HOST = "127.0.0.1"
PORT = int(os.environ.get("YSIM_PORT", "31600"))
RESULT_FILE = r"E:\afsim\ysim\temp\ysim_client_result.txt"


def main():
    lines = []
    sock = None
    deadline = time.time() + 60  # 总超时：1 分钟（含等待 mission 启动）

    # 循环重试连接：mission 初始化期间端口才开放（20ms 高频轮询抢在首帧前连上）
    while time.time() < deadline:
        try:
            sock = socket.create_connection((HOST, PORT), timeout=1)
            break
        except OSError:
            time.sleep(0.02)

    if sock is None:
        with open(RESULT_FILE, "w", encoding="utf-8") as f:
            f.write("FAIL: 60 秒内未能连接数据出口 %s:%d\n" % (HOST, PORT))
        print("FAIL: connect timeout")
        sys.exit(1)

    sock.settimeout(1.0)
    buf = b""
    try:
        while time.time() < deadline:
            try:
                data = sock.recv(4096)
            except socket.timeout:
                continue
            if not data:
                break  # 服务端关闭连接
            buf += data
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                if line:
                    lines.append(line.decode("utf-8", "replace"))
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
                                lines.append(line.decode("utf-8", "replace"))
                except Exception:
                    pass
                break
    finally:
        sock.close()

    # 汇总统计
    stats = {}
    for text in lines:
        try:
            obj = json.loads(text)
            t = obj.get("type", "?")
        except Exception:
            t = "_bad"
        stats[t] = stats.get(t, 0) + 1

    with open(RESULT_FILE, "w", encoding="utf-8") as f:
        f.write("收到总行数: %d\n" % len(lines))
        f.write("类型统计: %r\n" % stats)
        f.write("\n--- 前 8 行 ---\n")
        f.write("\n".join(lines[:8]))
        f.write("\n\n--- 后 5 行 ---\n")
        f.write("\n".join(lines[-5:]))
        f.write("\n")
    print("OK lines=%d stats=%r" % (len(lines), stats))


if __name__ == "__main__":
    main()
