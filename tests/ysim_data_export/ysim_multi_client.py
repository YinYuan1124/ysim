# -*- coding: utf-8 -*-
# ****************************************************************************
# 二次开发（ext-01 阶段 B）：ysim_data_export 三通道并发验证客户端
# 通道：
#   1. TCP（延迟 4s 接入）——验证快照补推：接入即收 4x platform_added + 4x state
#   2. WebSocket（裸 socket 实现 RFC 6455 握手 + 文本帧解析，零第三方依赖）
#   3. UDP 组播（加入 239.16.16.16 组收数据报，每报一行 JSON）
# 输出：统计与样例帧写入 temp/ysim_multi_result.txt
# ****************************************************************************
import base64
import json
import os
import socket
import struct
import threading
import time

HOST = "127.0.0.1"
TCP_PORT = 31600
WS_PORT = 31601
UDP_GROUP = "239.16.16.16"
UDP_PORT = 31602
RESULT_FILE = r"E:\afsim\ysim\temp\ysim_multi_result.txt"

results = {}
lock = threading.Lock()


def stats_of(lines):
    """统计行数/类型分布/解析失败数/是否含 sim_complete"""
    stats, bad = {}, 0
    for text in lines:
        try:
            obj = json.loads(text)
            t = obj.get("type", "?")
        except Exception:
            bad += 1
            t = "_bad"
        stats[t] = stats.get(t, 0) + 1
    return {"total": len(lines), "bad": bad,
            "types": stats, "complete": '"sim_complete"' in " ".join(lines[-3:])}


def record(name, lines, extra=""):
    st = stats_of(lines)
    with lock:
        results[name] = {"lines": lines, "stats": st, "extra": extra}


def read_lines_until_complete(sock, deadline):
    """按行读取直到 sim_complete 或超时（流式通道通用）"""
    lines, buf = [], b""
    sock.settimeout(1.0)
    while time.time() < deadline:
        try:
            data = sock.recv(4096)
        except socket.timeout:
            continue
        if not data:
            break
        buf += data
        while b"\n" in buf:
            line, buf = buf.split(b"\n", 1)
            if line:
                lines.append(line.decode("utf-8", "replace"))
        if lines and '"sim_complete"' in lines[-1]:
            break
    return lines


def tcp_client():
    """TCP 客户端：延迟 4 秒接入，验证快照补推"""
    time.sleep(4.0)  # 等仿真跑起来再接入（中途接入场景）
    sock, deadline = None, time.time() + 50
    while time.time() < deadline:
        try:
            sock = socket.create_connection((HOST, TCP_PORT), timeout=1)
            break
        except OSError:
            time.sleep(0.05)
    if sock is None:
        record("tcp", [], extra="FAIL: connect timeout")
        return
    # 接入后 2 秒内收集消息（快照应立即到达，无需等下一帧）
    lines, buf, t0 = [], b"", time.time()
    sock.settimeout(0.5)
    while time.time() - t0 < 2.0:
        try:
            data = sock.recv(8192)
        except socket.timeout:
            continue
        if not data:
            break
        buf += data
        while b"\n" in buf:
            line, buf = buf.split(b"\n", 1)
            if line:
                lines.append(line.decode("utf-8", "replace"))
    snapshot_added = sum(1 for l in lines if '"platform_added"' in l)
    snapshot_state = sum(1 for l in lines if '"type":"state"' in l)
    # 继续收流直到 sim_complete
    rest = read_lines_until_complete(sock, time.time() + 30)
    sock.close()
    record("tcp", lines + rest,
           extra="接入2s内快照: platform_added=%d state=%d（要求=4/4）" % (snapshot_added, snapshot_state))


