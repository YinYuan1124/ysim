# -*- coding: utf-8 -*-
# ****************************************************************************
# 二次开发（e2e 控制面板）：无头端到端验证
# 全链路：run_demo.py 桥接器 → mission.exe（start_paused）→ 控制指令（WS→
# 桥接→TCP 31610）→ 数据流（WS 31601）行为断言 → 重置（进程重启）→ 清场。
# 断言：起步暂停冻结 / resume 推进 / pause 冻结 / set_rate 4 加速 / reset
#       进程重启 / 新进程再次起步暂停 / 树杀清理无残留。输出 RESULT JSON 行。
# ****************************************************************************
import base64
import hashlib
import json
import os
import socket
import struct
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
TEMP = Path(r"e:\afsim\ysim\temp")
BRIDGE = str(HERE / "run_demo.py")
HTTP_PORT = 18080
DATA_HOST, DATA_PORT = "127.0.0.1", 31601
WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


class WSClient:
    """通用 WebSocket 客户端（发送帧带掩码，接收帧不掩码）"""

    def __init__(self, host, port, timeout=5):
        self.sock = socket.create_connection((host, port), timeout=timeout)
        key = base64.b64encode(os.urandom(16)).decode()
        req = ("GET /ws HTTP/1.1\r\nHost: %s:%d\r\nUpgrade: websocket\r\n"
               "Connection: Upgrade\r\nSec-WebSocket-Key: %s\r\n"
               "Sec-WebSocket-Version: 13\r\n\r\n" % (host, port, key))
        self.sock.sendall(req.encode())
        resp = b""
        while b"\r\n\r\n" not in resp:
            chunk = self.sock.recv(1024)
            if not chunk:
                raise ConnectionError("handshake EOF")
            resp += chunk
        status = resp.split(b"\r\n", 1)[0]
        accept = [l.split(b": ", 1)[1] for l in resp.split(b"\r\n\r\n")[0].split(b"\r\n")
                  if l.lower().startswith(b"sec-websocket-accept")][0]
        expect = base64.b64encode(hashlib.sha1(
            (key + WS_GUID).encode()).digest()).decode()
        assert b"101" in status and accept.decode() == expect, "WS 握手失败"
        self.sock.settimeout(1.0)

    def send_text(self, text):
        data = text.encode("utf-8")
        n = len(data)
        mask = os.urandom(4)
        if n < 126:
            hdr = struct.pack(">BB", 0x81, 0x80 | n)
        elif n < 65536:
            hdr = struct.pack(">BBH", 0x81, 0x80 | 126, n)
        else:
            hdr = struct.pack(">BBQ", 0x81, 0x80 | 127, n)
        masked = bytes(b ^ mask[i % 4] for i, b in enumerate(data))
        self.sock.sendall(hdr + mask + masked)

    def _recv_exact(self, n):
        data = b""
        while len(data) < n:
            chunk = self.sock.recv(n - len(data))
            if not chunk:
                raise ConnectionError("socket closed")
            data += chunk
        return data

    def recv_text(self):
        """返回 (帧文本|None)；None=本轮超时；断开抛 ConnectionError"""
        try:
            hdr = self._recv_exact(2)
        except socket.timeout:
            return None
        opcode, ln = hdr[0] & 0x0F, hdr[1] & 0x7F
        if ln == 126:
            ln = struct.unpack(">H", self._recv_exact(2))[0]
        elif ln == 127:
            ln = struct.unpack(">Q", self._recv_exact(8))[0]
        if ln > (1 << 20):
            raise ConnectionError("oversized frame")
        data = self._recv_exact(ln) if ln else b""
        if opcode == 0x8:
            raise ConnectionError("close frame")
        return data.decode("utf-8", "replace") if opcode == 0x1 else ""


class DataListener:
    """数据流 WS 客户端：收帧计数与仿真时间跟踪"""

    def __init__(self):
        self.ws = WSClient(DATA_HOST, DATA_PORT)
        self.frames = []
        self.closed = False

    def drain(self, seconds):
        end = time.time() + seconds
        while time.time() < end:
            try:
                t = self.ws.recv_text()
            except (ConnectionError, socket.timeout):
                self.closed = True
                return
            if t:
                try:
                    self.frames.append(json.loads(t))
                except json.JSONDecodeError:
                    pass

    def sim_time(self):
        ts = [f.get("time") for f in self.frames
              if f.get("type") in ("state", "heartbeat") and f.get("time") is not None]
        return max(ts) if ts else None

    def added(self):
        return [f for f in self.frames if f.get("type") == "platform_added"]

    def advance(self):
        """最近 1 帧时间与历史最大时间之差（负=冻结后无新帧）"""
        ts = [f.get("time") for f in self.frames
              if f.get("type") in ("state", "heartbeat") and f.get("time") is not None]
        return (ts[-1] - ts[0]) if len(ts) >= 2 else 0.0


