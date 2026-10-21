# -*- coding: utf-8 -*-
# ****************************************************************************
# 二次开发（ext-01 阶段 B）：ysim_data_export 三通道并发直播客户端
# 通道：
#   1. TCP（延迟 4s 接入）——验证快照补推：接入即收 4x platform_added + 4x state
#   2. WebSocket（裸 socket 实现 RFC 6455 握手 + 文本帧解析，零第三方依赖）
#   3. UDP 组播（加入 239.16.16.16 组收数据报，每报一行 JSON）
# 输出：控制台按 [TCP]/[WS]/[UDP] 前缀实时直播各通道动态 + 统计样例写
#       temp/ysim_multi_result.txt
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
print_lock = threading.Lock()


def live(tag, text):
    """线程安全的控制台直播（前缀区分通道）"""
    with print_lock:
        print("[%-3s] %s" % (tag, text))


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


def progress_tag(tag, lines, last):
    """每 2 秒打印一次通道进度行"""
    if time.time() - last[0] >= 2.0:
        st = stats_of(lines)
        live(tag, "收流中... %d 行  %r" % (st["total"], st["types"]))
        last[0] = time.time()


def read_lines_until_complete(sock, deadline, tag, live_start=False):
    """按行读取直到 sim_complete 或超时（流式通道通用）；live_start=True 时前 3 条逐条直播"""
    lines, buf, last = [], b"", [time.time()]
    shown = 0
    sock.settimeout(1.0)
    while time.time() < deadline:
        try:
            data = sock.recv(4096)
        except socket.timeout:
            progress_tag(tag, lines, last)
            continue
        if not data:
            break
        buf += data
        while b"\n" in buf:
            line, buf = buf.split(b"\n", 1)
            if line:
                lines.append(line.decode("utf-8", "replace"))
                if live_start and shown < 3:
                    shown += 1
                    live(tag, "首条> %s" % lines[-1].strip()[:110])
        if lines and '"sim_complete"' in lines[-1]:
            break
        progress_tag(tag, lines, last)
    return lines


