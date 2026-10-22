# -*- coding: utf-8 -*-
# ****************************************************************************
# 二次开发（端到端联调示例）：无头验证脚本
# 证明 situation.html 依赖的管道可用：mission.exe 后台 → ysim_data_export
# WS 推流 → 裸 RFC 6455 客户端收流 → 断言平台快照补推 + 状态流 + 心跳。
# 断言满足后提前终止 mission（不等待 120s 演示时长），输出 RESULT JSON 行。
# ****************************************************************************
import base64
import json
import os
import socket
import struct
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
TEMP = Path(r"e:\Project\ysim\temp")
MISSION_EXE = r"e:\Project\ysim\build\Release\mission.exe"
SCENARIO = str(HERE / "scenarios" / "e2e_visual_scenario.txt")
CWD_DEMO = r"e:\Project\ysim\build\demos\air_to_air"
WS_HOST, WS_PORT = "127.0.0.1", 31601


def ws_collect(stop_at, state_min, added_min):
    """裸 WS 客户端：RFC 6455 握手 + 文本帧逐行收集，断言满足即返回"""
    sock = None
    deadline = time.time() + 30
    while time.time() < deadline:
        try:
            sock = socket.create_connection((WS_HOST, WS_PORT), timeout=1)
            break
        except OSError:
            time.sleep(0.05)
    if sock is None:
        return {"error": "connect timeout"}
    key = base64.b64encode(os.urandom(16)).decode()
    req = ("GET / HTTP/1.1\r\nHost: %s:%d\r\nUpgrade: websocket\r\n"
           "Connection: Upgrade\r\nSec-WebSocket-Key: %s\r\n"
           "Sec-WebSocket-Version: 13\r\n\r\n" % (WS_HOST, WS_PORT, key))
    sock.sendall(req.encode())
    resp = b""
    while b"\r\n\r\n" not in resp:
        chunk = sock.recv(1024)
        if not chunk:
            break
        resp += chunk
    if b"101" not in resp.split(b"\r\n", 1)[0]:
        sock.close()
        return {"error": "handshake rejected"}

    def read_exact(n):
        data = b""
        while len(data) < n:
            c = sock.recv(n - len(data))
            if not c:
                raise EOFError
            data += c
        return data

    types, states, added = {}, [], []
    hb_deadline = None  # 样本满足后延长收流窗口至跨过一个心跳周期（heartbeat_interval 5s）
    sock.settimeout(1.0)
    try:
        while time.time() < stop_at:
            try:
                hdr = read_exact(2)
            except (socket.timeout, EOFError):
                if hb_deadline and time.time() > hb_deadline:
                    break
                continue
            opcode, ln = hdr[0] & 0x0F, hdr[1] & 0x7F
            if ln == 126:
                ln = struct.unpack(">H", read_exact(2))[0]
            elif ln == 127:
                ln = struct.unpack(">Q", read_exact(8))[0]
            if ln > (1 << 20):
                break
            payload = read_exact(ln) if ln else b""
            if opcode != 0x1:
                continue
            try:
                obj = json.loads(payload.decode("utf-8", "replace"))
            except Exception:
                types["_bad"] = types.get("_bad", 0) + 1
                continue
            t = obj.get("type", "?")
            types[t] = types.get(t, 0) + 1
            if t == "state":
                states.append(obj)
            elif t == "platform_added":
                added.append(obj)
            if len(states) >= state_min and len(added) >= added_min and hb_deadline is None:
                hb_deadline = time.time() + 7.0  # 等首帧心跳（5s 周期 + 余量）再收尾
            if t == "heartbeat" and hb_deadline is not None:
                break
    except EOFError:
        pass
    sock.close()
    return {"types": types, "n_state": len(states), "n_added": len(added),
            "sample": states[0] if states else None,
            "added_names": sorted({a.get("name", "") for a in added})}


def main():
    out = open(TEMP / "e2e_visual_mission.out", "w")
    m = subprocess.Popen([MISSION_EXE, "-rt", SCENARIO], cwd=CWD_DEMO,
                         stdout=out, stderr=subprocess.STDOUT)
    time.sleep(3)  # 等 mission 起稳、WS 服务就绪
    r = ws_collect(stop_at=time.time() + 60, state_min=20, added_min=4)
    m.terminate()
    try:
        m.wait(timeout=10)
    except subprocess.TimeoutExpired:
        m.kill()
    out.close()

    ok = True
    def check(name, cond, detail=""):
        nonlocal ok
        print("  [%s] %s %s" % ("PASS" if cond else "FAIL", name, detail))
        ok = ok and cond

    check("WS 握手成功", "error" not in r, r.get("error", ""))
    if "error" not in r:
        types = r["types"]
        check("platform_added >=4（快照/运行期）", r["n_added"] >= 4,
              "n=%d names=%s" % (r["n_added"], r["added_names"]))
        check("state 状态流 >=20 帧", r["n_state"] >= 20, "n=%d" % r["n_state"])
        check("心跳帧存在", types.get("heartbeat", 0) >= 1, "n=%d" % types.get("heartbeat", 0))
        check("JSON 解析零失败", types.get("_bad", 0) == 0, "bad=%d" % types.get("_bad", 0))
        s = r.get("sample") or {}
        need = ["id", "name", "side", "lat", "lon", "alt_m", "speed_mps", "heading_deg"]
        check("state 字段齐全（态势页渲染依赖）",
              all(k in s for k in need), "keys=%s" % sorted(s.keys()))
    rc = m.returncode
    print("RESULT: %s" % json.dumps({"ok": ok, "mission_rc": rc},
                                    ensure_ascii=False))
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