def ctrl_cmd(ctrl, action, value=None):
    msg = {"action": action}
    if value is not None:
        msg["value"] = value
    ctrl.send_text(json.dumps(msg))
    deadline = time.time() + 20
    while time.time() < deadline:
        try:
            t = ctrl.recv_text()
        except ConnectionError:
            raise AssertionError("控制通道断开: %s" % action)
        if t:
            return json.loads(t)
    raise AssertionError("控制指令超时: %s" % action)


def kill_stray():
    subprocess.run(["taskkill", "/F", "/IM", "mission.exe", "/T"],
                   capture_output=True)
    time.sleep(1)


def main():
    kill_stray()
    bridge = subprocess.Popen(
        [sys.executable, BRIDGE, "--no-browser", "--http-port", str(HTTP_PORT)],
        stdout=open(TEMP / "e2e_ctrl_bridge.out", "w"),
        stderr=subprocess.STDOUT)

    ok = True
    def check(name, cond, detail=""):
        nonlocal ok
        print("  [%s] %s %s" % ("PASS" if cond else "FAIL", name, detail))
        ok = ok and cond

    try:
        # 1) 数据流连接：快照补推 + 起步暂停（时间冻结）
        data = DataListener()
        data.drain(4)
        check("起步快照补推 >=4 平台", len(data.added()) >= 4,
              "n=%d" % len(data.added()))
        t0 = data.sim_time() or 0
        data.drain(3)
        check("start_paused 时间冻结（3s 推进 <0.5s）",
              (data.sim_time() or 0) - t0 < 0.5,
              "t=%s" % data.sim_time())

        # 2) 控制通道经桥接器：status
        ctrl = WSClient("127.0.0.1", HTTP_PORT)
        st = ctrl_cmd(ctrl, "status")
        check("status: paused=true / 平台=4",
              st.get("paused") is True and st.get("platform_count") == 4,
              "paused=%s pc=%s" % (st.get("paused"), st.get("platform_count")))

        # 3) resume：推进
        r = ctrl_cmd(ctrl, "resume")
        data.frames.clear()
        data.drain(3)
        check("resume 后 3s 推进 >2s", (data.sim_time() or 0) > 2.0,
              "t=%s ok=%s" % (data.sim_time(), r.get("ok")))

        # 4) pause：冻结
        ctrl_cmd(ctrl, "pause")
        tp = data.sim_time() or 0
        data.drain(2.5)
        check("pause 后时间冻结（<0.3s）", (data.sim_time() or 0) - tp < 0.3,
              "delta=%s" % ((data.sim_time() or 0) - tp))

        # 5) set_rate 4：加速推进
        r = ctrl_cmd(ctrl, "set_rate", 4)
        ctrl_cmd(ctrl, "resume")
        data.frames.clear()
        data.drain(3)
        check("set_rate 4 后 3s 推进 >6s", (data.sim_time() or 0) > 6.0,
              "t=%s detail=%s" % (data.sim_time(), r.get("detail")))

        # 6) reset：进程重启（旧数据流断开 → 新进程起步暂停）
        r = ctrl_cmd(ctrl, "reset")
        check("reset 回执 ok", r.get("ok") is True, str(r.get("detail")))
        deadline = time.time() + 15
        while time.time() < deadline and not data.closed:
            data.drain(0.5)
        check("reset 后旧数据流断开", data.closed)
        data2 = DataListener()
        data2.drain(4)
        check("reset 后新快照 >=4 平台", len(data2.added()) >= 4,
              "n=%d" % len(data2.added()))
        t1 = data2.sim_time() or 0
        data2.drain(2)
        check("reset 后新进程起步暂停（推进 <0.5s）",
              (data2.sim_time() or 0) - t1 < 0.5, "t=%s" % data2.sim_time())
        st2 = ctrl_cmd(ctrl, "status")
        check("reset 后 status: paused=true / 倍率=1",
              st2.get("paused") is True and abs(st2.get("clock_rate", 0) - 1) < 1e-6,
              "paused=%s rate=%s" % (st2.get("paused"), st2.get("clock_rate")))

        # 7) 页面可静态访问（HTTP 200 + 含控制按钮）
        import urllib.request
        page = urllib.request.urlopen(
            "http://127.0.0.1:%d/" % HTTP_PORT, timeout=5).read().decode("utf-8")
        check("态势页可访问且含控制控件",
              all(k in page for k in ("btnStart", "btnPause", "btnReset", "rateSel")))
    except Exception as exc:
        check("验证过程无异常", False, repr(exc))
    finally:
        subprocess.run(["taskkill", "/F", "/T", "/PID", str(bridge.pid)],
                       capture_output=True)
        time.sleep(2)
        remain = subprocess.run(["tasklist", "/FI", "IMAGENAME eq mission.exe"],
                                capture_output=True, text=True).stdout
        check("树杀清理无 mission 残留", "mission.exe" not in remain)

    print("RESULT: %s" % json.dumps({"ok": ok}, ensure_ascii=False))
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