def ws_client():
    """WebSocket 客户端：裸 socket 握手 + 文本帧解析"""
    sock, deadline = None, time.time() + 30
    while time.time() < deadline:
        try:
            sock = socket.create_connection((HOST, WS_PORT), timeout=1)
            break
        except OSError:
            time.sleep(0.05)
    if sock is None:
        record("websocket", [], extra="FAIL: connect timeout")
        return
    # RFC 6455 握手
    key = base64.b64encode(os.urandom(16)).decode()
    req = ("GET / HTTP/1.1\r\nHost: %s:%d\r\nUpgrade: websocket\r\n"
           "Connection: Upgrade\r\nSec-WebSocket-Key: %s\r\n"
           "Sec-WebSocket-Version: 13\r\n\r\n" % (HOST, WS_PORT, key))
    sock.sendall(req.encode())
    resp = b""
    while b"\r\n\r\n" not in resp:
        chunk = sock.recv(1024)
        if not chunk:
            break
        resp += chunk
    if b"101" not in resp.split(b"\r\n", 1)[0] or b"websocket" not in resp.lower():
        record("websocket", [], extra="FAIL: handshake rejected: %r" % resp[:120])
        sock.close()
        return
    lines = []

    def read_exact(n):
        data = b""
        while len(data) < n:
            chunk = sock.recv(n - len(data))
            if not chunk:
                raise EOFError
            data += chunk
        return data

    sock.settimeout(1.0)
    deadline = time.time() + 50
    try:
        while time.time() < deadline:
            try:
                hdr = read_exact(2)
            except (socket.timeout, EOFError):
                continue
            b0, b1 = hdr[0], hdr[1]
            opcode, ln = b0 & 0x0F, b1 & 0x7F
            if ln == 126:
                ln = struct.unpack(">H", read_exact(2))[0]
            elif ln == 127:
                ln = struct.unpack(">Q", read_exact(8))[0]
            if ln > (1 << 20):
                break
            payload = read_exact(ln) if ln else b""
            if opcode == 0x1:  # 文本帧
                lines.append(payload.decode("utf-8", "replace"))
                if '"sim_complete"' in lines[-1]:
                    break
            elif opcode == 0x8:  # 服务端关闭
                break
    except EOFError:
        pass
    sock.close()
    record("websocket", lines)


def udp_client():
    """UDP 组播客户端：加入组播组收数据报（每报一行 JSON）"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(("", UDP_PORT))
        mreq = struct.pack("4sl", socket.inet_aton(UDP_GROUP), socket.INADDR_ANY)
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
    except OSError as e:
        record("udp", [], extra="FAIL: join group error %s" % e)
        return
    lines = []
    sock.settimeout(1.0)
    deadline = time.time() + 50
    while time.time() < deadline:
        try:
            data, _ = sock.recvfrom(65535)
        except socket.timeout:
            continue
        text = data.decode("utf-8", "replace").strip()
        if text:
            lines.append(text)
            if '"sim_complete"' in text:
                break
    sock.close()
    record("udp", lines)


def main():
    threads = [threading.Thread(target=ws_client),
               threading.Thread(target=udp_client),
               threading.Thread(target=tcp_client)]
    for t in threads:
        t.start()
    for t in threads:
        t.join(timeout=75)

    verdict = []
    with open(RESULT_FILE, "w", encoding="utf-8") as f:
        for name in ("tcp", "websocket", "udp"):
            r = results.get(name)
            if r is None:
                f.write("[%s] FAIL: 无结果\n" % name)
                verdict.append("%s:FAIL" % name)
                continue
            st = r["stats"]
            f.write("[%s] 总行数=%d 解析失败=%d 含sim_complete=%s\n" %
                    (name, st["total"], st["bad"], st["complete"]))
            f.write("  类型统计: %r\n" % st["types"])
            if r["extra"]:
                f.write("  %s\n" % r["extra"])
            for line in r["lines"][:3]:
                f.write("  首> %s\n" % line.strip()[:150])
            for line in r["lines"][-2:]:
                f.write("  尾> %s\n" % line.strip()[:150])
            f.write("\n")
            ok = (st["total"] > 0 and st["bad"] == 0 and st["complete"])
            if name == "tcp" and ok:
                # 快照验证：接入 2s 内应收到 4 平台元信息 + 4 状态
                ok = ("platform_added=4" in r["extra"] and "state=4" in r["extra"])
            verdict.append("%s:%s" % (name, "PASS" if ok else "FAIL"))
        f.write("结论: %s\n" % " ".join(verdict))
    print(" ".join(verdict))


if __name__ == "__main__":
    main()