def tcp_client():
    """TCP 客户端：延迟 4 秒接入，验证快照补推"""
    live("TCP", "等待 4s 后中途接入（验证快照补推）...")
    time.sleep(4.0)  # 等仿真跑起来再接入（中途接入场景）
    sock, deadline = None, time.time() + 50
    while time.time() < deadline:
        try:
            sock = socket.create_connection((HOST, TCP_PORT), timeout=1)
            break
        except OSError:
            time.sleep(0.05)
    if sock is None:
        live("TCP", "FAIL: connect timeout")
        record("tcp", [], extra="FAIL: connect timeout")
        return
    live("TCP", "已连接 %s:%d（中途接入, 等待快照...）" % (HOST, TCP_PORT))
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
    live("TCP", "接入 2s 内: platform_added=%d state=%d（要求 >=4/4, 含同窗活流）"
         % (snapshot_added, snapshot_state))
    # 继续收流直到 sim_complete
    rest = read_lines_until_complete(sock, time.time() + 30, "TCP")
    sock.close()
    record("tcp", lines + rest,
           extra="接入2s内: platform_added=%d state=%d（要求>=4/4）" % (snapshot_added, snapshot_state))


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
        live("WS", "FAIL: connect timeout")
        record("websocket", [], extra="FAIL: connect timeout")
        return
    live("WS", "已连接 %s:%d, 发起 RFC 6455 握手..." % (HOST, WS_PORT))
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
        live("WS", "FAIL: handshake rejected: %r" % resp[:120])
        record("websocket", [], extra="FAIL: handshake rejected: %r" % resp[:120])
        sock.close()
        return
    # 关键修复：服务端 101 与首批快照帧背靠背发送，握手 recv 可能一并读到
    # \r\n\r\n 之后的帧字节——必须保留为帧解析的初始缓冲，否则流错位全乱码
    leftover = resp.split(b"\r\n\r\n", 1)[1]
    live("WS", "握手成功 (HTTP 101 Upgrade), 开始接收文本帧..." +
         ("(握手期已预读 %d 字节帧数据)" % len(leftover) if leftover else ""))
    lines = []

    def deliver(payload):
        text = payload.decode("utf-8", "replace")
        lines.append(text)
        if len(lines) <= 3:
            live("WS", "首条> %s" % text.strip()[:110])
        return '"sim_complete"' in text

    # 先消化握手期预读的完整帧（可能已含多帧）
    pos = 0
    while pos + 2 <= len(leftover):
        b0, b1 = leftover[pos], leftover[pos + 1]
        opcode, ln = b0 & 0x0F, b1 & 0x7F
        hdr = 2
        if ln == 126:
            if pos + 4 > len(leftover):
                break
            ln = struct.unpack(">H", leftover[pos + 2:pos + 4])[0]
            hdr = 4
        elif ln == 127:
            if pos + 10 > len(leftover):
                break
            ln = struct.unpack(">Q", leftover[pos + 2:pos + 10])[0]
            hdr = 10
        if pos + hdr + ln > len(leftover):
            break
        if opcode == 0x1:
            if deliver(leftover[pos + hdr:pos + hdr + ln]):
                sock.close()
                record("websocket", lines)
                return
        pos += hdr + ln
    buf = bytearray(leftover[pos:])  # 半帧残片转交主循环（bytearray 支持 del 消费）

    def parse_buf():
        """从 buf 消化所有完整帧；返回 (close_requested, complete_seen, consumed)"""
        pos = 0
        close_req, complete = False, False
        while pos + 2 <= len(buf):
            b0, b1 = buf[pos], buf[pos + 1]
            opcode, ln = b0 & 0x0F, b1 & 0x7F
            hdr = 2
            if ln == 126:
                if pos + 4 > len(buf):
                    break
                ln = struct.unpack(">H", buf[pos + 2:pos + 4])[0]
                hdr = 4
            elif ln == 127:
                if pos + 10 > len(buf):
                    break
                ln = struct.unpack(">Q", buf[pos + 2:pos + 10])[0]
                hdr = 10
            if ln > (1 << 20):
                close_req = True  # 异常长度：放弃解析
                break
            if pos + hdr + ln > len(buf):
                break
            if opcode == 0x1:
                if deliver(buf[pos + hdr:pos + hdr + ln]):
                    complete = True
            elif opcode == 0x8:
                close_req = True
            pos += hdr + ln
        return close_req, complete, pos

    sock.settimeout(1.0)
    deadline = time.time() + 50
    last = [time.time()]
    while time.time() < deadline:
        # 先消化缓冲中的完整帧
        close_req, complete, consumed = parse_buf()
        del buf[:consumed]
        if close_req or complete:
            break
        # 再收新数据（EOF 即服务端关闭：立即退出，不再空转）
        try:
            data = sock.recv(65536)
        except socket.timeout:
            progress_tag("WS", lines, last)
            continue
        if not data:
            live("WS", "连接被服务端关闭 (EOF)，共收 %d 行" % len(lines))
            break
        buf += data
        progress_tag("WS", lines, last)
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
        live("UDP", "FAIL: join group error %s" % e)
        record("udp", [], extra="FAIL: join group error %s" % e)
        return
    live("UDP", "已加入组播 %s:%d, 等待数据报..." % (UDP_GROUP, UDP_PORT))
    lines = []
    sock.settimeout(1.0)
    deadline = time.time() + 50
    last = [time.time()]
    while time.time() < deadline:
        try:
            data, _ = sock.recvfrom(65535)
        except socket.timeout:
            progress_tag("UDP", lines, last)
            continue
        text = data.decode("utf-8", "replace").strip()
        if text:
            lines.append(text)
            if len(lines) <= 3:
                live("UDP", "首条> %s" % text[:110])
            if '"sim_complete"' in text:
                break
        progress_tag("UDP", lines, last)
    sock.close()
    record("udp", lines)


def main():
    print("=" * 62)
    print(" ysim_data_export 插件 - 三通道并发直播演示")
    print(" TCP %-5d | WS %-5d | UDP组播 %s:%d" % (TCP_PORT, WS_PORT, UDP_GROUP, UDP_PORT))
    print("=" * 62)
    threads = [threading.Thread(target=ws_client),
               threading.Thread(target=udp_client),
               threading.Thread(target=tcp_client)]
    for t in threads:
        t.start()
    for t in threads:
        t.join(timeout=75)

    verdict = []
    print("\n" + "-" * 62)
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
                # 快照验证：接入 2s 内至少收到 4 平台元信息 + 4 状态（活流同窗到达属正常）
                ok = ("platform_added=4" in r["extra"] and
                      int(r["extra"].split("state=")[1].split("（")[0]) >= 4)
            verdict.append("%s:%s" % (name, "PASS" if ok else "FAIL"))
        f.write("结论: %s\n" % " ".join(verdict))
    for v in verdict:
        print(" %s" % v)
    print(" 明细已写入: %s" % RESULT_FILE)
    print("-" * 62)


if __name__ == "__main__":
    main()
